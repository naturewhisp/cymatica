#include <catch2/catch_test_macros.hpp>

#include "acao02_calibration.h"
#include "audio_engine.h"
#include "audio_probe.h"
#include "audio_scheduler.h"
#include "event_chain_log.h"
#include "stem_analysis.h"

#include <cmath>
#include <limits>
#include <string>
#include <vector>

using namespace cymatica::cacp;
using cymatica::audio::AudioCommand;
using cymatica::audio::AudioCommandAck;
using cymatica::audio::AudioCommandType;
using cymatica::audio::AudioEngine;
using cymatica::audio::AudioScheduler;
using cymatica::audio::MusicIntentEvent;
using cymatica::audio::MusicRole;

namespace {

// Renders notes of one role offline with the stem sink installed; returns the rendered frame count
std::uint64_t renderRoleNotes(MusicRole role, const std::vector<std::pair<double, double>>& onsetsAndDurations,
                              StemCapture& cap, std::vector<AppliedEventRecord>& applied, bool pauseHalfway = false) {
    AudioEngine engine;
    REQUIRE(engine.initOffline());
    AppliedEventLog log(1024);
    REQUIRE(engine.setAppliedEventLog(&log));
    REQUIRE(engine.setRoleStemSink(cap.sink()));
    AudioScheduler scheduler({.lookaheadFrames = 7200, .sampleRate = 48000});
    engine.sendCommand(AudioCommand{.commandId = 1, .type = AudioCommandType::Start, .targetSampleFrame = 0});
    std::uint64_t id = 1;
    for (const auto& [t, d] : onsetsAndDurations) {
        scheduler.scheduleIntent(MusicIntentEvent{.eventId = id++, .sampleFrame = static_cast<std::uint64_t>(t * 48000),
                                                  .transportEpoch = 1, .role = role, .pitchHz = 220.0f, .velocity = 0.8f,
                                                  .durationFrames = static_cast<std::uint32_t>(d * 48000), .timbreParam = 0.5f},
                                 1);
    }
    std::vector<float> block(400 * 2);
    const std::uint64_t ticks = 360; // 3 s
    for (std::uint64_t t = 0; t < ticks; ++t) {
        if (pauseHalfway && t == ticks / 2) {
            engine.sendCommand(AudioCommand{.commandId = 2, .type = AudioCommandType::Pause, .targetSampleFrame = t * 400});
        }
        (void)scheduler.dispatchDueEvents(t * 400, engine);
        engine.processBlock(block.data(), 400);
        AudioCommandAck ack{};
        while (engine.pollAck(ack)) {}
    }
    REQUIRE(engine.setRoleStemSink(nullptr));
    REQUIRE(engine.setAppliedEventLog(nullptr));
    applied.assign(log.records().begin(), log.records().end());
    return ticks * 400;
}

} // namespace

TEST_CASE("Stem tap: frame-aligned, bit-exact bus-mix reconstruction and role isolation (D-M2-13)", "[stem_analysis][stem_tap]") {
    StemCapture cap(200000);
    std::vector<AppliedEventRecord> applied;
    const auto rendered = renderRoleNotes(MusicRole::Corpo, {{0.10, 0.30}, {0.60, 0.30}, {1.10, 0.20}}, cap, applied);

    // Every output frame went through exactly one renderBlock call
    CHECK(cap.frames() == rendered);
    CHECK(cap.overflowFrames() == 0);

    // Sum of the four stems in bus order == bus mix, bit-exact
    const auto rec = reconstructBusMix(cap);
    CHECK(rec.status == AnalysisStatus::Ok);
    CHECK(rec.mismatchedSamples == 0);
    CHECK(rec.maxAbsError == 0.0);

    // Only the Corpo stem carries signal; the others are exactly zero
    const auto corpo = measureStemPresence(cap.stem(1), 0, cap.frames());
    CHECK(corpo.status == AnalysisStatus::Ok);
    CHECK(corpo.peakDbfs > -40.0);
    for (const std::size_t r : {0u, 2u, 3u}) {
        const auto p = measureStemPresence(cap.stem(r), 0, cap.frames());
        CHECK(p.status == AnalysisStatus::ExactSilence);
        CHECK(p.exactSilence);
    }
}

TEST_CASE("Stem tap stays aligned through frozen pause silence and counts overflow (D-M2-13)", "[stem_analysis][stem_tap]") {
    StemCapture cap(200000);
    std::vector<AppliedEventRecord> applied;
    const auto rendered = renderRoleNotes(MusicRole::Vettore, {{0.10, 0.40}}, cap, applied, true);
    CHECK(cap.frames() == rendered); // frozen-silence frames are written as zeros
    const auto tail = measureStemPresence(cap.stem(3), cap.frames() - 4800, cap.frames());
    CHECK(tail.status == AnalysisStatus::ExactSilence);

    StemCapture small(1000);
    std::vector<AppliedEventRecord> applied2;
    const auto rendered2 = renderRoleNotes(MusicRole::Pulso, {{0.01, 0.05}}, small, applied2);
    CHECK(small.frames() == 1000);
    CHECK(small.overflowFrames() == rendered2 - 1000);
}

TEST_CASE("Stem presence, references, releases and the release diagnostic (D-M2-14, OSS-5b-01)", "[stem_analysis][references]") {
    // Presence measured on PCM
    const std::vector<float> silent(2000, 0.0f);
    CHECK(measureStemPresence(silent, 0, 1000).status == AnalysisStatus::ExactSilence);
    std::vector<float> tiny(2000, 0.0f);
    tiny[100] = 1e-30f; // physically present, far below any spectrogram floor
    const auto pTiny = measureStemPresence(tiny, 0, 1000);
    CHECK(pTiny.status == AnalysisStatus::Ok);
    CHECK_FALSE(pTiny.exactSilence);
    CHECK(measureStemPresence(tiny, 10, 5).status == AnalysisStatus::InvalidConfig);
    tiny[50] = std::numeric_limits<float>::quiet_NaN();
    CHECK(measureStemPresence(tiny, 0, 1000).status == AnalysisStatus::NonFiniteInput);

    // References: Applied NoteOns of the role, merged within 30 ms keeping the first; releases from NoteOffs
    using cymatica::audio::AudioEventType;
    auto rec = [](AudioEventType type, MusicRole role, std::uint64_t frame, EventDisposition d) {
        return AppliedEventRecord{.id = {.eventId = frame, .sampleFrame = frame, .transportEpoch = 1, .type = type, .role = role},
                                  .consumedFrame = frame, .disposition = d};
    };
    const std::vector<AppliedEventRecord> applied{
        rec(AudioEventType::NoteOn, MusicRole::Corpo, 48000, EventDisposition::Applied),
        rec(AudioEventType::NoteOn, MusicRole::Corpo, 48000 + 960, EventDisposition::Applied),   // +20 ms: fused
        rec(AudioEventType::NoteOn, MusicRole::Corpo, 96000, EventDisposition::Applied),
        rec(AudioEventType::NoteOn, MusicRole::Corpo, 120000, EventDisposition::Dropped),       // not applied
        rec(AudioEventType::NoteOn, MusicRole::Trama, 50000, EventDisposition::Applied),        // other role
        rec(AudioEventType::NoteOff, MusicRole::Corpo, 72000, EventDisposition::Applied)};
    const auto refs = roleReferenceOnsetsS(applied, MusicRole::Corpo);
    REQUIRE(refs.size() == 2);
    CHECK(refs[0] == 1.0);
    CHECK(refs[1] == 2.0);
    const auto rel = roleReleaseTimesS(applied, MusicRole::Corpo);
    REQUIRE(rel.size() == 1);
    CHECK(rel[0] == 1.5);

    // On a real render every extra detection of a sustained note sits on its release (2 ms ramp)
    StemCapture cap(200000);
    std::vector<AppliedEventRecord> rendered;
    (void)renderRoleNotes(MusicRole::Vettore, {{0.25, 0.30}, {0.75, 0.30}, {1.25, 0.30}, {1.75, 0.30}}, cap, rendered);
    const auto vRefs = roleReferenceOnsetsS(rendered, MusicRole::Vettore);
    const auto vRel = roleReleaseTimesS(rendered, MusicRole::Vettore);
    REQUIRE(vRefs.size() == 4);
    REQUIRE(vRel.size() == 4);
    const auto ev = evaluateRoleOnsets(cap.stem(3), vRefs, RoleOnsetProfile{0.10, 0.0}, 48000, vRel);
    REQUIRE(ev.status == AnalysisStatus::Ok);
    CHECK(ev.at50.recall == 1.0);
    CHECK(ev.detections == ev.at50.matches + ev.releaseDetections);
    CHECK(ev.precisionExclReleases == 1.0);

    // Invalid offset is a configuration error
    CHECK(evaluateRoleOnsets(cap.stem(3), vRefs, RoleOnsetProfile{0.10, std::nan("")}).status == AnalysisStatus::InvalidConfig);
}

TEST_CASE("Frozen stem onset profile v1 is versioned and hashed (D-M2-16)", "[stem_analysis][frozen_profile]") {
    const auto frozen = frozenStemProfileV1();
    REQUIRE(frozen.has_value());
    CHECK(std::string(frozen->version) == "stem_onset_profile_v1");
    // Any change of the frozen values must change this hash and be recorded as a new profile version
    CHECK(stemProfileSha256(frozen->roles) == "1ad4410052efe85e0fb4d4ea00c7e77383da9a1cea05e4b49fc5fea93bd8b3cd");
}

TEST_CASE("planner_probe_v1 analysis capture: stems aligned with the PCM and applied events recorded", "[stem_analysis][probe_capture]") {
    StemCapture cap(800000);
    std::vector<float> stereo;
    std::vector<AppliedEventRecord> applied;
    cymatica::audio::ProbeAnalysisCapture pc{};
    pc.stems = cap.sink();
    pc.stereo = &stereo;
    pc.applied = &applied;
    const auto res = cymatica::audio::runPlannerProbeV1Offline("artifacts/cacp", false, &pc);
    INFO(res.failureReason);
    CHECK(res.passed);
    CHECK(cap.overflowFrames() == 0);
    CHECK(cap.frames() * 2 == stereo.size());
    CHECK(reconstructBusMix(cap).mismatchedSamples == 0);
    std::size_t noteOns = 0;
    for (const auto& r : applied) if (r.id.type == cymatica::audio::AudioEventType::NoteOn) ++noteOns;
    CHECK(noteOns == res.generatedEvents);
}

TEST_CASE("fixture_probe_v1 on another seed: goldens NOT_APPLICABLE, structural checks kept, fail-closed (DIF-5c-01)",
          "[stem_analysis][seed_probe]") {
    using cymatica::audio::FixtureInjectedFault;
    using cymatica::audio::ProbeAnalysisCapture;
    using cymatica::audio::runFixtureProbeV1Offline;

    // Seed 2: same musical content as 1337 (declared scan, 5c) but a different provenance hash, so the 1337 goldens
    // cannot match. The probe must still pass on determinism, offline == JIT, O3 and integrity.
    ProbeAnalysisCapture pc{};
    pc.fixtureSeed = 2;
    const auto res = runFixtureProbeV1Offline("artifacts/cacp", false, FixtureInjectedFault::None, &pc);
    INFO(res.failureReason);
    CHECK(res.passed);
    CHECK(res.runSeed == 2);
    CHECK_FALSE(res.goldenApplicable);
    CHECK(res.canonicalPlanHash != cymatica::audio::kFixtureProbeV1GoldenPlanHash);
    CHECK(res.jitGoldenStatus == "NOT_APPLICABLE");
    CHECK(res.jitOfflinePlanMatch);
    CHECK(res.contentFingerprintJitMatch);
    CHECK(res.determinismPassed);
    CHECK(res.eventChainPassed);
    const std::string json = res.toJson();
    CHECK(json.find("\"run_seed\": 2,") != std::string::npos);
    CHECK(json.find("\"golden_applicable\": false") != std::string::npos);
    CHECK(json.find("\"golden_plan_passed\": \"NOT_APPLICABLE\"") != std::string::npos);

    // The same seed with an injected fault still fails: NOT_APPLICABLE goldens never turn a failure into a pass
    const auto failed = runFixtureProbeV1Offline("artifacts/cacp", false, FixtureInjectedFault::InvalidTempoMap, &pc);
    CHECK_FALSE(failed.passed);
    CHECK(failed.coordinatorFailed);
    CHECK_FALSE(failed.failureReason.empty());
}

namespace {

CaptureCheck validCheck(std::size_t frames = 1000) {
    CaptureCheck c;
    c.rendered = true;
    c.probePassed = true;
    c.expectedFrames = frames;
    c.stemFrames = frames;
    return c;
}

std::array<RoleSetEvaluation, kRoleCount> measuredRoles(double recall, double precision) {
    std::array<RoleSetEvaluation, kRoleCount> roles{};
    for (auto& r : roles) {
        r.status = "MEASURED";
        r.references = 10;
        r.recall50 = recall;
        r.precision50 = precision;
    }
    return roles;
}

// A result whose every recorded check is valid: the aggregate must be MEASURED with proposed thresholds
Acao02Result validAcao02() {
    Acao02Result r;
    r.completed = true;
    for (std::size_t i = 0; i < kRoleCount; ++i) r.intrinsicCaptures.push_back(validCheck());
    r.fixturePassed = true;
    r.calibrationCapture = validCheck(8'825'200);
    r.calibrationAnalysisOk = true;
    r.reconstructionStatus = "OK";
    r.profileFrozen = true;
    r.calibratedSha256 = "1ad44100";
    r.frozenSha256 = "1ad44100";
    r.calibratedEqualsFrozen = true;
    r.verification.push_back(VerificationSet{"fixture_other_seeds", "INCONCLUSIVE", "no different content", {}, false});
    r.verification.push_back(verificationSetFromCaptures("planner_probe_v1", {validCheck()}, measuredRoles(0.95, 1.0)));
    r.verification.push_back(
        verificationSetFromCaptures("synthetic_set_b", {validCheck(), validCheck()}, measuredRoles(0.80, 0.50)));
    return r;
}

} // namespace

TEST_CASE("Capture validity: render, probe verdict, stem coverage and overflow (DIF-5c-02)", "[stem_analysis][validity]") {
    CHECK(captureInvalidReason(validCheck()).empty());

    const CaptureCheck notRendered{};
    CHECK_FALSE(captureInvalidReason(notRendered).empty());

    auto failedProbe = validCheck();
    failedProbe.probePassed = false;
    failedProbe.probeFailure = "golden mismatch";
    CHECK(captureInvalidReason(failedProbe).find("golden mismatch") != std::string::npos);

    auto shortStems = validCheck();
    shortStems.stemFrames = 999;
    CHECK_FALSE(captureInvalidReason(shortStems).empty());

    auto overflow = validCheck();
    overflow.stemOverflowFrames = 1;
    CHECK(captureInvalidReason(overflow).find("overflow") != std::string::npos);

    auto logOverflow = validCheck();
    logOverflow.appliedOverflow = 3;
    CHECK(captureInvalidReason(logOverflow).find("applied-event log") != std::string::npos);

    // Real overflow on a scenario render: a stem capture too small for planner_probe_v1
    StemCapture small(1000);
    cymatica::audio::ProbeAnalysisCapture pc{};
    pc.stems = small.sink();
    const auto res = cymatica::audio::runPlannerProbeV1Offline("artifacts/cacp", false, &pc);
    CaptureCheck real;
    real.rendered = true;
    real.probePassed = res.passed;
    real.expectedFrames = res.totalCapturedFrames;
    real.stemFrames = small.frames();
    real.stemOverflowFrames = small.overflowFrames();
    CHECK(small.overflowFrames() > 0);
    CHECK_FALSE(captureInvalidReason(real).empty());

    // An invalid capture makes the set INCONCLUSIVE with no usable scores
    const auto vs = verificationSetFromCaptures("planner_probe_v1", {real}, measuredRoles(0.9, 0.9));
    CHECK(vs.status == "INCONCLUSIVE");
    CHECK(vs.invalidCapture);
    for (const auto& role : vs.roles) CHECK(role.status == "NOT_RUN");
    // One invalid render among several (synthetic set B is one render per role) is enough
    CHECK(verificationSetFromCaptures("synthetic_set_b", {validCheck(), overflow}, measuredRoles(0.9, 0.9)).invalidCapture);
    CHECK(verificationSetFromCaptures("empty", {}, measuredRoles(0.9, 0.9)).status == "INCONCLUSIVE");
}

TEST_CASE("acao02_v1 aggregate is fail-closed: MEASURED only on valid data (DIF-5c-02)", "[stem_analysis][validity]") {
    auto expectInconclusive = [](Acao02Result r, const char* what) {
        finalizeAcao02Status(r);
        INFO(what);
        CHECK(r.status == "INCONCLUSIVE");
        CHECK(r.verificationStatus == "INCONCLUSIVE");
        CHECK(r.thresholdsStatus == "NOT_PROPOSED");
        CHECK_FALSE(r.inconclusiveReasons.empty());
        for (std::size_t i = 0; i < kRoleCount; ++i) {
            CHECK(std::isnan(r.proposedRecall[i]));
            CHECK(std::isnan(r.proposedPrecision[i]));
        }
        return r;
    };

    // Nominal: MEASURED; thresholds from the valid MEASURED sets only (min - 0.05, floored to 0.05)
    {
        auto r = validAcao02();
        finalizeAcao02Status(r);
        INFO((r.inconclusiveReasons.empty() ? std::string{} : r.inconclusiveReasons.front()));
        CHECK(r.status == "MEASURED");
        CHECK(r.verificationStatus == "MEASURED");
        CHECK(r.thresholdsStatus == "PROPOSED");
        CHECK(std::abs(r.proposedRecall[0] - 0.75) < 1e-9);
        CHECK(std::abs(r.proposedPrecision[0] - 0.45) < 1e-9);
    }
    // Calibrated profile differs from the frozen one: verification measures kept as diagnostic, not conclusive
    {
        auto r = validAcao02();
        r.calibratedSha256 = "different";
        r.calibratedEqualsFrozen = false;
        const auto out = expectInconclusive(r, "profile mismatch");
        CHECK(out.verification[1].status == "MEASURED");
        CHECK(out.verification[1].roles[0].recall50 > 0.9);
    }
    // Stem overflow in the calibration capture
    {
        auto r = validAcao02();
        r.calibrationCapture.stemOverflowFrames = 10;
        (void)expectInconclusive(r, "calibration overflow");
    }
    // Failed calibration fixture probe
    {
        auto r = validAcao02();
        r.fixturePassed = false;
        r.calibrationCapture.probePassed = false;
        (void)expectInconclusive(r, "calibration probe failed");
    }
    // Bus-mix reconstruction mismatch
    {
        auto r = validAcao02();
        r.reconstructionMismatches = 1;
        (void)expectInconclusive(r, "reconstruction mismatch");
    }
    // A verification probe that failed (e.g. an alternative seed): the set and the aggregate are INCONCLUSIVE
    {
        auto r = validAcao02();
        auto bad = validCheck();
        bad.probePassed = false;
        bad.probeFailure = "probe FAIL";
        r.verification.push_back(verificationSetFromCaptures("fixture_seed_7", {bad}, measuredRoles(0.99, 0.99)));
        (void)expectInconclusive(r, "verification probe failed");
    }
    // Invalid intrinsic-latency render
    {
        auto r = validAcao02();
        r.intrinsicCaptures[2].stemOverflowFrames = 5;
        (void)expectInconclusive(r, "intrinsic overflow");
    }
    // Profile not frozen
    {
        auto r = validAcao02();
        r.profileFrozen = false;
        r.calibratedEqualsFrozen = false;
        (void)expectInconclusive(r, "not frozen");
    }
    // No verification set measured on valid data
    {
        auto r = validAcao02();
        r.verification.resize(1);
        (void)expectInconclusive(r, "no measured set");
    }
}
