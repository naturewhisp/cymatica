#include "procedural_scenario.h"

#include "audio_probe.h"
#include "loudness.h"
#include "music_planner.h"
#include "onset.h"
#include "staff_view.h"
#include "stft.h"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <limits>
#include <map>
#include <sstream>

namespace cymatica::cacp {

using nlohmann::json;

namespace {

constexpr std::uint32_t kFs = 48000;
constexpr std::size_t kCaptureFrames = 8'900'000; // > 8,825,200 frames rendered by fixture_probe_v1
constexpr double kTrueTrialDbtp = -1.0;           // trial value of ADR-0004 §5: measured, never a gate
constexpr double kStaffPixelsPerSecond = 100.0;
constexpr double kOnsetToleranceS = 0.050;

json optNum(const std::optional<double>& v) { return (v && std::isfinite(*v)) ? json(*v) : json(nullptr); }
json num(double v) { return std::isfinite(v) ? json(v) : json(nullptr); }

// Detected onsets of one role, offset-corrected, classified against the applied NoteOns and NoteOffs
std::vector<StaffOnset> classifiedOnsets(std::size_t role, std::span<const float> stem, const RoleOnsetProfile& profile,
                                         const std::vector<double>& refs, const std::vector<double>& releases) {
    std::vector<StaffOnset> out;
    OnsetParams p{};
    p.delta = profile.delta;
    const OnsetDetection det = detectOnsetsSeconds(monoFromStereo(stem), p, kFs);
    if (det.status != AnalysisStatus::Ok) return out;
    std::vector<double> est = det.timesS;
    for (auto& t : est) t -= profile.offsetS;
    std::sort(est.begin(), est.end());
    const OnsetScore sc = matchOnsetsMaxCardinality(refs, est, kOnsetToleranceS); // indices refer to the sorted lists
    std::vector<bool> matched(est.size(), false);
    for (const auto& pr : sc.pairs) matched[pr.estimate] = true;
    for (std::size_t i = 0; i < est.size(); ++i) {
        if (est[i] < 0.0) continue;
        StaffOnsetKind kind = StaffOnsetKind::Matched;
        if (!matched[i]) {
            const auto rel = std::lower_bound(releases.begin(), releases.end(), est[i] - kOnsetToleranceS);
            kind = (rel != releases.end() && *rel <= est[i] + kOnsetToleranceS) ? StaffOnsetKind::Release : StaffOnsetKind::Unmatched;
        }
        out.push_back({role, static_cast<std::uint64_t>(std::llround(est[i] * kFs)), kind});
    }
    return out;
}

} // namespace

std::vector<std::string> proceduralBindingFailures(const audio::FixtureProbeResult& f) {
    std::vector<std::string> v;
    auto need = [&](bool ok, const char* what) {
        if (!ok) v.emplace_back(what);
    };
    need(f.captureCompletePassed, "capture incomplete or discontinuous");
    need(f.determinismPassed, "determinism (offline passes or rendered plan differ)");
    need(!f.goldenApplicable || (f.goldenTracePassed && f.goldenPlanPassed), "golden decision trace or canonical plan");
    need(f.goldenPcmStatus != "FAIL", "golden PCM content hash (audio regression, DIF-5e.1-01)");
    need(f.goldenPcmStatus != "NOT_RUN", "golden PCM status not computed");
    need(f.noteOffPairingPassed, "NoteOn/NoteOff pairing");
    need(f.tempoAckPassed, "transport acks");
    need(f.schedulingAccountingPassed, "scheduling accounting");
    need(f.nonSilentAudioPassed, "non-silent sections");
    need(f.roleActivityPassed, "role activity");
    need(f.silenceBoundaryPassed, "silence after Stop");
    need(f.sampleCeilingPassed, "sample ceiling |x| <= 0.89125 (limiter invariant)");
    need(f.clippedSamples == 0, "clipped samples");
    need(f.dspAnomaliesPassed, "DSP anomalies or non-finite samples");
    need(f.safeStopPassed, "safe stop");
    need(f.eventChainPassed, "O3 event chain");
    need(!f.coordinatorFailed, "coordinator failure");
    // f.truePeakPassed is deliberately absent: measured, never a gate here (ADR-0006 5e-01, DIF-5e.0-01)
    return v;
}

ProceduralResult runProceduralV1(const std::string& outputDir, bool writeFiles) {
    ProceduralResult r;
    StemCapture cap(kCaptureFrames);
    std::vector<float> stereo;
    std::vector<AppliedEventRecord> applied;
    std::vector<audio::MusicIntentEvent> intents;
    std::vector<audio::PlannedNoteOff> noteOffs;
    std::uint64_t appliedOverflow = 0;
    audio::ProbeAnalysisCapture pc{};
    pc.stems = cap.sink();
    pc.stereo = &stereo;
    pc.applied = &applied;
    pc.appliedOverflow = &appliedOverflow;
    pc.intents = &intents;
    pc.noteOffs = &noteOffs;
    const audio::FixtureProbeResult f = audio::runFixtureProbeV1Offline(outputDir, false, audio::FixtureInjectedFault::None, &pc);

    // ---- binding ----
    r.rendered = true;
    r.fixture = f;
    r.failures = proceduralBindingFailures(f);
    r.fixtureProbePassed = f.passed;
    r.pcmContentHash = f.pcmContentHash;
    r.canonicalPlanHash = f.canonicalPlanHash;
    r.contentFingerprint = f.contentFingerprint;
    r.renderedFrames = stereo.size() / 2;
    r.contentEndFrame = f.contentEndFrame;
    r.capture.rendered = true;
    r.capture.probePassed = r.failures.empty(); // the probe verdict here is the binding subset
    r.capture.probeFailure = r.failures.empty() ? std::string{} : r.failures.front();
    r.capture.expectedFrames = static_cast<std::size_t>(f.requestedSampleFrames);
    r.capture.stemFrames = cap.frames();
    r.capture.stemOverflowFrames = cap.overflowFrames();
    r.capture.appliedOverflow = appliedOverflow;
    if (cap.frames() != f.requestedSampleFrames || cap.overflowFrames() != 0 || appliedOverflow != 0) {
        r.failures.push_back("invalid capture: " + captureInvalidReason(r.capture));
    }
    if (r.renderedFrames != f.requestedSampleFrames) r.failures.emplace_back("PCM frames differ from the requested frames");
    const ReconstructionResult rec = reconstructBusMix(cap);
    r.reconstructionStatus = analysisStatusName(rec.status);
    r.reconstructionMismatches = rec.mismatchedSamples;
    if (rec.status != AnalysisStatus::Ok || rec.mismatchedSamples != 0) r.failures.emplace_back("bus-mix reconstruction not exact");

    // ---- measures: true peak and loudness (whole render and per section) ----
    r.truePeakDbtp = f.truePeakDbtp;
    r.truePeakMeasured = r.renderedFrames > 0 && std::isfinite(f.truePeakDbtp);
    r.maxGainReductionDb = f.maxGainReductionDb;
    if (r.truePeakMeasured && r.truePeakDbtp > kTrueTrialDbtp) {
        std::ostringstream s;
        s << "true peak " << std::fixed << std::setprecision(2) << r.truePeakDbtp
          << " dBTP above the -1 dBTP trial value (finding, not a failure; ADR-0004 §4-5)";
        r.findings.push_back(s.str());
    }
    if (!f.passed && r.failures.empty()) r.findings.emplace_back("fixture_probe_v1 FAIL only through its true-peak engineering check");
    const LoudnessResult whole = measureLoudnessStereo(stereo, kFs);
    r.loudnessStatus = whole.supported ? analysisStatusName(whole.status) : "UNSUPPORTED";
    r.integratedLufs = whole.integratedLufs;
    r.maxMomentaryLufs = whole.maxMomentaryLufs;
    r.maxShortTermLufs = whole.maxShortTermLufs;

    const auto clock = audio::MusicPlanner::makeFixtureM24V1Clock();
    std::vector<PackPhrase> phrases;
    if (clock) {
        for (const auto& p : audio::MusicPlanner::makeFixtureM24V1Phrases(*clock)) {
            phrases.push_back({static_cast<std::uint32_t>(phrases.size()), p.sectionIndex, p.startBar, p.barCount, p.startFrame, p.endFrame});
        }
    }
    if (phrases.empty()) r.failures.emplace_back("fixture phrases unavailable");
    std::map<std::uint32_t, std::uint64_t> sectionStart;
    for (const auto& p : phrases) {
        const auto it = sectionStart.find(p.section);
        if (it == sectionStart.end() || p.startFrame < it->second) sectionStart[p.section] = p.startFrame;
    }
    for (auto it = sectionStart.begin(); it != sectionStart.end(); ++it) {
        SectionMeasures sm;
        sm.section = it->first;
        sm.startFrame = it->second;
        const auto next = std::next(it);
        sm.endFrame = (next != sectionStart.end()) ? next->second : f.contentEndFrame;
        if (2 * sm.endFrame <= stereo.size() && sm.endFrame > sm.startFrame) {
            const std::span<const float> slice(stereo.data() + 2 * sm.startFrame, 2 * (sm.endFrame - sm.startFrame));
            const LoudnessResult l = measureLoudnessStereo(slice, kFs);
            sm.loudnessStatus = l.supported ? analysisStatusName(l.status) : "UNSUPPORTED";
            sm.integratedLufs = l.integratedLufs;
            sm.maxMomentaryLufs = l.maxMomentaryLufs;
            sm.maxShortTermLufs = l.maxShortTermLufs;
            sm.truePeakDbtp = audio::analysis::estimateTruePeakDbtp(slice.data(), sm.endFrame - sm.startFrame, 2);
        } else {
            sm.loudnessStatus = "INVALID_RANGE";
        }
        r.sections.push_back(sm);
    }

    // ---- measures: per-role onsets with the frozen profile ----
    const auto frozen = frozenStemProfileV1();
    std::array<std::vector<double>, kRoleCount> refs{}, releases{};
    std::vector<StaffOnset> detections;
    if (frozen) {
        r.onsetProfileSha256 = stemProfileSha256(frozen->roles);
        for (std::size_t role = 0; role < kRoleCount; ++role) {
            const auto mr = static_cast<audio::MusicRole>(role);
            refs[role] = roleReferenceOnsetsS(applied, mr, kFs);
            releases[role] = roleReleaseTimesS(applied, mr, kFs);
            const RoleOnsetEvaluation ev = evaluateRoleOnsets(cap.stem(role), refs[role], frozen->roles[role], kFs, releases[role]);
            RoleOnsetSummary& s = r.onsets[role];
            s.references = ev.references;
            s.detections = ev.detections;
            if (ev.status != AnalysisStatus::Ok) {
                s.status = analysisStatusName(ev.status);
            } else if (ev.references == 0) {
                s.status = "NO_REFERENCES";
            } else {
                s.status = "MEASURED";
                s.matched = ev.at50.matches;
                s.releaseDetections = ev.releaseDetections;
                s.precision50 = ev.at50.precision;
                s.recall50 = ev.at50.recall;
                s.precisionExclReleases = ev.precisionExclReleases;
            }
            const auto d = classifiedOnsets(role, cap.stem(role), frozen->roles[role], refs[role], releases[role]);
            detections.insert(detections.end(), d.begin(), d.end());
        }
    } else {
        r.failures.emplace_back("frozen onset profile unavailable");
    }

    // ---- measures: presence per phrase (whole phrase, no fades) ----
    PackSource src;
    src.name = "fixture_probe_v1 seed 1337";
    for (std::size_t role = 0; role < kRoleCount; ++role) src.stems[role] = cap.stem(role);
    src.applied = applied;
    src.planned = plannedSpans(intents, noteOffs);
    src.phrases = phrases;
    if (frozen) src.onsetProfile = frozen->roles;
    const OnsetIndex idx = buildOnsetIndex(src, kOnsetToleranceS);
    for (const auto& p : phrases) {
        WindowTruth t = measureWindow(src, idx, {p.startFrame, p.endFrame, {true, true, true, true}, 0.0, 0.0}, presenceProfileV1());
        t.id = "phrase_" + std::to_string(p.index);
        t.kind = "phrase";
        t.phrase = static_cast<int>(p.index);
        t.section = p.section;
        t.startBar = p.startBar;
        t.barCount = p.barCount;
        r.phrasePresence.push_back(t);
    }

    r.status = r.failures.empty() ? "PASS" : "FAIL";
    if (!writeFiles) return r;

    // ---- files: staff views, WAV, manifest (never removes anything) ----
    namespace fs = std::filesystem;
    std::error_code ec;
    const fs::path staffDir = fs::path(outputDir) / "procedural_v1";
    fs::create_directories(staffDir, ec);
    r.staffDir = staffDir.lexically_normal().string();
    std::array<std::pair<float, float>, kRoleCount> range{};
    for (auto& rg : range) rg = {std::numeric_limits<float>::max(), 0.0f};
    for (const auto& e : intents) {
        auto& rg = range[static_cast<std::size_t>(e.role)];
        if (e.pitchHz > 0.0f && std::isfinite(e.pitchHz)) {
            rg.first = (std::min)(rg.first, e.pitchHz);
            rg.second = (std::max)(rg.second, e.pitchHz);
        }
    }
    std::map<std::pair<std::uint64_t, int>, std::uint64_t> offs;
    for (const auto& o : noteOffs) offs[{o.eventId, static_cast<int>(o.role)}] = o.sampleFrame;
    for (const auto& p : phrases) {
        StaffViewInput in;
        in.startFrame = p.startFrame;
        in.endFrame = p.endFrame;
        in.sampleRate = kFs;
        in.pixelsPerSecond = kStaffPixelsPerSecond;
        in.pitchRange = range;
        for (std::uint32_t b = 0; b <= p.barCount && clock; ++b) in.barFrames.push_back(clock->frameAtBar(p.startBar + b));
        for (const auto& e : intents) {
            const auto it = offs.find({e.eventId, static_cast<int>(e.role)});
            const std::uint64_t end = (it != offs.end()) ? it->second : e.sampleFrame + e.durationFrames;
            in.planned.push_back({static_cast<std::size_t>(e.role), e.sampleFrame, end, e.pitchHz});
        }
        for (const auto& a : applied) {
            if (a.id.type == audio::AudioEventType::NoteOn && a.disposition == EventDisposition::Applied) {
                in.appliedNoteOns.emplace_back(static_cast<std::size_t>(a.id.role), a.id.sampleFrame);
            }
        }
        in.detected = detections;
        const StaffViewImage img = renderStaffView(in);
        std::ostringstream name;
        name << "staff_p" << std::setw(2) << std::setfill('0') << p.index << ".png";
        std::ofstream out(staffDir / name.str(), std::ios::binary);
        out.write(reinterpret_cast<const char*>(img.png.data()), static_cast<std::streamsize>(img.png.size()));
        if (!out.good() || img.png.empty()) {
            r.failures.push_back("cannot write staff view " + name.str());
        } else {
            r.staffViews.push_back(name.str());
        }
    }
    r.wavFilePath = (staffDir / "procedural_v1.wav").lexically_normal().string();
    r.wavWriteSuccess = audio::analysis::writeWavStereo16(r.wavFilePath, stereo.data(), stereo.size() / 2, kFs);
    if (!r.wavWriteSuccess) r.failures.emplace_back("cannot write the WAV");
    r.status = r.failures.empty() ? "PASS" : "FAIL";
    r.manifestFilePath = (fs::path(outputDir) / "procedural_v1_manifest.json").lexically_normal().string();
    std::ofstream mf(r.manifestFilePath);
    r.manifestWriteSuccess = mf.is_open();
    if (r.manifestWriteSuccess) {
        mf << r.toJson();
        r.manifestWriteSuccess = mf.good();
    }
    return r;
}

MeasureStatus loudnessMeasureStatus(const std::string& analysisStatus, const std::optional<double>& integrated,
                                    const std::optional<double>& maxMomentary, const std::optional<double>& maxShortTerm) {
    if (analysisStatus == "NOT_RUN") return {"NOT_RUN", ""};
    if (analysisStatus != "OK") return {"INCONCLUSIVE", "loudness analysis status " + analysisStatus};
    if (!integrated) return {"INCONCLUSIVE", "integrated loudness undefined (nothing above the gates)"};
    if (!std::isfinite(*integrated)) return {"INCONCLUSIVE", "integrated loudness not finite"};
    if ((maxMomentary && !std::isfinite(*maxMomentary)) || (maxShortTerm && !std::isfinite(*maxShortTerm))) {
        return {"INCONCLUSIVE", "momentary or short-term maximum not finite"};
    }
    return {"MEASURED", ""};
}

namespace {

MeasureStatus truePeakStatus(bool computed, double value, const char* notComputedReason) {
    if (!computed) return {"INCONCLUSIVE", notComputedReason};
    if (!std::isfinite(value)) return {"INCONCLUSIVE", "true peak not finite"};
    return {"MEASURED", ""};
}

} // namespace

ProceduralMeasureStatuses proceduralMeasureStatuses(const ProceduralResult& r) {
    ProceduralMeasureStatuses m;
    m.truePeak = r.rendered ? truePeakStatus(r.truePeakMeasured && r.renderedFrames > 0, r.truePeakDbtp,
                                             "true peak not computed (no valid rendered frames)")
                            : MeasureStatus{"NOT_RUN", ""};
    m.loudness = loudnessMeasureStatus(r.loudnessStatus, r.integratedLufs, r.maxMomentaryLufs, r.maxShortTermLufs);
    for (const auto& s : r.sections) {
        m.sections.push_back(loudnessMeasureStatus(s.loudnessStatus, s.integratedLufs, s.maxMomentaryLufs, s.maxShortTermLufs));
        if (s.loudnessStatus == "NOT_RUN" && !s.truePeakDbtp) {
            m.sectionTruePeaks.push_back({"NOT_RUN", ""});
        } else {
            m.sectionTruePeaks.push_back(truePeakStatus(s.truePeakDbtp.has_value(), s.truePeakDbtp.value_or(0.0),
                                                        "section true peak not computed (invalid range)"));
        }
    }
    // Onsets: every role MEASURED or NO_REFERENCES (no evidence, not an error, DIF-5c-R2-01)
    std::size_t onsetRun = 0;
    std::string onsetReason;
    for (std::size_t i = 0; i < kRoleCount; ++i) {
        const std::string& st = r.onsets[i].status;
        if (st != "NOT_RUN") ++onsetRun;
        if (st != "MEASURED" && st != "NO_REFERENCES") onsetReason += std::string(kRoleNames[i]) + " " + st + "; ";
        const auto& o = r.onsets[i];
        auto finite = [](const std::optional<double>& v) { return v && std::isfinite(*v); };
        if (st == "MEASURED" && !(finite(o.precision50) && finite(o.recall50))) {
            onsetReason += std::string(kRoleNames[i]) + " MEASURED without finite precision and recall; ";
        }
    }
    if (onsetRun == 0) m.onsets = {"NOT_RUN", ""};
    else if (!onsetReason.empty()) m.onsets = {"INCONCLUSIVE", onsetReason};
    else m.onsets = {"MEASURED", ""};
    // Presence: every phrase and role with an available signal level
    if (r.phrasePresence.empty()) {
        m.presence = {"NOT_RUN", ""};
    } else {
        std::string reason;
        for (const auto& w : r.phrasePresence) {
            for (std::size_t i = 0; i < kRoleCount; ++i) {
                if (w.roles[i].signal == SignalLevel::Unavailable) reason += w.id + " " + kRoleNames[i] + " unavailable; ";
            }
        }
        m.presence = reason.empty() ? MeasureStatus{"MEASURED", ""} : MeasureStatus{"INCONCLUSIVE", reason};
    }
    std::vector<const MeasureStatus*> all{&m.truePeak, &m.loudness, &m.onsets, &m.presence};
    for (const auto& s : m.sections) all.push_back(&s);
    for (const auto& s : m.sectionTruePeaks) all.push_back(&s);
    const bool noneRun = std::all_of(all.begin(), all.end(), [](const MeasureStatus* s) { return s->status == "NOT_RUN"; });
    const bool allMeasured = std::all_of(all.begin(), all.end(), [](const MeasureStatus* s) { return s->status == "MEASURED"; });
    if (m.sections.empty() && !noneRun) {
        m.overall = {"INCONCLUSIVE", "no section measured"};
    } else {
        m.overall = noneRun ? MeasureStatus{"NOT_RUN", ""} : allMeasured ? MeasureStatus{"MEASURED", ""}
                                                                        : MeasureStatus{"INCONCLUSIVE", "see the measure statuses"};
    }
    return m;
}

namespace {

// The value only when its measure is MEASURED (DIF-5e.1-R2-01)
json measured(const MeasureStatus& s, const std::optional<double>& v) {
    return (s.status == "MEASURED" && v && std::isfinite(*v)) ? json(*v) : json(nullptr);
}

json statusJson(const MeasureStatus& s) {
    json j{{"status", s.status}};
    if (!s.reason.empty()) j["reason"] = s.reason;
    return j;
}

// Binding evidence of the render, copied from fixture_probe_v1 without re-running it (DIF-5e.1-03)
json bindingEvidence(const audio::FixtureProbeResult& f) {
    auto check = [](bool v) { return json(v ? "PASS" : "FAIL"); };
    auto golden = [&](bool v) { return f.goldenApplicable ? check(v) : json("NOT_APPLICABLE"); };
    json acks = json::array();
    for (const auto& a : f.transportAcks) {
        acks.push_back({{"command_id", a.commandId}, {"status", static_cast<int>(a.status)}, {"sample_frame", a.sampleFrame},
                        {"transport_epoch", a.transportEpoch}});
    }
    return {
        {"source", "fixture_probe_v1 (same render, not re-run)"},
        {"run_seed", f.runSeed},
        {"fixture_probe_v1_verdict", f.passed ? "PASS" : "FAIL"},
        {"checks",
         {{"capture_complete", check(f.captureCompletePassed)},
          {"determinism", check(f.determinismPassed)},
          {"golden_decision_trace", golden(f.goldenTracePassed)},
          {"golden_canonical_plan", golden(f.goldenPlanPassed)},
          {"golden_pcm", f.goldenPcmStatus},
          {"note_off_pairing", check(f.noteOffPairingPassed)},
          {"transport_acks", check(f.tempoAckPassed)},
          {"scheduling_accounting", check(f.schedulingAccountingPassed)},
          {"non_silent_sections", check(f.nonSilentAudioPassed)},
          {"role_activity", check(f.roleActivityPassed)},
          {"silence_after_stop", check(f.silenceBoundaryPassed)},
          {"sample_ceiling", check(f.sampleCeilingPassed)},
          {"zero_clipping", check(f.clippedSamples == 0)},
          {"dsp_anomalies", check(f.dspAnomaliesPassed)},
          {"safe_stop", check(f.safeStopPassed)},
          {"event_chain", check(f.eventChainPassed)},
          {"coordinator", check(!f.coordinatorFailed)},
          {"true_peak_engineering_check", {{"result", check(f.truePeakPassed)}, {"binding_in_procedural_v1", false}}}}},
        {"hashes",
         {{"decision_trace", f.decisionTraceHash}, {"golden_decision_trace", audio::kFixtureProbeV1GoldenTraceHash},
          {"canonical_plan", f.canonicalPlanHash}, {"golden_canonical_plan", audio::kFixtureProbeV1GoldenPlanHash},
          {"jit_plan", f.jitPlanHash}, {"jit_golden_status", f.jitGoldenStatus}, {"jit_offline_plan_match", f.jitOfflinePlanMatch},
          {"pcm_content", f.pcmContentHash}, {"golden_pcm_content", audio::kFixtureProbeV1GoldenPcmHash},
          {"content_fingerprint", f.contentFingerprint}, {"content_fingerprint_jit_match", f.contentFingerprintJitMatch}}},
        {"event_chain",
         {{"path", f.coordinatorFailed ? "injected_fault" : "nominal"}, {"expected", f.chainExpectedEvents}, {"sent", f.chainSentEvents},
          {"canceled", f.chainCanceledEvents}, {"consumed", f.chainConsumedEvents}, {"applied", f.chainAppliedEvents},
          {"ignored", f.chainIgnoredEvents}, {"dropped", f.chainDroppedEvents}, {"spsc_drained", f.chainSpscDrained},
          {"mismatches", f.chainMismatches}, {"first_mismatch", f.chainFirstMismatch}, {"passed", f.eventChainPassed}}},
        {"scheduling",
         {{"phrases_planned", f.phrasesPlanned}, {"generated_events", f.generatedEvents}, {"planner_note_offs", f.plannerNoteOffs},
          {"engine_processed_events", f.engineProcessedEvents}, {"expected_engine_events", f.expectedEngineEvents},
          {"dropped_scheduled_events", f.droppedScheduledEvents}, {"deferred_scheduled_events", f.deferredScheduledEvents},
          {"scheduler_backpressure_drops", f.schedulerBackpressureDrops}, {"orphan_note_recovered", f.orphanNoteRecoveredCount},
          {"voice_drop_count", f.voiceDropCount}}},
        {"transport_acks", acks},
        {"signal",
         {{"captured_frames", f.totalCapturedFrames}, {"requested_frames", f.requestedSampleFrames}, {"dropped_frames", f.droppedFrames},
          {"peak_amplitude", f.peakAmplitude}, {"clipped_samples", f.clippedSamples}, {"max_gain_reduction_db", f.maxGainReductionDb}}},
        {"failure_reason", f.failureReason}};
}

} // namespace

std::string ProceduralResult::toJson() const {
    const ProceduralMeasureStatuses ms = proceduralMeasureStatuses(*this);
    json j;
    j["manifest_version"] = "1.0";
    j["scenario"] = "procedural_v1";
    j["build_id"] = CYMATICA_BUILD_ID;
    j["verdict"] = status;
    j["verdict_scope"] = "binding checks only: O3 chain, determinism and goldens, PCM integrity and approved limiter "
                         "invariants, capture validity, exact bus-mix reconstruction";
    j["evidence_kind"] = "technical, single fixture (fixture-m2-4-v1, seed 1337): no perceptual claim, no generalisation";
    j["failures"] = failures;
    j["findings"] = findings;
    j["fixture_probe_v1_passed"] = fixtureProbePassed;
    j["binding_evidence"] = fixture ? bindingEvidence(*fixture) : json{{"status", "NOT_RUN"}};
    j["pcm_content_hash"] = pcmContentHash;
    j["canonical_plan_hash"] = canonicalPlanHash;
    j["content_fingerprint"] = contentFingerprint;
    j["rendered_frames"] = renderedFrames;
    j["content_end_frame"] = contentEndFrame;
    j["capture"] = {{"valid", captureInvalidReason(capture).empty()},
                    {"stem_frames", capture.stemFrames},
                    {"stem_overflow_frames", capture.stemOverflowFrames},
                    {"applied_log_overflow", capture.appliedOverflow},
                    {"reconstruction_status", reconstructionStatus},
                    {"reconstruction_mismatched_samples", reconstructionMismatches}};
    json sec = json::array();
    for (std::size_t i = 0; i < sections.size(); ++i) {
        const auto& s = sections[i];
        sec.push_back({{"section", s.section}, {"start_frame", s.startFrame}, {"end_frame", s.endFrame},
                       {"status", statusJson(ms.sections[i])}, {"loudness_analysis_status", s.loudnessStatus},
                       {"integrated_lufs", measured(ms.sections[i], s.integratedLufs)},
                       {"max_momentary_lufs", measured(ms.sections[i], s.maxMomentaryLufs)},
                       {"max_short_term_lufs", measured(ms.sections[i], s.maxShortTermLufs)},
                       {"true_peak", {{"status", statusJson(ms.sectionTruePeaks[i])},
                                      {"dbtp", measured(ms.sectionTruePeaks[i], s.truePeakDbtp)}}}});
    }
    const bool tpOk = ms.truePeak.status == "MEASURED";
    j["measures"] = {
        {"status", statusJson(ms.overall)},
        {"true_peak", {{"status", statusJson(ms.truePeak)}, {"dbtp", tpOk ? num(truePeakDbtp) : json(nullptr)}, {"trial_value_dbtp", kTrueTrialDbtp},
                       {"above_trial", tpOk ? json(truePeakDbtp > kTrueTrialDbtp) : json(nullptr)},
                       {"gate", false}, {"max_gain_reduction_db", tpOk ? num(maxGainReductionDb) : json(nullptr)}}},
        {"loudness", {{"standard", "ITU-R BS.1770-5, EBU Tech 3341"}, {"status", statusJson(ms.loudness)},
                      {"analysis_status", loudnessStatus},
                      {"integrated_lufs", measured(ms.loudness, integratedLufs)},
                      {"max_momentary_lufs", measured(ms.loudness, maxMomentaryLufs)},
                      {"max_short_term_lufs", measured(ms.loudness, maxShortTermLufs)}, {"trial_target_lufs", -23.0}, {"trial_tolerance_lu", 2.0},
                      {"gate", false}, {"note", "about 183 s: no 30-minute rolling measure (ADR-0004 §5)"}}},
        {"sections", sec}};
    json on = json::object();
    for (std::size_t r = 0; r < kRoleCount; ++r) {
        const auto& s = onsets[r];
        on[kRoleNames[r]] = {{"status", s.status}, {"references", s.references}, {"detections", s.detections},
                             {"matched_50ms", s.status == "MEASURED" ? json(s.matched) : json(nullptr)},
                             {"release_detections", s.status == "MEASURED" ? json(s.releaseDetections) : json(nullptr)},
                             {"precision_50", s.status == "MEASURED" ? optNum(s.precision50) : json(nullptr)},
                             {"recall_50", s.status == "MEASURED" ? optNum(s.recall50) : json(nullptr)},
                             {"precision_excl_releases_50", s.status == "MEASURED" ? optNum(s.precisionExclReleases) : json(nullptr)}};
    }
    j["onsets"] = {{"status", statusJson(ms.onsets)}, {"profile", "stem_onset_profile_v1"}, {"profile_sha256", onsetProfileSha256}, {"tolerance_ms", 50},
                   {"roles", on}, {"note", "MEASURED; AC-AO-02 thresholds proposed in 5c, ratified at M2.5"}};
    json pres = json::array();
    for (const auto& w : phrasePresence) {
        json roles = json::object();
        for (std::size_t r = 0; r < kRoleCount; ++r) {
            const auto& rt = w.roles[r];
            roles[kRoleNames[r]] = {{"planned", rt.planned}, {"signal", signalLevelName(rt.signal)}, {"rms_dbfs", num(rt.rmsDbfs)},
                                    {"stem_to_mix_db", num(rt.stemToMixDb)}};
        }
        pres.push_back({{"phrase", w.phrase}, {"section", w.section}, {"start_bar", w.startBar}, {"roles", roles}});
    }
    j["presence_per_phrase"] = {{"status", statusJson(ms.presence)}, {"profile", "presence_profile_v1"}, {"validated", false}, {"phrases", pres}};
    j["diversity"] = {{"status", "NOT_RUN"}, {"phase", "5e.2 (ADR-0006 5e-04)"}};
    j["staff_view"] = {{"dir", staffDir}, {"files", staffViews}, {"pixels_per_second", kStaffPixelsPerSecond},
                       {"legend", {{"lanes_top_to_bottom", json::array({"pulso", "corpo", "trama", "vettore"})},
                                   {"top_strip_white", "applied NoteOn"},
                                   {"middle_bars", "planned notes, height = log pitch within the role range of the render"},
                                   {"bottom_strip_green", "detected onset matched to an applied NoteOn (+-50 ms)"},
                                   {"bottom_strip_red", "detected onset unmatched"},
                                   {"bottom_strip_orange", "unmatched, within 50 ms of an applied NoteOff (release)"},
                                   {"grey_vertical_lines", "bar lines"},
                                   {"role_colours", {{"pulso", "red"}, {"corpo", "blue"}, {"trama", "purple"}, {"vettore", "yellow"}}}}},
                       {"diagnostic_only", true}};
    j["wav_file"] = wavFilePath;
    j["wav_write_success"] = wavWriteSuccess;
    return j.dump(2) + "\n";
}

} // namespace cymatica::cacp
