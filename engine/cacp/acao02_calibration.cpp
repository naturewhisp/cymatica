#include "acao02_calibration.h"

#include "audio_engine.h"
#include "audio_probe.h"
#include "audio_scheduler.h"
#include "music_planner.h"
#include "plan_content_fingerprint.h"
#include "sha256.h"
#include "stft.h"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <map>
#include <sstream>

namespace cymatica::cacp {

namespace {

constexpr std::uint32_t kFs = 48000;
constexpr std::size_t kFixtureCaptureFrames = 8'900'000;      // > 8,825,200 frames rendered by fixture_probe_v1
constexpr std::uint64_t kCalibrationSeed = 1337;
constexpr std::uint64_t kSeedScanFirst = 1;
constexpr std::uint64_t kSeedScanLast = 32;
constexpr std::size_t kMaxVerificationSeeds = 3;

std::string num(double v, int precision = 10) {
    if (!std::isfinite(v)) return "null";
    std::ostringstream s;
    s << std::setprecision(precision) << v;
    return s.str();
}

std::string jsonEscape(const std::string& in) {
    std::string out;
    for (const char c : in) {
        if (c == '"' || c == '\\') {
            out += '\\';
            out += c;
        } else if (static_cast<unsigned char>(c) < 0x20) {
            out += ' ';
        } else {
            out += c;
        }
    }
    return out;
}

struct SynthNote {
    double timeS;
    float pitchHz;
    float velocity;
    double durationS;
    float timbre;
};

// Renders isolated notes of one role through the real engine (offline), capturing stems and applied events.
// Returns the validity of the capture: every NoteOn Applied, stems covering the rendered frames, no overflow.
CaptureCheck renderSyntheticRole(audio::MusicRole role, const std::vector<SynthNote>& notes, StemCapture& cap,
                                 std::vector<AppliedEventRecord>& applied) {
    CaptureCheck check;
    cap.reset();
    applied.clear();
    audio::AudioEngine engine;
    if (!engine.initOffline({kFs, 2, 220.0f})) {
        check.probeFailure = "offline engine initialisation failed";
        return check;
    }
    AppliedEventLog log(audio::kEventChainLogCapacity);
    (void)engine.setAppliedEventLog(&log);
    (void)engine.setRoleStemSink(cap.sink());
    audio::AudioScheduler scheduler({.lookaheadFrames = 7200, .sampleRate = kFs});
    engine.sendCommand(audio::AudioCommand{.commandId = 1, .type = audio::AudioCommandType::Start, .targetSampleFrame = 0});
    std::uint64_t id = 1;
    double endS = 0.0;
    for (const auto& n : notes) {
        const audio::MusicIntentEvent e{.eventId = id++,
                                        .sampleFrame = static_cast<std::uint64_t>(std::llround(n.timeS * kFs)),
                                        .tick = 0,
                                        .transportEpoch = 1,
                                        .role = role,
                                        .pitchHz = n.pitchHz,
                                        .velocity = n.velocity,
                                        .durationFrames = static_cast<std::uint32_t>(std::llround(n.durationS * kFs)),
                                        .timbreParam = n.timbre};
        scheduler.scheduleIntent(e, 1);
        endS = (std::max)(endS, n.timeS + n.durationS);
    }
    constexpr std::uint32_t kTick = 400;
    const auto ticks = static_cast<std::uint64_t>(std::ceil((endS + 1.0) * kFs / kTick));
    std::vector<float> block(kTick * 2);
    for (std::uint64_t t = 0; t < ticks; ++t) {
        (void)scheduler.dispatchDueEvents(t * kTick, engine);
        engine.processBlock(block.data(), kTick);
        audio::AudioCommandAck ack{};
        while (engine.pollAck(ack)) {}
    }
    (void)engine.setRoleStemSink(nullptr);
    (void)engine.setAppliedEventLog(nullptr);
    applied.assign(log.records().begin(), log.records().end());
    engine.shutdown();

    std::size_t appliedNoteOns = 0;
    for (const auto& rec : applied) {
        if (rec.id.type == audio::AudioEventType::NoteOn && rec.id.role == role &&
            rec.disposition == audio::EventDisposition::Applied) {
            ++appliedNoteOns;
        }
    }
    check.rendered = true;
    check.probePassed = (appliedNoteOns == notes.size());
    if (!check.probePassed) {
        check.probeFailure = std::to_string(appliedNoteOns) + " of " + std::to_string(notes.size()) + " NoteOn applied";
    }
    check.expectedFrames = static_cast<std::size_t>(ticks * kTick);
    check.stemFrames = cap.frames();
    check.stemOverflowFrames = cap.overflowFrames();
    check.appliedOverflow = log.overflow();
    return check;
}

// Capture check of a scenario probe render (fixture or planner): the probe verdict plus the stem coverage
CaptureCheck probeCaptureCheck(bool passed, const std::string& failure, std::uint64_t expectedFrames,
                               const StemCapture& cap, std::uint64_t appliedOverflow) {
    CaptureCheck check;
    check.rendered = true;
    check.probePassed = passed;
    check.probeFailure = failure;
    check.expectedFrames = static_cast<std::size_t>(expectedFrames);
    check.stemFrames = cap.frames();
    check.stemOverflowFrames = cap.overflowFrames();
    check.appliedOverflow = appliedOverflow;
    return check;
}

// Set A (intrinsic latency): 24 notes per role, every 0.5 s, fixed pitch, velocity and timbre
std::vector<SynthNote> syntheticSetA(std::size_t role) {
    static constexpr std::array<float, kRoleCount> kPitch{55.0f, 220.0f, 440.0f, 660.0f};
    static constexpr std::array<double, kRoleCount> kDur{0.12, 0.40, 0.25, 0.30};
    std::vector<SynthNote> v;
    for (int i = 0; i < 24; ++i) v.push_back({0.25 + 0.5 * i, kPitch[role], 0.8f, kDur[role], 0.5f});
    return v;
}

// Set B (verification): 40 notes per role, irregular intervals, two-octave pitches, varied velocity and timbre
std::vector<SynthNote> syntheticSetB(std::size_t role) {
    static constexpr std::array<float, kRoleCount> kBase{45.0f, 130.81f, 261.63f, 392.0f};
    static constexpr std::array<double, 8> kInterval{0.18, 0.31, 0.45, 0.27, 0.62, 0.22, 0.38, 0.70};
    static constexpr std::array<int, 8> kSemitone{0, 7, 3, 12, 5, 10, 2, 15};
    static constexpr std::array<float, 6> kVel{0.5f, 0.9f, 0.7f, 1.0f, 0.6f, 0.8f};
    static constexpr std::array<float, 5> kTimbre{0.2f, 0.8f, 0.5f, 0.9f, 0.3f};
    std::vector<SynthNote> v;
    double t = 0.3;
    for (int i = 0; i < 40; ++i) {
        const double interval = kInterval[static_cast<std::size_t>(i) % kInterval.size()];
        const float pitch = kBase[role] * std::pow(2.0f, static_cast<float>(kSemitone[static_cast<std::size_t>(i) % 8]) / 12.0f);
        v.push_back({t, pitch, kVel[static_cast<std::size_t>(i) % kVel.size()], 0.6 * interval,
                     kTimbre[static_cast<std::size_t>(i) % kTimbre.size()]});
        t += interval;
    }
    return v;
}

RoleSetEvaluation toRoleSet(const RoleOnsetEvaluation& e) {
    RoleSetEvaluation r;
    r.references = e.references;
    r.detections = e.detections;
    if (e.status != AnalysisStatus::Ok) {
        r.status = analysisStatusName(e.status);
        return r;
    }
    if (e.references == 0) {
        r.status = "NO_REFERENCES";
        return r;
    }
    r.precision50 = e.at50.precision; r.recall50 = e.at50.recall; r.f50 = e.at50.fMeasure;
    r.precision25 = e.at25.precision; r.recall25 = e.at25.recall; r.f25 = e.at25.fMeasure;
    r.errMedianMs = e.errors.medianS * 1000.0;
    r.errP05Ms = e.errors.p05S * 1000.0;
    r.errP95Ms = e.errors.p95S * 1000.0;
    r.unmatched = e.unmatchedLag.pairs;
    r.releaseDetections = e.releaseDetections;
    r.precisionExclReleases = e.precisionExclReleases;
    r.unmatchedLagMedianMs = e.unmatchedLag.medianS * 1000.0;
    r.unmatchedLagP05Ms = e.unmatchedLag.p05S * 1000.0;
    r.unmatchedLagP95Ms = e.unmatchedLag.p95S * 1000.0;
    r.status = "MEASURED";
    return r;
}

std::array<RoleSetEvaluation, kRoleCount> evaluateCapture(const StemCapture& cap, const std::vector<AppliedEventRecord>& applied,
                                                          const std::array<RoleOnsetProfile, kRoleCount>& profile) {
    std::array<RoleSetEvaluation, kRoleCount> out{};
    for (std::size_t r = 0; r < kRoleCount; ++r) {
        const auto refs = roleReferenceOnsetsS(applied, static_cast<audio::MusicRole>(r), kFs);
        const auto rel = roleReleaseTimesS(applied, static_cast<audio::MusicRole>(r), kFs);
        out[r] = toRoleSet(evaluateRoleOnsets(cap.stem(r), refs, profile[r], kFs, rel));
    }
    return out;
}

audio::MusicalContentFingerprint fixturePlanFingerprint(std::uint64_t seed) {
    const auto clock = audio::MusicPlanner::makeFixtureM24V1Clock();
    if (!clock) return {};
    const auto phrases = audio::MusicPlanner::makeFixtureM24V1Phrases(*clock);
    audio::MusicPlanner planner({.runSeed = seed, .candidateCount = 8, .sampleRate = kFs});
    planner.setFormPlan(audio::MusicPlanner::makeFixtureM24V1Form());
    std::vector<std::vector<audio::MusicIntentEvent>> intents;
    for (const auto& p : phrases) {
        std::vector<audio::MusicIntentEvent> out;
        planner.planPhrase(p, *clock, 0, p.sectionIndex + 1ULL, out, nullptr);
        intents.push_back(std::move(out));
    }
    return audio::musicalContentFingerprint(intents, planner.plannedNoteOffs(), kFs);
}

} // namespace

std::string captureInvalidReason(const CaptureCheck& c) {
    std::ostringstream s;
    if (!c.rendered) s << "render not executed" << (c.probeFailure.empty() ? "" : " (" + c.probeFailure + ")") << "; ";
    if (c.rendered && !c.probePassed) s << "probe failed: " << c.probeFailure << "; ";
    if (c.rendered && (c.expectedFrames == 0 || c.stemFrames != c.expectedFrames)) {
        s << "stems cover " << c.stemFrames << " of " << c.expectedFrames << " frames; ";
    }
    if (c.stemOverflowFrames != 0) s << "stem overflow " << c.stemOverflowFrames << " frames; ";
    if (c.appliedOverflow != 0) s << "applied-event log overflow " << c.appliedOverflow << "; ";
    return s.str();
}

VerificationSet verificationSetFromCaptures(std::string name, const std::vector<CaptureCheck>& checks,
                                            const std::array<RoleSetEvaluation, kRoleCount>& roles) {
    VerificationSet vs{std::move(name), "MEASURED", "", roles, false};
    std::string reasons;
    for (const auto& c : checks) reasons += captureInvalidReason(c);
    if (checks.empty()) reasons = "no capture recorded; ";
    if (!reasons.empty()) {
        vs.status = "INCONCLUSIVE";
        vs.note = "invalid capture: " + reasons;
        vs.invalidCapture = true;
        vs.roles = {}; // no usable scores
    }
    return vs;
}

void finalizeAcao02Status(Acao02Result& r) {
    r.inconclusiveReasons.clear();
    auto reason = [&](std::string text) { r.inconclusiveReasons.push_back(std::move(text)); };

    if (r.intrinsicCaptures.size() != kRoleCount) reason("intrinsic latency: missing captures");
    for (std::size_t i = 0; i < r.intrinsicCaptures.size(); ++i) {
        const std::string why = captureInvalidReason(r.intrinsicCaptures[i]);
        if (!why.empty()) reason("intrinsic latency " + std::string(kRoleNames[i % kRoleCount]) + ": " + why);
    }
    const std::string calWhy = captureInvalidReason(r.calibrationCapture);
    if (!calWhy.empty()) reason("calibration capture (seed 1337): " + calWhy);
    if (!r.fixturePassed) reason("calibration fixture probe failed");
    if (!r.calibrationAnalysisOk) reason("calibration analysis failed: " + r.failureReason);
    if (r.reconstructionStatus != "OK" || r.reconstructionMismatches != 0) {
        reason("bus-mix reconstruction " + r.reconstructionStatus + ", " + std::to_string(r.reconstructionMismatches) +
               " mismatched samples");
    }
    if (!r.profileFrozen) {
        reason("profile not frozen: verification not run");
    } else if (!r.calibratedEqualsFrozen) {
        reason("calibrated profile " + r.calibratedSha256 + " != frozen " + r.frozenSha256 +
               ": verification measures kept as diagnostic only");
    }
    bool anyMeasuredSet = false;
    for (const auto& vs : r.verification) {
        if (vs.invalidCapture) reason("verification set " + vs.name + ": " + vs.note);
        if (vs.status == "MEASURED" && !vs.invalidCapture) anyMeasuredSet = true;
    }
    if (r.profileFrozen && !anyMeasuredSet) reason("no verification set measured on valid data");

    r.status = r.inconclusiveReasons.empty() ? "MEASURED" : "INCONCLUSIVE";
    r.verificationStatus = r.status;

    // Proposed thresholds: worst measured verification value minus 0.05, rounded down to 0.05 (D-M2-16)
    r.thresholdsStatus = (r.status == "MEASURED") ? "PROPOSED" : "NOT_PROPOSED";
    for (std::size_t role = 0; role < kRoleCount; ++role) {
        double minR = 2.0, minP = 2.0;
        if (r.status == "MEASURED") {
            for (const auto& vs : r.verification) {
                if (vs.invalidCapture || vs.status != "MEASURED" || vs.roles[role].status != "MEASURED") continue;
                minR = (std::min)(minR, vs.roles[role].recall50);
                minP = (std::min)(minP, vs.roles[role].precision50);
            }
        }
        r.proposedRecall[role] = minR > 1.5 ? std::nan("") : (std::max)(0.0, std::floor((minR - 0.05) * 20.0) / 20.0);
        r.proposedPrecision[role] = minP > 1.5 ? std::nan("") : (std::max)(0.0, std::floor((minP - 0.05) * 20.0) / 20.0);
    }
}

std::optional<FrozenStemProfile> frozenStemProfileV1() noexcept {
    // Frozen 2026-10-29 from the first calibration run on seed 1337 (declared rule, no verification data seen):
    // per role {delta, offset in s}; SHA-256 of the canonical text recorded in the 5c dossier.
    FrozenStemProfile p{};
    p.roles = {{{0.30, -0.01166666667},   // pulso
                {0.30, -0.01066666667},   // corpo
                {0.20, +0.009958333333},  // trama
                {0.20, -0.009333333333}}}; // vettore
    return p;
}

std::string stemProfileSha256(const std::array<RoleOnsetProfile, kRoleCount>& roles) {
    std::ostringstream s;
    for (std::size_t r = 0; r < kRoleCount; ++r) {
        s << kRoleNames[r] << ' ' << std::fixed << std::setprecision(4) << roles[r].delta << ' '
          << std::llround(roles[r].offsetS * 1e6) << '\n';
    }
    return sha256Hex(std::string_view(s.str()));
}

Acao02Result runAcao02V1(const std::string& outputDir, bool writeFiles) {
    Acao02Result r;
    StemCapture cap(kFixtureCaptureFrames);
    r.stemBytes = cap.bytesAllocated();

    // ---- a. intrinsic latency on synthetic set A (delta 0.10, no offset) ----
    {
        std::vector<AppliedEventRecord> applied;
        for (std::size_t role = 0; role < kRoleCount; ++role) {
            r.intrinsicCaptures.push_back(renderSyntheticRole(static_cast<audio::MusicRole>(role), syntheticSetA(role), cap, applied));
            if (!captureInvalidReason(r.intrinsicCaptures.back()).empty()) continue; // no scores on invalid data
            const auto refs = roleReferenceOnsetsS(applied, static_cast<audio::MusicRole>(role), kFs);
            const auto rel = roleReleaseTimesS(applied, static_cast<audio::MusicRole>(role), kFs);
            r.intrinsic[role] = toRoleSet(evaluateRoleOnsets(cap.stem(role), refs, RoleOnsetProfile{0.10, 0.0}, kFs, rel));
        }
    }

    // ---- b. calibration on fixture-m2-4-v1, seed 1337 ----
    std::vector<AppliedEventRecord> applied1337;
    {
        cap.reset();
        std::uint64_t appliedOverflow = 0;
        audio::ProbeAnalysisCapture pc{};
        pc.fixtureSeed = kCalibrationSeed;
        pc.stems = cap.sink();
        pc.applied = &applied1337;
        pc.appliedOverflow = &appliedOverflow;
        const auto fixture = audio::runFixtureProbeV1Offline(outputDir, false, audio::FixtureInjectedFault::None, &pc);
        r.fixturePassed = fixture.passed;
        r.calibrationCapture = probeCaptureCheck(fixture.passed, fixture.failureReason, fixture.requestedSampleFrames, cap,
                                                 appliedOverflow);
        r.stemFrames = cap.frames();
        r.stemOverflowFrames = cap.overflowFrames();

        // e. reconstruction and Trama presence per phrase of section A (measured on the stem PCM)
        const ReconstructionResult rec = reconstructBusMix(cap);
        r.reconstructionStatus = analysisStatusName(rec.status);
        r.reconstructionMismatches = rec.mismatchedSamples;
        r.reconstructionMaxAbsError = rec.maxAbsError;
        for (std::size_t p = 0; p < 8; ++p) {
            const auto pres = measureStemPresence(cap.stem(2), p * 384000, (p + 1) * 384000);
            r.tramaPresenceA[p] = pres.exactSilence ? "EXACT_SILENCE" : analysisStatusName(pres.status);
            r.tramaPeakDbfsA[p] = pres.exactSilence ? -1.0e9 : pres.peakDbfs;
        }

        r.calibrationAnalysisOk = true;
        for (std::size_t role = 0; role < kRoleCount; ++role) {
            const auto refs = roleReferenceOnsetsS(applied1337, static_cast<audio::MusicRole>(role), kFs);
            const std::vector<float> mono = monoFromStereo(cap.stem(role));
            const MagnitudeSpectrogram mag = stftMagnitude(mono);
            const NoveltyCurve nov = spectralFluxNovelty(mag, OnsetParams{}.logGamma);
            if (nov.status != AnalysisStatus::Ok) {
                r.failureReason += std::string(kRoleNames[role]) + " calibration novelty status " +
                                   analysisStatusName(nov.status) + "; ";
                r.calibrationAnalysisOk = false;
                continue;
            }
            CalibrationGridPoint best{};
            bool haveBest = false;
            for (const double delta : kCalibrationDeltas) {
                OnsetParams p{};
                p.delta = delta;
                const OnsetFrames fr = pickOnsetFrames(nov.values, p, kDescriptorsV1Stft, kFs);
                std::vector<double> est;
                for (const auto f : fr.frames) est.push_back(frameCentreSeconds(f, kDescriptorsV1Stft, kFs));
                const SignedErrorStats raw = nearestSignedErrors(refs, est);
                std::vector<double> corrected = est;
                for (auto& t : corrected) t -= raw.medianS;
                const OnsetScore sc = matchOnsetsMaxCardinality(refs, corrected, 0.050);
                const CalibrationGridPoint pt{delta, raw.medianS, est.size(), sc.matches, sc.precision, sc.recall, sc.fMeasure};
                r.grid[role].push_back(pt);
                if (!haveBest || pt.f > best.f + 1e-9 || (std::abs(pt.f - best.f) <= 1e-9 && pt.delta > best.delta)) {
                    best = pt;
                    haveBest = true;
                }
            }
            r.calibrated[role] = RoleOnsetProfile{best.delta, best.offsetS};
        }
        r.calibratedSha256 = stemProfileSha256(r.calibrated);
        r.calibrationScores = evaluateCapture(cap, applied1337, r.calibrated);
    }

    // ---- c. frozen profile ----
    const auto frozen = frozenStemProfileV1();
    r.profileFrozen = frozen.has_value();
    if (frozen) {
        r.frozenSha256 = stemProfileSha256(frozen->roles);
        r.calibratedEqualsFrozen = (r.frozenSha256 == r.calibratedSha256);
    }

    // ---- d. verification (frozen profile only) ----
    for (std::uint64_t s = kSeedScanFirst; s <= kSeedScanLast; ++s) r.scannedSeeds.push_back(s);
    {
        const auto base = fixturePlanFingerprint(kCalibrationSeed);
        std::map<std::uint64_t, int> unique;
        unique[base.hash] = 1;
        for (const auto s : r.scannedSeeds) {
            const auto fp = fixturePlanFingerprint(s);
            if (!fp.valid) continue;
            unique[fp.hash] = 1;
            if (!fp.sameContent(base)) r.distinctSeeds.push_back(s);
        }
        r.distinctFingerprints = unique.size();
    }
    if (frozen) {
        // (1) fixture seeds with a different musical content
        if (r.distinctSeeds.empty()) {
            VerificationSet vs{"fixture_other_seeds", "INCONCLUSIVE",
                               "no scanned seed produces a musical content different from seed 1337", {}};
            r.verification.push_back(vs);
        }
        for (std::size_t i = 0; i < r.distinctSeeds.size() && i < kMaxVerificationSeeds; ++i) {
            cap.reset();
            std::vector<AppliedEventRecord> applied;
            std::uint64_t appliedOverflow = 0;
            audio::ProbeAnalysisCapture pc{};
            pc.fixtureSeed = r.distinctSeeds[i];
            pc.stems = cap.sink();
            pc.applied = &applied;
            pc.appliedOverflow = &appliedOverflow;
            const auto probe = audio::runFixtureProbeV1Offline(outputDir, false, audio::FixtureInjectedFault::None, &pc);
            const CaptureCheck check = probeCaptureCheck(probe.passed, probe.failureReason, probe.requestedSampleFrames,
                                                         cap, appliedOverflow);
            const bool valid = captureInvalidReason(check).empty();
            r.verification.push_back(verificationSetFromCaptures(
                "fixture_seed_" + std::to_string(r.distinctSeeds[i]), {check},
                valid ? evaluateCapture(cap, applied, frozen->roles) : std::array<RoleSetEvaluation, kRoleCount>{}));
        }
        // (2) planner_probe_v1
        {
            cap.reset();
            std::vector<AppliedEventRecord> applied;
            std::uint64_t appliedOverflow = 0;
            audio::ProbeAnalysisCapture pc{};
            pc.stems = cap.sink();
            pc.applied = &applied;
            pc.appliedOverflow = &appliedOverflow;
            const auto probe = audio::runPlannerProbeV1Offline(outputDir, false, &pc);
            const CaptureCheck check = probeCaptureCheck(probe.passed, probe.failureReason, probe.totalCapturedFrames,
                                                         cap, appliedOverflow);
            const bool valid = captureInvalidReason(check).empty();
            r.verification.push_back(verificationSetFromCaptures(
                "planner_probe_v1", {check},
                valid ? evaluateCapture(cap, applied, frozen->roles) : std::array<RoleSetEvaluation, kRoleCount>{}));
        }
        // (3) synthetic set B
        {
            std::array<RoleSetEvaluation, kRoleCount> roles{};
            std::vector<CaptureCheck> checks;
            std::vector<AppliedEventRecord> applied;
            for (std::size_t role = 0; role < kRoleCount; ++role) {
                checks.push_back(renderSyntheticRole(static_cast<audio::MusicRole>(role), syntheticSetB(role), cap, applied));
                if (!captureInvalidReason(checks.back()).empty()) continue;
                const auto refs = roleReferenceOnsetsS(applied, static_cast<audio::MusicRole>(role), kFs);
                const auto rel = roleReleaseTimesS(applied, static_cast<audio::MusicRole>(role), kFs);
                roles[role] = toRoleSet(evaluateRoleOnsets(cap.stem(role), refs, frozen->roles[role], kFs, rel));
            }
            r.verification.push_back(verificationSetFromCaptures("synthetic_set_b", checks, roles));
        }
    }

    r.completed = true;
    finalizeAcao02Status(r);

    if (writeFiles) {
        std::error_code ec;
        std::filesystem::create_directories(outputDir, ec);
        r.manifestFilePath = (std::filesystem::path(outputDir) / "acao02_v1_manifest.json").lexically_normal().string();
        std::ofstream mf(r.manifestFilePath);
        r.manifestWriteSuccess = mf.is_open();
        if (r.manifestWriteSuccess) {
            mf << r.toJson();
            r.manifestWriteSuccess = mf.good();
        }
    }
    return r;
}

namespace {

void writeRoleSet(std::ostringstream& s, const RoleSetEvaluation& e) {
    s << "{\"status\": \"" << e.status << "\", \"references\": " << e.references << ", \"detections\": " << e.detections
      << ", \"p50\": " << num(e.precision50) << ", \"r50\": " << num(e.recall50) << ", \"f50\": " << num(e.f50)
      << ", \"p25\": " << num(e.precision25) << ", \"r25\": " << num(e.recall25) << ", \"f25\": " << num(e.f25)
      << ", \"err_median_ms\": " << num(e.errMedianMs) << ", \"err_p05_ms\": " << num(e.errP05Ms)
      << ", \"err_p95_ms\": " << num(e.errP95Ms) << ", \"unmatched\": " << e.unmatched
      << ", \"release_detections\": " << e.releaseDetections << ", \"precision_excl_releases_50\": " << num(e.precisionExclReleases)
      << ", \"unmatched_lag_ms\": {\"median\": " << num(e.unmatchedLagMedianMs) << ", \"p05\": " << num(e.unmatchedLagP05Ms)
      << ", \"p95\": " << num(e.unmatchedLagP95Ms) << "}}";
}

void writeRoles(std::ostringstream& s, const std::array<RoleSetEvaluation, kRoleCount>& roles, const char* indent) {
    s << "{\n";
    for (std::size_t r = 0; r < kRoleCount; ++r) {
        s << indent << "  \"" << kRoleNames[r] << "\": ";
        writeRoleSet(s, roles[r]);
        s << (r + 1 < kRoleCount ? ",\n" : "\n");
    }
    s << indent << "}";
}

} // namespace

std::string Acao02Result::toJson() const {
    std::ostringstream s;
    s << "{\n";
    s << "  \"manifest_version\": \"1.0\",\n";
    s << "  \"scenario\": \"acao02_v1\",\n";
    s << "  \"build_id\": \"" << CYMATICA_BUILD_ID << "\",\n";
    s << "  \"verdict\": \"" << status << "\",\n";
    s << "  \"inconclusive_reasons\": [";
    for (std::size_t i = 0; i < inconclusiveReasons.size(); ++i) {
        s << "\"" << jsonEscape(inconclusiveReasons[i]) << "\"" << (i + 1 < inconclusiveReasons.size() ? ", " : "");
    }
    s << "],\n";
    s << "  \"gate\": false,\n";
    s << "  \"descriptors_version\": \"descriptors_v1\",\n";
    s << "  \"tolerances_ms\": {\"primary\": 50, \"diagnostic\": 25, \"calibration_window\": 100},\n";
    s << "  \"calibration_deltas\": [";
    for (std::size_t i = 0; i < kCalibrationDeltas.size(); ++i) s << num(kCalibrationDeltas[i]) << (i + 1 < kCalibrationDeltas.size() ? ", " : "");
    s << "],\n";
    s << "  \"intrinsic_latency_set_a\": ";
    writeRoles(s, intrinsic, "  ");
    s << ",\n  \"intrinsic_captures_valid\": " << (intrinsicCaptures.size() == kRoleCount &&
                                                   std::all_of(intrinsicCaptures.begin(), intrinsicCaptures.end(),
                                                               [](const CaptureCheck& c) { return captureInvalidReason(c).empty(); })
                                                   ? "true" : "false");
    s << ",\n  \"calibration\": {\n    \"seed\": 1337, \"fixture_passed\": " << (fixturePassed ? "true" : "false")
      << ", \"capture_valid\": " << (captureInvalidReason(calibrationCapture).empty() ? "true" : "false")
      << ", \"analysis_ok\": " << (calibrationAnalysisOk ? "true" : "false") << ",\n";
    s << "    \"grid\": {\n";
    for (std::size_t r = 0; r < kRoleCount; ++r) {
        s << "      \"" << kRoleNames[r] << "\": [";
        for (std::size_t i = 0; i < grid[r].size(); ++i) {
            const auto& g = grid[r][i];
            s << "{\"delta\": " << num(g.delta) << ", \"offset_ms\": " << num(g.offsetS * 1000.0) << ", \"detections\": "
              << g.detections << ", \"pairs\": " << g.pairs << ", \"p\": " << num(g.precision) << ", \"r\": " << num(g.recall)
              << ", \"f\": " << num(g.f) << "}" << (i + 1 < grid[r].size() ? ", " : "");
        }
        s << "]" << (r + 1 < kRoleCount ? ",\n" : "\n");
    }
    s << "    },\n    \"calibrated_profile\": {";
    for (std::size_t r = 0; r < kRoleCount; ++r) {
        s << "\"" << kRoleNames[r] << "\": {\"delta\": " << num(calibrated[r].delta) << ", \"offset_ms\": "
          << num(calibrated[r].offsetS * 1000.0) << "}" << (r + 1 < kRoleCount ? ", " : "");
    }
    s << "},\n    \"calibrated_sha256\": \"" << calibratedSha256 << "\",\n    \"scores\": ";
    writeRoles(s, calibrationScores, "    ");
    s << "\n  },\n";
    s << "  \"frozen_profile\": {\"frozen\": " << (profileFrozen ? "true" : "false") << ", \"sha256\": \"" << frozenSha256
      << "\", \"equals_calibrated\": " << (calibratedEqualsFrozen ? "true" : "false") << "},\n";
    s << "  \"seed_scan\": {\"seeds\": [" << scannedSeeds.front() << ", \"...\", " << scannedSeeds.back()
      << "], \"distinct_fingerprints\": " << distinctFingerprints << ", \"seeds_with_different_content\": [";
    for (std::size_t i = 0; i < distinctSeeds.size(); ++i) s << distinctSeeds[i] << (i + 1 < distinctSeeds.size() ? ", " : "");
    s << "]},\n";
    s << "  \"verification_status\": \"" << verificationStatus << "\",\n";
    s << "  \"verification\": [\n";
    for (std::size_t i = 0; i < verification.size(); ++i) {
        const auto& v = verification[i];
        s << "    {\"set\": \"" << v.name << "\", \"status\": \"" << v.status << "\", \"invalid_capture\": "
          << (v.invalidCapture ? "true" : "false") << ", \"note\": \"" << jsonEscape(v.note) << "\", \"roles\": ";
        writeRoles(s, v.roles, "    ");
        s << "}" << (i + 1 < verification.size() ? ",\n" : "\n");
    }
    s << "  ],\n";
    s << "  \"proposed_thresholds_status\": \"" << thresholdsStatus << "\",\n";
    s << "  \"proposed_thresholds_50ms\": {";
    for (std::size_t r = 0; r < kRoleCount; ++r) {
        s << "\"" << kRoleNames[r] << "\": {\"recall\": " << num(proposedRecall[r]) << ", \"precision\": "
          << num(proposedPrecision[r]) << "}" << (r + 1 < kRoleCount ? ", " : "");
    }
    s << "},\n";
    s << "  \"stems\": {\"frames\": " << stemFrames << ", \"overflow_frames\": " << stemOverflowFrames
      << ", \"bytes_allocated\": " << stemBytes << ", \"reconstruction_status\": \"" << reconstructionStatus
      << "\", \"reconstruction_mismatched_samples\": " << reconstructionMismatches
      << ", \"reconstruction_max_abs_error\": " << num(reconstructionMaxAbsError) << "},\n";
    s << "  \"trama_presence_section_a\": [";
    for (std::size_t p = 0; p < 8; ++p) {
        s << "{\"phrase\": " << p << ", \"status\": \"" << tramaPresenceA[p] << "\", \"peak_dbfs\": "
          << num(tramaPeakDbfsA[p] < -1e8 ? std::nan("") : tramaPeakDbfsA[p]) << "}" << (p < 7 ? ", " : "");
    }
    s << "]";
    if (!failureReason.empty()) s << ",\n  \"failure_reason\": \"" << jsonEscape(failureReason) << "\"";
    s << "\n}\n";
    return s.str();
}

} // namespace cymatica::cacp
