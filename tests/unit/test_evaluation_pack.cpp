#include <catch2/catch_test_macros.hpp>

#include "acao02_calibration.h"
#include "audio_engine.h"
#include "audio_probe.h"
#include "audio_scheduler.h"
#include "evaluation_pack.h"
#include "music_planner.h"
#include "sha256.h"

#include <nlohmann/json.hpp>

#include <cmath>
#include <filesystem>
#include <fstream>
#include <functional>
#include <memory>
#include <regex>
#include <string>
#include <vector>

using namespace cymatica::cacp;
using cymatica::audio::MusicRole;
using nlohmann::json;

namespace {

// planner_probe_v1 rendered with stems, applied events and the plan, as a pack source
struct PlannerSource {
    StemCapture cap{800000};
    std::vector<AppliedEventRecord> applied;
    std::vector<cymatica::audio::MusicIntentEvent> intents;
    std::vector<cymatica::audio::PlannedNoteOff> noteOffs;
    PackSource src;
};

std::unique_ptr<PlannerSource> plannerSource() {
    auto ps = std::make_unique<PlannerSource>();
    cymatica::audio::ProbeAnalysisCapture pc{};
    pc.stems = ps->cap.sink();
    pc.applied = &ps->applied;
    pc.intents = &ps->intents;
    pc.noteOffs = &ps->noteOffs;
    const auto res = cymatica::audio::runPlannerProbeV1Offline("artifacts/cacp", false, &pc);
    REQUIRE(res.passed);
    REQUIRE(ps->cap.overflowFrames() == 0);
    ps->src.name = "planner_probe_v1";
    for (std::size_t r = 0; r < kRoleCount; ++r) ps->src.stems[r] = ps->cap.stem(r);
    ps->src.applied = ps->applied;
    ps->src.planned = plannedSpans(ps->intents, ps->noteOffs);
    const auto clock = cymatica::audio::MusicPlanner::makePlannerProbeV1Clock();
    REQUIRE(clock.has_value());
    for (const auto& p : cymatica::audio::MusicPlanner::makePlannerProbeV1Phrases(*clock)) {
        ps->src.phrases.push_back({static_cast<std::uint32_t>(ps->src.phrases.size()), p.sectionIndex, p.startBar, p.barCount,
                                   p.startFrame, p.endFrame});
    }
    const auto frozen = frozenStemProfileV1();
    REQUIRE(frozen.has_value());
    ps->src.onsetProfile = frozen->roles;
    ps->src.onsetProfileSha256 = stemProfileSha256(frozen->roles);
    return ps;
}

std::string str(const std::vector<std::uint8_t>& b) { return {b.begin(), b.end()}; }

std::vector<std::string> pngChunkTypes(const std::vector<std::uint8_t>& png) {
    std::vector<std::string> types;
    std::size_t pos = 8;
    while (pos + 8 <= png.size()) {
        const std::uint32_t len = (std::uint32_t{png[pos]} << 24) | (std::uint32_t{png[pos + 1]} << 16) |
                                  (std::uint32_t{png[pos + 2]} << 8) | std::uint32_t{png[pos + 3]};
        types.emplace_back(png.begin() + static_cast<std::ptrdiff_t>(pos + 4), png.begin() + static_cast<std::ptrdiff_t>(pos + 8));
        pos += 12 + len;
    }
    return types;
}

// ---- prefabricated truth for the checker ----

RoleWindowTruth roleTruth(SignalLevel s, bool planned, bool onset, std::string onsetStatus = "MEASURED") {
    RoleWindowTruth r;
    r.planned = planned;
    r.applied = planned;
    r.signal = s;
    r.onsetStatus = std::move(onsetStatus);
    r.appliedNoteOns = onset ? 2 : 0;
    r.matchedOnsets = onset ? 2 : 0;
    r.onsetDetected = onset;
    return r;
}

WindowTruth window(std::string id, std::string kind) {
    WindowTruth w;
    w.id = std::move(id);
    w.kind = std::move(kind);
    w.sideMidStatus = "OK";
    w.sideMidDb = -20.0;
    w.roles = {roleTruth(SignalLevel::ExactSilence, false, false),          // pulso
               roleTruth(SignalLevel::BelowThreshold, true, false),         // corpo
               roleTruth(SignalLevel::Present, true, true),                 // trama
               roleTruth(SignalLevel::BelowNumericalFloor, false, false)};  // vettore
    return w;
}

TruthBundle prefabTruth() {
    TruthBundle t;
    t.packSeed = 987654321;
    t.seedSha256 = packSeedCommitment(t.packSeed);
    t.blind.push_back(window("w0000000a", "test"));
    auto b = window("w0000000b", "test");
    b.sideMidDb = -10.0;  // wider than w0000000a by 10 dB
    b.clippedSamples = 3;
    b.roles[0] = roleTruth(SignalLevel::Present, true, true);
    b.roles[1] = roleTruth(SignalLevel::Unavailable, true, false, "NON_FINITE_INPUT");
    t.blind.push_back(b);
    auto c = window("w0000000c", "test");
    c.sideMidDb = -20.5;  // within the 1 dB margin of w0000000a
    t.blind.push_back(c);
    auto dup = window("w0000000d", "duplicate");
    dup.duplicateOf = "w0000000a";
    t.blind.push_back(dup);
    auto alt = window("w0000000e", "altered");
    alt.removedRole = 2;
    alt.roles[2] = roleTruth(SignalLevel::ExactSilence, true, false);
    t.blind.push_back(alt);
    auto cat = window("w0000000f", "catch");
    for (auto& r : cat.roles) r = roleTruth(SignalLevel::ExactSilence, false, false);
    t.blind.push_back(cat);
    t.diagnostic.push_back(window("p00", "test"));
    // Every window has its files, with stand-in hashes; one labelled reference per mode
    auto addFiles = [](std::map<std::string, std::string>& files, const std::string& id, const std::string& salt) {
        for (const char* suffix : {".wav", "_mid.png", "_side.png"}) files[id + suffix] = sha256Hex(std::string_view(salt + id + suffix));
    };
    for (const auto& w : t.blind) addFiles(t.publicBlindFiles, w.id, "blind:");
    t.publicBlindFiles["pack.json"] = sha256Hex(std::string_view("blind:pack.json"));
    t.publicBlindFiles["ref_trama_s0_mid.png"] = sha256Hex(std::string_view("blind:ref"));
    addFiles(t.publicDiagnosticFiles, "p00", "diag:");
    t.publicDiagnosticFiles["pack.json"] = sha256Hex(std::string_view("diag:pack.json"));
    t.publicDiagnosticFiles["p00_trama_mid.png"] = sha256Hex(std::string_view("diag:stem"));
    return t;
}

const std::vector<std::string> kSealedHashes{std::string(64, '9')};

json hashesOf(const std::map<std::string, std::string>& files) {
    json a = json::array();
    for (const auto& [name, h] : files) a.push_back(h);
    return a;
}

// A result that declares every public file of the mode
json resultBase(const TruthBundle& t, const char* mode = "blind") {
    return json{{"schema", "evaluation_result_v0"},
                {"claims_version", "claims_v0"},
                {"pack_mode", mode},
                {"evaluator", {{"type", "agent"}, {"model", "test"}, {"version", "0"}}},
                {"channel", {{"audio_channels", 2}, {"tokens_per_s", 0}}},
                {"access", {{"repository", false}, {"sealed", false}, {"diagnostic_pack", false}, {"code_execution", false}}},
                {"spectrogram_references_used", true},
                {"input_hashes", hashesOf(std::string(mode) == "blind" ? t.publicBlindFiles : t.publicDiagnosticFiles)},
                {"timestamp", "2026-10-29T00:00:00Z"},
                {"claims", json::array()}};
}

json claim(const char* window, const char* method, const char* predicate, json args, json value) {
    return json{{"window_id", window}, {"method", method}, {"predicate", predicate}, {"args", std::move(args)},
                {"claimed_value", std::move(value)}, {"text", "descriptive only"}};
}

std::string outcomeOf(const TruthBundle& t, const json& c, json base) {
    base["claims"] = json::array({c});
    const CheckReport rep = checkEvaluation(t, base.dump(), kSealedHashes);
    REQUIRE(rep.checked);
    REQUIRE(rep.claims.size() == 1);
    INFO(rep.claims[0].reason);
    return rep.claims[0].outcome;
}

std::string outcomeOf(const TruthBundle& t, const json& c) { return outcomeOf(t, c, resultBase(t)); }

json role(const char* r) { return json{{"role", r}}; }

} // namespace

TEST_CASE("evaluation_pack_v1 is deterministic and the selection stream is isolated (D-M2-15)", "[evaluation_pack]") {
    auto a = plannerSource();
    auto b = plannerSource(); // second independent render
    const PackBuild p1 = buildEvaluationPack(a->src, 424242);
    const PackBuild p2 = buildEvaluationPack(b->src, 424242);
    INFO((p1.errors.empty() ? std::string{} : p1.errors.front()));
    REQUIRE(p1.ok);
    REQUIRE(p2.ok);
    CHECK(p1.publicBlind == p2.publicBlind);
    CHECK(p1.publicDiagnostic == p2.publicDiagnostic);
    CHECK(p1.sealed == p2.sealed);
    // Another selection seed changes the opaque identifiers, never the musical content
    const PackBuild p3 = buildEvaluationPack(a->src, 424243);
    REQUIRE(p3.ok);
    CHECK(p3.publicBlind != p1.publicBlind);
    CHECK(p3.publicDiagnostic.at("p00.wav") == p1.publicDiagnostic.at("p00.wav"));
    // Sealed truth round-trips through JSON
    const auto truth = truthFromJson(str(p1.sealed.at("evaluation_truth.json")));
    REQUIRE(truth.has_value());
    CHECK(truthToJson(*truth) == str(p1.sealed.at("evaluation_truth.json")));
    CHECK(truth->blind.size() == p1.truth.blind.size());
}

TEST_CASE("evaluation_pack_v1 blind mode does not leak the ground truth (D-M2-15 leakage test)", "[evaluation_pack][leakage]") {
    auto ps = plannerSource();
    const std::uint64_t seed = 31415926535ULL;
    const PackBuild p = buildEvaluationPack(ps->src, seed);
    REQUIRE(p.ok);
    const std::regex windowFile("^w[0-9a-f]{8}(_mid|_side)?\\.(wav|png)$");
    const std::regex refFile("^ref_(pulso|corpo|trama|vettore)_s[0-9]+_mid\\.png$");
    std::size_t wavSize = 0;
    for (const auto& [name, bytes] : p.publicBlind) {
        INFO(name);
        const bool known = std::regex_match(name, windowFile) || std::regex_match(name, refFile) || name == "pack.json" ||
                           name == "SHA256SUMS";
        CHECK(known);
        if (name.ends_with(".wav")) {
            // RIFF/WAVE with only fmt and data chunks, every window of the same duration
            REQUIRE(bytes.size() > 44);
            CHECK(str({bytes.begin(), bytes.begin() + 4}) == "RIFF");
            CHECK(str({bytes.begin() + 12, bytes.begin() + 16}) == "fmt ");
            CHECK(str({bytes.begin() + 36, bytes.begin() + 40}) == "data");
            CHECK(bytes.size() == 44 + static_cast<std::size_t>(6.0 * 48000) * 4);
            if (wavSize == 0) wavSize = bytes.size();
            CHECK(bytes.size() == wavSize);
        }
        if (name.ends_with(".png")) CHECK(pngChunkTypes(bytes) == std::vector<std::string>{"IHDR", "IDAT", "IEND"});
        if (std::regex_match(name, windowFile)) {
            const std::string text = str(bytes);
            for (const char* r : kRoleNames) CHECK(text.find(r) == std::string::npos);
        }
    }
    // pack.json: only non-revealing fields; window entries carry nothing but the opaque files
    const json pack = json::parse(str(p.publicBlind.at("pack.json")));
    CHECK(pack.at("mode") == "blind");
    for (const auto& w : pack.at("windows")) {
        CHECK(w.size() == 4);
        for (const char* k : {"id", "audio", "mel_mid", "mel_side"}) CHECK(w.contains(k));
    }
    const std::string packText = str(p.publicBlind.at("pack.json"));
    for (const char* forbidden : {"start_frame", "end_frame", "start_bar", "bar_count", "epoch", "event", "phrase", "\"kind\"",
                                  "duplicate_of", "removed_role", "truth", "pack_seed", "\"section\""}) {
        INFO(forbidden);
        CHECK(packText.find(forbidden) == std::string::npos);
    }
    CHECK(packText.find(std::to_string(seed)) == std::string::npos);
    CHECK(pack.at("seed_commitment_sha256") == packSeedCommitment(seed));
    // Every window has the same duration; SHA256SUMS lists every other public file with its hash
    const std::string sums = str(p.publicBlind.at("SHA256SUMS"));
    for (const auto& [name, bytes] : p.publicBlind) {
        if (name == "SHA256SUMS") continue;
        CHECK(sums.find(sha256Hex(std::span<const std::uint8_t>(bytes.data(), bytes.size())) + "  " + name + "\n") != std::string::npos);
    }
    // The controls exist in the sealed key: duplicate, catch; diagnostic mode keeps the seed in clear
    std::size_t duplicates = 0, catches = 0;
    for (const auto& w : p.truth.blind) {
        duplicates += w.kind == "duplicate";
        catches += w.kind == "catch";
        CHECK(w.endFrame - w.startFrame == 288000);
        if (w.kind != "duplicate") continue;
        // AG-5d-03: shifted by 7 ms (336 frames) as well as attenuated, so that the PNGs differ, not only the WAV
        for (const auto& o : p.truth.blind) {
            if (o.id != w.duplicateOf) continue;
            CHECK(w.startFrame == o.startFrame + 336);
            const auto& a = p.publicBlind.at(w.id + "_mid.png");
            const auto& b = p.publicBlind.at(o.id + "_mid.png");
            REQUIRE(a.size() == b.size());
            std::size_t differing = 0;
            for (std::size_t k = 0; k < a.size(); ++k) differing += a[k] != b[k];
            CHECK(differing > a.size() / 10);
        }
    }
    CHECK(duplicates == 1);
    CHECK(catches == 1);
    // AG-5d-03: unless declared novel, the altered item leaves a role set that some phrase has
    for (const auto& w : p.truth.blind) {
        if (w.kind != "altered" || p.truth.alteredMaskNovel) continue;
        std::uint32_t remaining = 0;
        for (std::size_t r = 0; r < kRoleCount; ++r) remaining |= (w.roles[r].signal == SignalLevel::Present ? 1U : 0U) << r;
        bool exists = false;
        for (const auto& ph : p.truth.phrases) {
            std::uint32_t m = 0;
            for (std::size_t r = 0; r < kRoleCount; ++r) m |= (ph.roles[r].planned ? 1U : 0U) << r;
            exists = exists || m == remaining;
        }
        CHECK(exists);
    }
    CHECK(str(p.publicDiagnostic.at("pack.json")).find(std::to_string(seed)) != std::string::npos);
    CHECK(p.publicDiagnostic.contains("truth.json"));
}

TEST_CASE("Window truth levels: exact silence, numerical floor, threshold, presence (D-M2-15)", "[evaluation_pack][truth]") {
    constexpr std::size_t kFrames = 48000;
    std::vector<float> silent(2 * kFrames, 0.0f), tiny(2 * kFrames), weak(2 * kFrames), loud(2 * kFrames);
    for (std::size_t i = 0; i < kFrames; ++i) {
        const float s = std::sin(2.0f * 3.14159265f * 440.0f * static_cast<float>(i) / 48000.0f);
        tiny[2 * i] = tiny[2 * i + 1] = 1e-7f * s;   // -140 dBFS peak: below the -120 dBFS floor
        weak[2 * i] = weak[2 * i + 1] = 3e-4f * s;   // about -73 dBFS RMS: above the floor, below -60 dBFS
        loud[2 * i] = loud[2 * i + 1] = 0.3f * s;
    }
    PackSource src;
    src.stems = {silent, tiny, weak, loud};
    const OnsetIndex idx = buildOnsetIndex(src, 0.050);
    const WindowTruth t = measureWindow(src, idx, {0, kFrames, {true, true, true, true}, 0.0, 0.0}, presenceProfileV1());
    CHECK(t.roles[0].signal == SignalLevel::ExactSilence);
    CHECK(t.roles[1].signal == SignalLevel::BelowNumericalFloor);
    CHECK(t.roles[2].signal == SignalLevel::BelowThreshold);
    CHECK(t.roles[3].signal == SignalLevel::Present);
    CHECK(t.sideIsZero); // L = R
    // Removing a role from the delivered audio is measured as exact silence for it (altered-truth item)
    const WindowTruth removed = measureWindow(src, idx, {0, kFrames, {true, true, true, false}, 0.0, 0.0}, presenceProfileV1());
    CHECK(removed.roles[3].signal == SignalLevel::ExactSilence);
    // Outside the stems: unavailable, never silence
    const WindowTruth outside = measureWindow(src, idx, {0, kFrames + 1, {true, true, true, true}, 0.0, 0.0}, presenceProfileV1());
    CHECK(outside.roles[3].signal == SignalLevel::Unavailable);
}

TEST_CASE("Sustained pad across the window start: present, no attack, audible compatible (D-M2-15)", "[evaluation_pack][sustained]") {
    // Corpo note from 0.5 s to 3.5 s rendered by the real synth; window [1.0, 3.0) s has no NoteOn
    constexpr std::uint32_t kFs = 48000;
    StemCapture cap(5 * kFs);
    cymatica::audio::AudioEngine engine;
    REQUIRE(engine.initOffline({kFs, 2, 220.0f}));
    AppliedEventLog log(cymatica::audio::kEventChainLogCapacity);
    (void)engine.setAppliedEventLog(&log);
    (void)engine.setRoleStemSink(cap.sink());
    cymatica::audio::AudioScheduler scheduler({.lookaheadFrames = 7200, .sampleRate = kFs});
    engine.sendCommand(cymatica::audio::AudioCommand{.commandId = 1, .type = cymatica::audio::AudioCommandType::Start, .targetSampleFrame = 0});
    const cymatica::audio::MusicIntentEvent pad{.eventId = 1, .sampleFrame = kFs / 2, .tick = 0, .transportEpoch = 1,
                                                .role = MusicRole::Corpo, .pitchHz = 220.0f, .velocity = 0.8f,
                                                .durationFrames = 3 * kFs, .timbreParam = 0.5f};
    scheduler.scheduleIntent(pad, 1);
    std::vector<float> block(800);
    for (std::uint64_t t = 0; t < 5ULL * kFs / 400; ++t) {
        (void)scheduler.dispatchDueEvents(t * 400, engine);
        engine.processBlock(block.data(), 400);
        cymatica::audio::AudioCommandAck ack{};
        while (engine.pollAck(ack)) {}
    }
    (void)engine.setRoleStemSink(nullptr);
    (void)engine.setAppliedEventLog(nullptr);
    engine.shutdown();

    PackSource src;
    for (std::size_t r = 0; r < kRoleCount; ++r) src.stems[r] = cap.stem(r);
    src.applied.assign(log.records().begin(), log.records().end());
    const std::array<cymatica::audio::MusicIntentEvent, 1> intents{pad};
    src.planned = plannedSpans(intents, {});
    src.onsetProfile = frozenStemProfileV1()->roles;
    const OnsetIndex idx = buildOnsetIndex(src, 0.050);
    WindowTruth w = measureWindow(src, idx, {kFs, 3 * kFs, {true, true, true, true}, 0.0, 0.010}, presenceProfileV1());
    w.id = "w00000001";
    CHECK(w.roles[1].planned);
    CHECK(w.roles[1].applied);
    CHECK(w.roles[1].signal == SignalLevel::Present);
    CHECK(w.roles[1].appliedNoteOns == 0);
    CHECK_FALSE(w.roles[1].onsetDetected);

    TruthBundle t;
    t.packSeed = 5;
    t.blind.push_back(w);
    t.publicBlindFiles = {{"w00000001.wav", std::string(64, '2')}, {"w00000001_mid.png", std::string(64, '3')},
                          {"ref_corpo_s0_mid.png", std::string(64, '4')}};
    CHECK(outcomeOf(t, claim("w00000001", "listening", "role_signal_present", role("corpo"), true)) == "supported");
    CHECK(outcomeOf(t, claim("w00000001", "listening", "role_onset_detected", role("corpo"), true)) == "contradicted");
    CHECK(outcomeOf(t, claim("w00000001", "listening", "role_audible", role("corpo"), true)) == "compatible");
}

TEST_CASE("claims_v0 checker: every outcome for every predicate (D-M2-15)", "[evaluation_pack][checker]") {
    const TruthBundle t = prefabTruth();
    // role_signal_present
    CHECK(outcomeOf(t, claim("w0000000a", "listening", "role_signal_present", role("trama"), true)) == "supported");
    CHECK(outcomeOf(t, claim("w0000000a", "listening", "role_signal_present", role("pulso"), true)) == "contradicted");
    CHECK(outcomeOf(t, claim("w0000000b", "listening", "role_signal_present", role("corpo"), true)) == "unverifiable");
    // role_onset_detected
    CHECK(outcomeOf(t, claim("w0000000a", "spectrogram", "role_onset_detected", role("trama"), true)) == "supported");
    CHECK(outcomeOf(t, claim("w0000000a", "spectrogram", "role_onset_detected", role("corpo"), true)) == "contradicted");
    CHECK(outcomeOf(t, claim("w0000000b", "spectrogram", "role_onset_detected", role("corpo"), false)) == "unverifiable");
    // role_audible: never supported; contradicted only by exact silence
    CHECK(outcomeOf(t, claim("w0000000a", "listening", "role_audible", role("pulso"), true)) == "contradicted");
    CHECK(outcomeOf(t, claim("w0000000a", "listening", "role_audible", role("vettore"), true)) == "unverifiable"); // below floor, not zero
    CHECK(outcomeOf(t, claim("w0000000a", "listening", "role_audible", role("corpo"), true)) == "unverifiable");   // below threshold
    CHECK(outcomeOf(t, claim("w0000000a", "listening", "role_audible", role("trama"), true)) == "compatible");
    CHECK(outcomeOf(t, claim("w0000000a", "listening", "role_audible", role("trama"), false)) == "unverifiable");  // maybe masked
    CHECK(outcomeOf(t, claim("w0000000a", "listening", "role_audible", role("pulso"), false)) == "compatible");
    CHECK(outcomeOf(t, claim("w0000000a", "code", "role_audible", role("trama"), true)) == "not_attributable");
    CHECK(outcomeOf(t, claim("w0000000b", "listening", "role_audible", role("corpo"), true)) == "unverifiable");   // unavailable
    // role_planned
    CHECK(outcomeOf(t, claim("w0000000a", "code", "role_planned", role("trama"), true)) == "supported");
    CHECK(outcomeOf(t, claim("w0000000a", "code", "role_planned", role("pulso"), true)) == "contradicted");
    CHECK(outcomeOf(t, claim("w0000000a", "listening", "role_planned", role("trama"), true)) == "not_attributable");
    // stereo_wider_than
    json other = {{"window_id_other", "w0000000a"}};
    CHECK(outcomeOf(t, claim("w0000000b", "spectrogram", "stereo_wider_than", other, true)) == "supported");
    CHECK(outcomeOf(t, claim("w0000000b", "spectrogram", "stereo_wider_than", other, false)) == "contradicted");
    CHECK(outcomeOf(t, claim("w0000000c", "spectrogram", "stereo_wider_than", other, true)) == "unverifiable");
    json mono = resultBase(t);
    mono["channel"]["audio_channels"] = 1;
    CHECK(outcomeOf(t, claim("w0000000b", "listening", "stereo_wider_than", other, true), mono) == "not_attributable");
    // artifact_present
    CHECK(outcomeOf(t, claim("w0000000b", "descriptor", "artifact_present", {{"kind", "clipping"}}, true)) == "supported");
    CHECK(outcomeOf(t, claim("w0000000a", "descriptor", "artifact_present", {{"kind", "clipping"}}, true)) == "contradicted");
    CHECK(outcomeOf(t, claim("w0000000a", "listening", "artifact_present", {{"kind", "click"}}, true)) == "unverifiable");
    json gemini = resultBase(t);
    gemini["channel"] = {{"audio_channels", 1}, {"bitrate_kbps", 16}, {"tokens_per_s", 32}};
    CHECK(outcomeOf(t, claim("w0000000a", "listening", "artifact_present", {{"kind", "click"}}, true), gemini) == "not_attributable");
    // observation, abstention
    CHECK(outcomeOf(t, claim("w0000000a", "listening", "observation", json::object(), nullptr)) == "unverifiable");
    CHECK(outcomeOf(t, claim("w0000000a", "listening", "role_signal_present", role("trama"), nullptr)) == "abstained");
    // inadmissible claims
    CHECK(outcomeOf(t, claim("w99999999", "listening", "role_signal_present", role("trama"), true)) == "inadmissible");
    CHECK(outcomeOf(t, claim("w0000000a", "intuition", "role_signal_present", role("trama"), true)) == "inadmissible");
    CHECK(outcomeOf(t, claim("w0000000a", "listening", "role_loud", role("trama"), true)) == "inadmissible");
    CHECK(outcomeOf(t, claim("w0000000a", "listening", "role_signal_present", role("basso"), true)) == "inadmissible");
    CHECK(outcomeOf(t, claim("w0000000a", "listening", "role_signal_present", role("trama"), "yes")) == "inadmissible");
    CHECK(outcomeOf(t, claim("w0000000b", "spectrogram", "stereo_wider_than", {{"window_id_other", "w0000000b"}}, true)) == "inadmissible");
    json noRefs = resultBase(t);
    noRefs["spectrogram_references_used"] = false;
    CHECK(outcomeOf(t, claim("w0000000a", "spectrogram", "role_onset_detected", role("trama"), true), noRefs) == "inadmissible");
    // A diagnostic window id does not exist in the blind mode
    CHECK(outcomeOf(t, claim("p00", "listening", "role_signal_present", role("trama"), true)) == "inadmissible");
}

TEST_CASE("claims_v0 checker: result-level admissibility and blind qualification (D-M2-15)", "[evaluation_pack][checker]") {
    const TruthBundle t = prefabTruth();
    auto run = [&](json r) {
        r["claims"] = json::array({claim(r["pack_mode"] == "diagnostic" ? "p00" : "w0000000a", "listening", "role_signal_present",
                                         role("trama"), true)});
        return checkEvaluation(t, r.dump(), kSealedHashes);
    };
    {
        const auto rep = run(resultBase(t));
        CHECK(rep.resultAdmissible);
        CHECK(rep.blindQualified);
        CHECK(rep.claims[0].outcome == "supported");
    }
    auto expectInadmissible = [&](json r, const char* what) {
        const auto rep = run(std::move(r));
        INFO(what);
        CHECK_FALSE(rep.resultAdmissible);
        CHECK_FALSE(rep.blindQualified);
        CHECK(rep.claims[0].outcome == "inadmissible");
    };
    json sealed = resultBase(t);
    sealed["input_hashes"].push_back(std::string(64, '9'));
    expectInadmissible(sealed, "sealed input");
    json diagAsBlind = resultBase(t);
    diagAsBlind["input_hashes"].push_back(t.publicDiagnosticFiles.at("pack.json"));
    expectInadmissible(diagAsBlind, "diagnostic file presented as blind");
    json unknown = resultBase(t);
    unknown["input_hashes"].push_back(std::string(64, '7'));
    expectInadmissible(unknown, "unknown input");
    json none = resultBase(t);
    none["input_hashes"] = json::array();
    expectInadmissible(none, "no input");
    json schema = resultBase(t);
    schema["schema"] = "evaluation_result_v1";
    expectInadmissible(schema, "schema");
    json declaredSealed = resultBase(t);
    declaredSealed["access"]["sealed"] = true;
    expectInadmissible(declaredSealed, "access to sealed");

    // Admissible but not blind: repository access, diagnostic pack, default seed (undeclared access: DIF-5d-03 below)
    json repo = resultBase(t);
    repo["access"]["repository"] = true;
    CHECK(run(repo).resultAdmissible);
    CHECK_FALSE(run(repo).blindQualified);
    const auto diag = run(resultBase(t, "diagnostic"));
    CHECK(diag.resultAdmissible);
    CHECK_FALSE(diag.blindQualified);
    TruthBundle defaultSeed = prefabTruth();
    defaultSeed.defaultSeed = true;
    json r = resultBase(t);
    r["claims"] = json::array();
    CHECK_FALSE(checkEvaluation(defaultSeed, r.dump(), kSealedHashes).blindQualified);
    // Not JSON: not checked
    CHECK_FALSE(checkEvaluation(t, "{not json", kSealedHashes).checked);
}

TEST_CASE("claims_v0 checker: catch trial, altered item, duplicates and the trivial baseline (D-M2-15)", "[evaluation_pack][checker]") {
    const TruthBundle t = prefabTruth();
    json r = resultBase(t);
    r["claims"] = json::array({
        claim("w0000000a", "listening", "role_signal_present", role("trama"), true),   // TP
        claim("w0000000a", "listening", "role_signal_present", role("pulso"), false),  // TN
        claim("w0000000d", "listening", "role_signal_present", role("trama"), true),   // duplicate agrees
        claim("w0000000d", "listening", "role_signal_present", role("pulso"), true),   // duplicate disagrees (FP)
        claim("w0000000e", "listening", "role_signal_present", role("trama"), true),   // altered: follows the plan
    });
    auto rep = checkEvaluation(t, r.dump(), kSealedHashes);
    CHECK(rep.alteredPlanFollowing == 1);
    CHECK(rep.duplicatePairsCompared == 2);
    CHECK(rep.duplicatePairsAgreeing == 1);
    CHECK_FALSE(rep.catchTrialFailed);
    CHECK(rep.presenceStatus == "MEASURED");
    // OSS-5d-01: the hidden duplicate is a consistency control, not an independent observation
    CHECK(rep.sensitivity.trials == 1); // w0000000a trama
    CHECK(rep.specificity.trials == 2); // w0000000a pulso, w0000000e trama
    CHECK_FALSE(rep.aboveTrivialBaseline); // a handful of claims never exceeds the trivial baseline

    // Catch trial failed: the presence statistics are excluded by the declared rule
    r["claims"].push_back(claim("w0000000f", "listening", "role_audible", role("corpo"), true));
    rep = checkEvaluation(t, r.dump(), kSealedHashes);
    CHECK(rep.catchTrialFailed);
    CHECK(rep.presenceStatus == "EXCLUDED_BY_CATCH_RULE");

    // Many correct claims exceed the baseline; Wilson interval on known values
    const auto ci = wilsonInterval(40, 40);
    CHECK(ci.low > 0.91);
    CHECK(ci.high == 1.0);
    const auto half = wilsonInterval(5, 10);
    CHECK(std::abs(half.low - 0.2366) < 1e-3);
    CHECK(std::abs(half.high - 0.7634) < 1e-3);
}

TEST_CASE("Sealed truth integrity is verified against sealed/SHA256SUMS before checking (D-M2-15)", "[evaluation_pack][checker]") {
    auto ps = plannerSource();
    const PackBuild p = buildEvaluationPack(ps->src, 77);
    REQUIRE(p.ok);
    const auto dir = std::filesystem::temp_directory_path() / "cymatica_eval_pack_test";
    std::filesystem::remove_all(dir);
    std::filesystem::create_directories(dir / "sealed");
    for (const auto& [name, bytes] : p.sealed) {
        std::ofstream(dir / "sealed" / name, std::ios::binary).write(reinterpret_cast<const char*>(bytes.data()),
                                                                     static_cast<std::streamsize>(bytes.size()));
    }
    // A result that declares the sealed truth as input is inadmissible
    json r = resultBase(p.truth);
    r["input_hashes"] = json::array({sha256Hex(std::span<const std::uint8_t>(p.sealed.at("evaluation_truth.json").data(),
                                                                              p.sealed.at("evaluation_truth.json").size()))});
    std::ofstream(dir / "result.json") << r.dump();
    const auto rep = runEvaluationCheck((dir / "result.json").string(), (dir / "sealed").string(), (dir / "report.json").string());
    INFO(rep.error);
    CHECK(rep.checked);
    CHECK_FALSE(rep.resultAdmissible);
    CHECK(std::filesystem::exists(dir / "report.json"));
    // Tampered truth: refused
    std::ofstream(dir / "sealed" / "evaluation_truth.json", std::ios::app) << " ";
    const auto tampered = runEvaluationCheck((dir / "result.json").string(), (dir / "sealed").string(), "");
    CHECK_FALSE(tampered.checked);
    CHECK(tampered.error.find("does not match") != std::string::npos);
    std::filesystem::remove_all(dir);
}

TEST_CASE("claims_v0 checker: repeated claims and undeclared inputs are inadmissible (AG-5d-01, AG-5d-02)", "[evaluation_pack][checker]") {
    const TruthBundle t = prefabTruth();
    // A repeated claim never overwrites the first one nor inflates the counts
    json r = resultBase(t);
    for (int k = 0; k < 10; ++k) r["claims"].push_back(claim("w0000000a", "listening", "role_signal_present", role("trama"), true));
    auto rep = checkEvaluation(t, r.dump(), kSealedHashes);
    CHECK(rep.outcomeCounts["supported"] == 1);
    CHECK(rep.outcomeCounts["inadmissible"] == 9);
    CHECK(rep.sensitivity.trials == 1);
    // Catch trial: a later false cannot cancel an earlier true
    r = resultBase(t);
    r["claims"].push_back(claim("w0000000f", "listening", "role_audible", role("corpo"), true));
    r["claims"].push_back(claim("w0000000f", "listening", "role_audible", role("corpo"), false));
    rep = checkEvaluation(t, r.dump(), kSealedHashes);
    CHECK(rep.catchTrialFailed);
    CHECK(rep.claims[1].outcome == "inadmissible");
    // Altered item: a later false cannot cancel the plan-following true
    r = resultBase(t);
    r["claims"].push_back(claim("w0000000e", "listening", "role_signal_present", role("trama"), true));
    r["claims"].push_back(claim("w0000000e", "listening", "role_signal_present", role("trama"), false));
    CHECK(checkEvaluation(t, r.dump(), kSealedHashes).alteredPlanFollowing == 1);
    // The same predicate with different arguments is not a repetition; observations may repeat
    r = resultBase(t);
    r["claims"].push_back(claim("w0000000a", "listening", "role_signal_present", role("trama"), true));
    r["claims"].push_back(claim("w0000000a", "listening", "role_signal_present", role("pulso"), false));
    r["claims"].push_back(claim("w0000000a", "listening", "observation", json::object(), nullptr));
    r["claims"].push_back(claim("w0000000a", "listening", "observation", json::object(), nullptr));
    rep = checkEvaluation(t, r.dump(), kSealedHashes);
    CHECK(rep.outcomeCounts["inadmissible"] == 0);

    // Inputs: only w0000000b and pack.json declared
    json partial = resultBase(t);
    partial["input_hashes"] = json::array({t.publicBlindFiles.at("pack.json"), t.publicBlindFiles.at("w0000000b.wav"),
                                           t.publicBlindFiles.at("w0000000b_mid.png")});
    CHECK(outcomeOf(t, claim("w0000000b", "listening", "role_signal_present", role("pulso"), true), partial) == "supported");
    CHECK(outcomeOf(t, claim("w0000000a", "listening", "role_signal_present", role("trama"), true), partial) == "inadmissible");
    CHECK(outcomeOf(t, claim("w0000000a", "descriptor", "artifact_present", {{"kind", "clipping"}}, false), partial) == "inadmissible");
    // Spectrogram: the window image is declared but no labelled reference is
    CHECK(outcomeOf(t, claim("w0000000b", "spectrogram", "role_onset_detected", role("pulso"), true), partial) == "inadmissible");
    partial["input_hashes"].push_back(t.publicBlindFiles.at("ref_trama_s0_mid.png"));
    CHECK(outcomeOf(t, claim("w0000000b", "spectrogram", "role_onset_detected", role("pulso"), true), partial) == "supported");
    // stereo_wider_than needs the other window's input too
    CHECK(outcomeOf(t, claim("w0000000b", "spectrogram", "stereo_wider_than", {{"window_id_other", "w0000000a"}}, true), partial) ==
          "inadmissible");
    // A claim from code needs no audio input
    CHECK(outcomeOf(t, claim("w0000000a", "code", "role_planned", role("trama"), true), partial) == "supported");
}

TEST_CASE("Unavailable measures are null, never zero (AG-5d-05)", "[evaluation_pack][checker]") {
    const TruthBundle t = prefabTruth();
    const json truth = json::parse(truthToJson(t));
    const json& corpo = truth.at("blind").at(1).at("roles").at("corpo"); // onset status NON_FINITE_INPUT
    CHECK(corpo.at("matched_onsets").is_null());
    CHECK(corpo.at("onset_detected").is_null());
    CHECK(corpo.at("peak_dbfs").is_null());
    const auto back = truthFromJson(truthToJson(t));
    REQUIRE(back.has_value());
    CHECK(truthToJson(*back) == truthToJson(t));
    // Excluded or insufficient presence scoring keeps the counts but no rate or interval
    json r = resultBase(t);
    r["claims"].push_back(claim("w0000000a", "listening", "role_signal_present", role("trama"), true));
    const json report = json::parse(checkEvaluation(t, r.dump(), kSealedHashes).toJson());
    const json& sens = report.at("role_signal_present_scoring").at("sensitivity");
    CHECK(report.at("role_signal_present_scoring").at("status") == "INSUFFICIENT_DATA");
    CHECK(sens.at("trials") == 1);
    CHECK(sens.at("value").is_null());
    CHECK(sens.at("wilson95_low").is_null());
}

TEST_CASE("claims_v0 checker: acoustic predicates are never supported from code (DIF-5d-02)", "[evaluation_pack][checker]") {
    const TruthBundle t = prefabTruth();
    // Only pack.json declared, access declared false, non-default seed: still no perceptual score from code
    json onlyPack = resultBase(t);
    onlyPack["input_hashes"] = json::array({t.publicBlindFiles.at("pack.json")});
    for (const char* predicate : {"role_signal_present", "role_onset_detected", "role_audible"}) {
        INFO(predicate);
        CHECK(outcomeOf(t, claim("w0000000a", "code", predicate, role("trama"), true), onlyPack) == "not_attributable");
        CHECK(outcomeOf(t, claim("w0000000a", "code", predicate, role("trama"), true)) == "not_attributable");
    }
    CHECK(outcomeOf(t, claim("w0000000b", "code", "stereo_wider_than", {{"window_id_other", "w0000000a"}}, true)) == "not_attributable");
    CHECK(outcomeOf(t, claim("w0000000b", "code", "artifact_present", {{"kind", "clipping"}}, true)) == "not_attributable");
    // Audibility from a spectrogram or a descriptor is not a perception of audibility
    CHECK(outcomeOf(t, claim("w0000000a", "spectrogram", "role_audible", role("trama"), true)) == "not_attributable");
    CHECK(outcomeOf(t, claim("w0000000a", "descriptor", "role_audible", role("trama"), true)) == "not_attributable");
    // role_planned keeps its diagnostic path from code
    CHECK(outcomeOf(t, claim("w0000000a", "code", "role_planned", role("trama"), true), onlyPack) == "supported");
    // A code claim never enters the presence sample; descriptors neither (not a perception)
    json r = resultBase(t);
    r["claims"].push_back(claim("w0000000a", "code", "role_signal_present", role("trama"), true));
    r["claims"].push_back(claim("w0000000c", "descriptor", "role_signal_present", role("trama"), true));
    const auto rep = checkEvaluation(t, r.dump(), kSealedHashes);
    CHECK(rep.sensitivity.trials == 0);
    CHECK(rep.claims[1].outcome == "supported");
    // Listening needs an audio channel; stereo by listening needs two
    json noAudio = resultBase(t);
    noAudio["channel"]["audio_channels"] = 0;
    CHECK(outcomeOf(t, claim("w0000000a", "listening", "role_signal_present", role("trama"), true), noAudio) == "not_attributable");
    CHECK(outcomeOf(t, claim("w0000000a", "spectrogram", "role_signal_present", role("trama"), true), noAudio) == "supported");
}

TEST_CASE("evaluation_result_v0 mandatory metadata are validated before any claim (DIF-5d-03)", "[evaluation_pack][checker]") {
    const TruthBundle t = prefabTruth();
    auto expectInadmissible = [&](const std::function<void(json&)>& mutate, const char* what) {
        json r = resultBase(t);
        r["claims"] = json::array({claim("w0000000a", "spectrogram", "role_signal_present", role("trama"), true)});
        mutate(r);
        const auto rep = checkEvaluation(t, r.dump(), kSealedHashes);
        INFO(what);
        CHECK(rep.checked);
        CHECK_FALSE(rep.resultAdmissible);
        CHECK_FALSE(rep.blindQualified);
        for (const auto& c : rep.claims) CHECK(c.outcome == "inadmissible");
    };
    expectInadmissible([](json& r) { r.erase("evaluator"); }, "no evaluator");
    expectInadmissible([](json& r) { r["evaluator"].erase("model"); }, "no model");
    expectInadmissible([](json& r) { r["evaluator"]["version"] = ""; }, "empty version");
    expectInadmissible([](json& r) { r.erase("timestamp"); }, "no timestamp");
    expectInadmissible([](json& r) { r.erase("channel"); }, "no channel");
    expectInadmissible([](json& r) { r["channel"].erase("audio_channels"); }, "unknown audio channels");
    expectInadmissible([](json& r) { r["channel"]["audio_channels"] = -1; }, "negative channels");
    expectInadmissible([](json& r) { r["channel"]["audio_channels"] = 2.5; }, "non-integer channels");
    expectInadmissible([](json& r) { r["channel"].erase("tokens_per_s"); }, "unknown token rate");
    expectInadmissible([](json& r) { r["channel"]["tokens_per_s"] = -32; }, "negative token rate");
    expectInadmissible([](json& r) { r.erase("access"); }, "no access");
    expectInadmissible([](json& r) { r["access"].erase("code_execution"); }, "access flag missing");
    expectInadmissible([](json& r) { r["access"]["repository"] = "no"; }, "access flag not boolean");
    expectInadmissible([](json& r) { r.erase("spectrogram_references_used"); }, "references flag missing");
    {
        json r = resultBase(t);
        r.erase("claims");
        const auto rep = checkEvaluation(t, r.dump(), kSealedHashes);
        CHECK_FALSE(rep.resultAdmissible);
    }
    // The complete result stays admissible and blind-qualified
    json ok = resultBase(t);
    const auto rep = checkEvaluation(t, ok.dump(), kSealedHashes);
    CHECK(rep.resultAdmissible);
    CHECK(rep.blindQualified);
}

TEST_CASE("Pack output directories are isolated and never removed blindly (DIF-5d-01)", "[evaluation_pack][dirs]") {
    namespace fs = std::filesystem;
    const fs::path base = fs::temp_directory_path() / "cymatica_pack_dirs_test";
    fs::remove_all(base);
    fs::create_directories(base);
    const std::string root = (base / "pack").string();
    const std::string blind = (base / "pack" / "public_blind").string();
    const std::string diag = (base / "pack" / "public_diagnostic").string();
    const std::string sealed = (base / "sealed").string();
    CHECK(validatePackDirectories(blind, diag, sealed, root).empty());
    // Conflicts: equality, sealed inside a public dir or the root, sealed ancestor of the root, public dirs nested
    CHECK_FALSE(validatePackDirectories(blind, diag, blind, root).empty());
    CHECK_FALSE(validatePackDirectories(blind, diag, diag, root).empty());
    CHECK_FALSE(validatePackDirectories(blind, diag, root, root).empty());
    CHECK_FALSE(validatePackDirectories(blind, diag, (base / "pack" / "public_blind" / "s").string(), root).empty());
    CHECK_FALSE(validatePackDirectories(blind, diag, (base / "pack" / "other").string(), root).empty());
    CHECK_FALSE(validatePackDirectories(blind, diag, base.string(), root).empty());
    CHECK_FALSE(validatePackDirectories(blind, blind, sealed, root).empty());
    CHECK_FALSE(validatePackDirectories(blind, (base / "pack" / "public_blind" / "d").string(), sealed, root).empty());
    CHECK_FALSE(validatePackDirectories(blind, diag, (base / "pack" / "public_blind" / "..").string(), root).empty());
    CHECK_FALSE(validatePackDirectories(blind, diag, "", root).empty());
    // A symbolic link that resolves into a public dir is detected (skipped where links cannot be created)
    fs::create_directories(blind);
    std::error_code ec;
    fs::create_directory_symlink(blind, base / "link_to_blind", ec);
    if (!ec) {
        CHECK_FALSE(validatePackDirectories(blind, diag, (base / "link_to_blind").string(), root).empty());
        CHECK_FALSE(writePackDirectory((base / "link_to_blind").string(), {{"a.txt", {1}}}).empty());
    }

    // writePackDirectory: a foreign non-empty directory is refused and left untouched
    const fs::path foreign = base / "foreign";
    fs::create_directories(foreign);
    std::ofstream(foreign / "keep.txt") << "user data";
    std::map<std::string, std::vector<std::uint8_t>> files{{"a.bin", {1, 2, 3}}};
    files["SHA256SUMS"] = {};
    {
        std::map<std::string, std::vector<std::uint8_t>> listed{{"a.bin", {1, 2, 3}}};
        const std::string sums = sha256Hex(std::span<const std::uint8_t>(listed["a.bin"].data(), 3)) + "  a.bin\n";
        files["SHA256SUMS"] = std::vector<std::uint8_t>(sums.begin(), sums.end());
    }
    CHECK_FALSE(writePackDirectory(foreign.string(), files).empty());
    CHECK(fs::exists(foreign / "keep.txt"));
    // A fresh directory is written and verified; rewriting a directory it owns replaces only its files
    const fs::path own = base / "own";
    CHECK(writePackDirectory(own.string(), files).empty());
    CHECK(writePackDirectory(own.string(), files).empty());
    // A stray file in an owned directory makes it foreign again
    std::ofstream(own / "stray.txt") << "x";
    CHECK_FALSE(writePackDirectory(own.string(), files).empty());
    CHECK(fs::exists(own / "stray.txt"));
    // A regular file in place of the directory is refused
    std::ofstream(base / "plainfile") << "x";
    CHECK_FALSE(writePackDirectory((base / "plainfile").string(), files).empty());
    fs::remove_all(base);
}
