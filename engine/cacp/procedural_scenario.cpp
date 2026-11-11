#include "procedural_scenario.h"

#include "audio_probe.h"
#include "diversity.h"
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

namespace {

// BS.1770 loudness of frames [start, end) of an interleaved stereo signal; INVALID_RANGE when outside it
SectionMeasures measureRange(std::span<const float> pcm, std::uint32_t section, std::uint64_t start, std::uint64_t end,
                             bool withTruePeak) {
    SectionMeasures sm;
    sm.section = section;
    sm.startFrame = start;
    sm.endFrame = end;
    if (2 * end <= pcm.size() && end > start) {
        const std::span<const float> slice(pcm.data() + 2 * start, 2 * (end - start));
        const LoudnessResult l = measureLoudnessStereo(slice, kFs);
        sm.loudnessStatus = l.supported ? analysisStatusName(l.status) : "UNSUPPORTED";
        sm.integratedLufs = l.integratedLufs;
        sm.maxMomentaryLufs = l.maxMomentaryLufs;
        sm.maxShortTermLufs = l.maxShortTermLufs;
        if (withTruePeak) sm.truePeakDbtp = audio::analysis::estimateTruePeakDbtp(slice.data(), end - start, 2);
    } else {
        sm.loudnessStatus = "INVALID_RANGE";
    }
    return sm;
}

} // namespace

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
        const auto next = std::next(it);
        const std::uint64_t end = (next != sectionStart.end()) ? next->second : f.contentEndFrame;
        r.sections.push_back(measureRange(stereo, it->first, it->second, end, true));
    }

    // ---- measures: loudness per stem and of the bus mix, same ranges as the mix (M2.5.1c) ----
    const auto stemMeasures = [&](std::span<const float> pcm) {
        StemLoudness s;
        s.whole = measureRange(pcm, 0, 0, pcm.size() / 2, false);
        for (const auto& sec : r.sections) {
            s.sections.push_back(measureRange(pcm, sec.section, sec.startFrame, sec.endFrame, false));
        }
        return s;
    };
    r.busLoudness = stemMeasures(cap.busMix());
    for (std::size_t role = 0; role < kRoleCount; ++role) r.stemLoudness[role] = stemMeasures(cap.stem(role));

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
            const auto d = classifyRoleOnsets(role, cap.stem(role), frozen->roles[role], refs[role], releases[role], kFs, kOnsetToleranceS);
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

    // ---- measures: musical groups of the phrases and seed diversity (5e-04; offline planning only) ----
    if (const FixturePlan plan = planFixtureM24V1(1337); plan.ok) r.phraseGroups = phraseGroupIds(plan);
    r.diversityJson = diversityJson(measureSeedDiversity());

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
        // Grid and caption (P-M25-08): beats and steps of the section meter, spread evenly over each bar; the
        // positions are only drawn, so the integer division of the bar length is exact enough for a pixel
        const audio::FormPlan form = audio::MusicPlanner::makeFixtureM24V1Form();
        const audio::TempoMap& tm = form.sections[(std::min)(p.section, static_cast<std::uint32_t>(form.sectionCount - 1))].tempoMap;
        const std::uint64_t steps = static_cast<std::uint64_t>(tm.beatsPerBar) * tm.subdivisionsPerBeat;
        for (std::size_t b = 0; b + 1 < in.barFrames.size() && steps > 0; ++b) {
            const std::uint64_t f0 = in.barFrames[b];
            const std::uint64_t len = in.barFrames[b + 1] - f0;
            for (std::uint64_t s = 1; s < steps; ++s) {
                const std::uint64_t at = f0 + len * s / steps;
                (s % tm.subdivisionsPerBeat == 0 ? in.beatFrames : in.stepFrames).push_back(at);
            }
        }
        in.firstBarNumber = p.startBar + 1;
        static constexpr std::array<const char*, 3> kSectionNames{"A", "B", "A'"};
        std::ostringstream caption;
        caption << "PHRASE " << p.index << "  " << (p.section < kSectionNames.size() ? kSectionNames[p.section] : "?") << "  "
                << tm.bpm.numerator / (tm.bpm.denominator == 0 ? 1 : tm.bpm.denominator) << " BPM " << tm.timeSignatureNum
                << "/" << tm.timeSignatureDen;
        in.caption = caption.str();
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

MeasureStatus roleOnsetMeasureStatus(const RoleOnsetSummary& s) {
    if (s.status == "NOT_RUN" || s.status == "NO_REFERENCES") return {s.status, ""};
    if (s.status != "MEASURED") return {"INCONCLUSIVE", "onset analysis status " + s.status};
    auto finite = [](const std::optional<double>& v) { return v && std::isfinite(*v); };
    if (!finite(s.precision50) || !finite(s.recall50)) return {"INCONCLUSIVE", "precision or recall missing or not finite"};
    return {"MEASURED", ""};
}

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
    // Onsets: every role MEASURED or NO_REFERENCES (no evidence, not an error, DIF-5c-R2-01), from the same
    // effective per-role status that the role's own serialisation uses (DIF-5e.1-R3-01)
    std::size_t onsetRun = 0;
    std::string onsetReason;
    for (std::size_t i = 0; i < kRoleCount; ++i) {
        const MeasureStatus rs = roleOnsetMeasureStatus(r.onsets[i]);
        if (rs.status != "NOT_RUN") ++onsetRun;
        if (rs.status != "MEASURED" && rs.status != "NO_REFERENCES") {
            onsetReason += std::string(kRoleNames[i]) + " " + rs.status + (rs.reason.empty() ? "" : " (" + rs.reason + ")") + "; ";
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
    if (r.diversityJson.empty()) {
        m.diversity = {"NOT_RUN", ""};
    } else {
        const json d = json::parse(r.diversityJson, nullptr, false);
        const std::string st = (d.is_object() && d.contains("status") && d["status"].is_string()) ? d["status"].get<std::string>() : "";
        m.diversity = st == "MEASURED" ? MeasureStatus{"MEASURED", ""}
                                       : MeasureStatus{"INCONCLUSIVE", "diversity status " + (st.empty() ? std::string("unreadable") : st)};
    }
    std::vector<const MeasureStatus*> all{&m.truePeak, &m.loudness, &m.onsets, &m.presence};
    if (m.diversity.status != "NOT_RUN") all.push_back(&m.diversity);
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

json loudnessRangeJson(const SectionMeasures& s, const MeasureStatus& st) {
    return {{"status", statusJson(st)}, {"analysis_status", s.loudnessStatus}, {"start_frame", s.startFrame},
            {"end_frame", s.endFrame}, {"integrated_lufs", measured(st, s.integratedLufs)},
            {"max_momentary_lufs", measured(st, s.maxMomentaryLufs)},
            {"max_short_term_lufs", measured(st, s.maxShortTermLufs)}};
}

MeasureStatus rangeStatus(const SectionMeasures& s) {
    return loudnessMeasureStatus(s.loudnessStatus, s.integratedLufs, s.maxMomentaryLufs, s.maxShortTermLufs);
}

// Stem minus bus mix, in LU, only when both integrated values are MEASURED
json relativeLu(const SectionMeasures& stem, const SectionMeasures& bus) {
    if (rangeStatus(stem).status != "MEASURED" || rangeStatus(bus).status != "MEASURED") return json(nullptr);
    return num(*stem.integratedLufs - *bus.integratedLufs);
}

bool sameRange(const SectionMeasures& a, const SectionMeasures& b) {
    return a.section == b.section && a.startFrame == b.startFrame && a.endFrame == b.endFrame;
}

std::string rangeLabel(const SectionMeasures& s) {
    return "section " + std::to_string(s.section) + " [" + std::to_string(s.startFrame) + ", " + std::to_string(s.endFrame) + ")";
}

// Loudness per stem (M2.5.1c): MEASURED only when every range of the bus mix and of every stem is MEASURED and
// covers exactly the ranges of its reference (the bus mix for a stem, the mix sections for the bus, DIF-M2.5.1c-02);
// NOT_RUN when nothing ran; otherwise INCONCLUSIVE with the first gap. The bus whole range must be exactly
// [0, renderedFrames) and at least one section must exist (DIF-M2.5.1c-R3-01). A relative level is published only
// for two measured, identical ranges.
json stemLoudnessJson(std::uint64_t renderedFrames, const std::vector<SectionMeasures>& mixSections,
                      const StemLoudness& bus, const std::array<StemLoudness, kRoleCount>& stems) {
    std::string firstGap;
    bool anyRun = false;
    const auto gap = [&](const std::string& reason) {
        if (firstGap.empty()) firstGap = reason;
    };
    const auto note = [&](const std::string& label, const SectionMeasures& s) {
        const MeasureStatus st = rangeStatus(s);
        if (st.status != "NOT_RUN") anyRun = true;
        if (st.status != "MEASURED") gap(label + ": " + st.status);
        return st;
    };
    const auto relative = [](const SectionMeasures& s, const SectionMeasures* ref) {
        return (ref != nullptr && sameRange(s, *ref)) ? relativeLu(s, *ref) : json(nullptr);
    };
    // expected: ranges the signal must cover, in order; reference: ranges a relative level is computed against
    const auto signalJson = [&](const std::string& name, const StemLoudness& s, const std::vector<SectionMeasures>& expected,
                                const StemLoudness* reference) {
        if (s.sections.size() != expected.size()) {
            gap(name + ": " + std::to_string(s.sections.size()) + " sections, " + std::to_string(expected.size()) + " expected");
        }
        json sec = json::array();
        for (std::size_t i = 0; i < s.sections.size(); ++i) {
            const SectionMeasures& x = s.sections[i];
            const bool aligned = i < expected.size() && sameRange(x, expected[i]);
            if (!aligned) gap(name + " " + rangeLabel(x) + " not aligned with the expected ranges");
            json e = loudnessRangeJson(x, note(name + " section " + std::to_string(x.section), x));
            e["section"] = x.section;
            e["aligned"] = aligned;
            if (reference != nullptr) {
                const SectionMeasures* ref = i < reference->sections.size() ? &reference->sections[i] : nullptr;
                e["relative_to_bus_mix_lu"] = aligned ? relative(x, ref) : json(nullptr);
            }
            sec.push_back(e);
        }
        if (reference != nullptr && !sameRange(s.whole, reference->whole)) {
            gap(name + " whole " + rangeLabel(s.whole) + " differs from the bus mix " + rangeLabel(reference->whole));
        }
        json out = {{"whole", loudnessRangeJson(s.whole, note(name + " whole", s.whole))}, {"sections", sec}};
        if (reference != nullptr) out["relative_to_bus_mix_lu"] = relative(s.whole, &reference->whole);
        return out;
    };
    json out;
    out["standard"] = "ITU-R BS.1770-5, EBU Tech 3341";
    out["scope"] = "role stems and their bus sum before the master stage, same ranges as the mix; K-weighted and gated, "
                   "so a stem silent in part of a range is measured on its active part";
    if (bus.whole.startFrame != 0 || bus.whole.endFrame != renderedFrames) {
        gap("bus mix whole " + rangeLabel(bus.whole) + " is not the render [0, " + std::to_string(renderedFrames) + ")");
    }
    if (mixSections.empty() || bus.sections.empty()) gap("no section to measure");
    out["bus_mix"] = signalJson("bus mix", bus, mixSections, nullptr);
    json roles = json::object();
    for (std::size_t i = 0; i < kRoleCount; ++i) roles[kRoleNames[i]] = signalJson(kRoleNames[i], stems[i], bus.sections, &bus);
    out["roles"] = roles;
    out["gate"] = false;
    out["status"] = statusJson(!anyRun ? MeasureStatus{"NOT_RUN", ""}
                               : firstGap.empty() ? MeasureStatus{"MEASURED", ""}
                                                  : MeasureStatus{"INCONCLUSIVE", firstGap});
    return out;
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
    j["measures"]["loudness_per_stem"] = stemLoudnessJson(renderedFrames, sections, busLoudness, stemLoudness);
    json on = json::object();
    for (std::size_t r = 0; r < kRoleCount; ++r) {
        const auto& s = onsets[r];
        const MeasureStatus rs = roleOnsetMeasureStatus(s);
        const bool ok = rs.status == "MEASURED";
        json role = {{"status", rs.status}, {"analysis_status", s.status}, {"references", s.references}, {"detections", s.detections},
                     {"matched_50ms", ok ? json(s.matched) : json(nullptr)},
                     {"release_detections", ok ? json(s.releaseDetections) : json(nullptr)},
                     {"precision_50", ok ? optNum(s.precision50) : json(nullptr)},
                     {"recall_50", ok ? optNum(s.recall50) : json(nullptr)},
                     {"precision_excl_releases_50", ok ? optNum(s.precisionExclReleases) : json(nullptr)}};
        if (!rs.reason.empty()) role["reason"] = rs.reason;
        on[kRoleNames[r]] = role;
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
    j["diversity"] = diversityJson.empty() ? json{{"status", "NOT_RUN"}} : json::parse(diversityJson);
    j["phrase_groups"] = {{"rule", "per-phrase content fingerprint, frames relative to the phrase start (DIF-5e.0-02)"},
                          {"groups", phraseGroups}};
    j["staff_view"] = {{"dir", staffDir}, {"files", staffViews}, {"pixels_per_second", kStaffPixelsPerSecond},
                       {"legend", {{"lanes_top_to_bottom", json::array({"pulso", "corpo", "trama", "vettore"})},
                                   {"lane_labels", json::array({kStaffInstrumentNames[0], kStaffInstrumentNames[1],
                                                                kStaffInstrumentNames[2], kStaffInstrumentNames[3]})},
                                   {"header", "phrase, section, tempo and meter; bar numbers from 1"},
                                   {"footer", "colour legend"},
                                   {"top_strip_white", "applied NoteOn"},
                                   {"middle_bars", "planned notes, height = log pitch within the role range of the render"},
                                   {"bottom_strip_green", "detected onset matched to an applied NoteOn (+-50 ms)"},
                                   {"bottom_strip_red", "detected onset unmatched"},
                                   {"bottom_strip_orange", "unmatched, within 50 ms of an applied NoteOff (release)"},
                                   {"grey_vertical_lines", "bars (light), beats (medium), grid steps (dark): quarters and sixteenths in 4/4"},
                                   {"role_colours", {{"pulso", "red"}, {"corpo", "blue"}, {"trama", "purple"}, {"vettore", "yellow"}}}}},
                       {"diagnostic_only", true}};
    j["wav_file"] = wavFilePath;
    j["wav_write_success"] = wavWriteSuccess;
    return j.dump(2) + "\n";
}

} // namespace cymatica::cacp
