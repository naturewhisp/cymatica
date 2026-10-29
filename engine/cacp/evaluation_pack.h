#pragma once

#include "analysis_status.h"
#include "event_chain_log.h"
#include "music_intent.h"
#include "stem_analysis.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <map>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace cymatica::audio {
struct PlannedNoteOff;
}

namespace cymatica::cacp {

// evaluation_pack_v1, evaluation_result_v0 and the claims_v0 checker (ADR-0006 D-M2-15, Inc-5 phase 5d).
// Offline only. The pack has a public part per mode (blind, diagnostic) handed to an evaluator and a sealed
// part (ground truth, key, selection seed) used only by the checker. The checker outcome is reported apart
// from the gates and never decides a verdict (D-M2-11).

inline constexpr const char* kEvaluationPackVersion = "evaluation_pack_v1";
inline constexpr const char* kEvaluationResultSchema = "evaluation_result_v0";
inline constexpr const char* kClaimsVersion = "claims_v0";
inline constexpr std::uint64_t kDefaultPackSeed = 20261029ULL; // public: never valid for a blind session

// role_signal_present base of truth and the numerical floor of the demonstrated absence (D-M2-15, 5d values)
struct PresenceProfile {
    const char* version{"presence_profile_v1"};
    double absoluteRmsDbfs{-60.0};   // stem RMS in the window (dB re 1.0, both channels)
    double stemToMixDb{-30.0};       // stem energy / delivered mix energy in the window
    double numericalFloorDbfs{-120.0};
};
[[nodiscard]] PresenceProfile presenceProfileV1() noexcept;

// Level of a role signal in a window. Only ExactSilence (every sample 0) is a demonstrated absence.
enum class SignalLevel : std::uint8_t { Unavailable, ExactSilence, BelowNumericalFloor, BelowThreshold, Present };
[[nodiscard]] const char* signalLevelName(SignalLevel s) noexcept;
[[nodiscard]] std::optional<SignalLevel> signalLevelFromName(std::string_view name) noexcept;

// Parameters fixed for 5d (ADR-0006 D-M2-15, 5d entry precisions)
struct PackConfig {
    double blindWindowS{6.0};          // common blind excerpt from the phrase start, <= shortest phrase (6.857 s)
    double fadeS{0.010};               // raised-cosine fade in and out of every delivered window
    double duplicateGainDb{-0.1};      // hidden duplicate: gain change ...
    double duplicateShiftS{0.007};     // ... and excerpt start shifted (not a multiple of the hop): PNGs differ too
    std::size_t windowsPerMaskGroup{2}; // test windows per (section, planned role mask)
    double stereoMarginDb{1.0};        // stereo_wider_than: |difference| below the margin is unverifiable
    double onsetToleranceS{0.050};     // role_onset_detected: detection matched to an applied NoteOn
    double clickResolutionS{0.010};    // artifact_present(click) needs a channel resolution at least this fine
};

// Interval in which a role sounds: [startFrame, endFrame)
struct RoleSpan {
    audio::MusicRole role{audio::MusicRole::Pulso};
    std::uint64_t startFrame{0};
    std::uint64_t endFrame{0};
};
// Planned spans: intent start to its planner NoteOff when it has one (open pad), else start + duration
[[nodiscard]] std::vector<RoleSpan> plannedSpans(std::span<const audio::MusicIntentEvent> intents,
                                                 std::span<const audio::PlannedNoteOff> noteOffs);
// Applied spans: Applied NoteOn to the Applied NoteOff of the same event (or endFrame when none)
[[nodiscard]] std::vector<RoleSpan> appliedSpans(std::span<const AppliedEventRecord> applied, std::uint64_t endFrame);

struct PackPhrase {
    std::uint32_t index{0};
    std::uint32_t section{0};
    std::uint32_t startBar{0};
    std::uint32_t barCount{0};
    std::uint64_t startFrame{0};
    std::uint64_t endFrame{0};
};

// Everything the pack is built from, in memory (render of a scenario with stems and event logs)
struct PackSource {
    std::string name;
    std::uint32_t sampleRate{48000};
    std::array<std::span<const float>, kRoleCount> stems{};  // interleaved stereo, frame-aligned with the render
    std::vector<AppliedEventRecord> applied{};
    std::vector<RoleSpan> planned{};
    std::vector<PackPhrase> phrases{};
    std::array<RoleOnsetProfile, kRoleCount> onsetProfile{};  // frozen stem_onset_profile_v1
    std::string onsetProfileSha256{};
};

// Ground truth of one role in one delivered window (the four truth levels of D-M2-15, kept distinct)
struct RoleWindowTruth {
    bool planned{false};
    bool applied{false};
    SignalLevel signal{SignalLevel::Unavailable};
    double peakDbfs{std::numeric_limits<double>::quiet_NaN()};
    double rmsDbfs{std::numeric_limits<double>::quiet_NaN()};
    double stemToMixDb{std::numeric_limits<double>::quiet_NaN()};
    std::string onsetStatus{"NOT_RUN"};   // MEASURED or the detector status
    std::size_t appliedNoteOns{0};        // applied NoteOn references starting in the window
    std::size_t matchedOnsets{0};         // of those, matched by a detection within the tolerance
    bool onsetDetected{false};
};

struct WindowTruth {
    std::string id;
    std::string kind{"test"};             // test, duplicate, catch, altered
    std::string duplicateOf{};
    int removedRole{-1};                   // altered: role removed from the audio
    int phrase{-1};                        // -1 for the catch trial (synthetic silence)
    std::uint32_t section{0};
    std::uint32_t startBar{0};
    std::uint32_t barCount{0};
    std::uint64_t startFrame{0};
    std::uint64_t endFrame{0};
    double gainDb{0.0};
    std::array<RoleWindowTruth, kRoleCount> roles{};
    std::string sideMidStatus{"NOT_RUN"};
    std::optional<double> sideMidDb{};
    bool sideIsZero{false};
    bool midIsZero{false};
    std::uint64_t clippedSamples{0};
    std::string melMidStatus{"NOT_RUN"};
    std::string melSideStatus{"NOT_RUN"};
};

// What is delivered for one window: frames [start, end), included roles, gain and fades
struct WindowRender {
    std::uint64_t startFrame{0};
    std::uint64_t endFrame{0};
    std::array<bool, kRoleCount> include{true, true, true, true};
    double gainDb{0.0};
    double fadeS{0.010};
};

// Detector run once per role on the full stem (frozen profile), with the matching against the applied NoteOns
struct OnsetIndex {
    std::array<std::string, kRoleCount> status{};
    std::array<std::vector<double>, kRoleCount> referencesS{};
    std::array<std::vector<bool>, kRoleCount> referenceMatched{};
};
[[nodiscard]] OnsetIndex buildOnsetIndex(const PackSource& src, double toleranceS);

// Measures the truth of a delivered window and optionally returns its audio (interleaved stereo)
[[nodiscard]] WindowTruth measureWindow(const PackSource& src, const OnsetIndex& onsets, const WindowRender& w,
                                        const PresenceProfile& profile, std::vector<float>* audioOut = nullptr);

// Sealed truth of a pack, as read by the checker
struct TruthBundle {
    std::string packVersion{kEvaluationPackVersion};
    std::string sourceName{};
    std::uint64_t packSeed{0};
    std::string seedSha256{};
    bool defaultSeed{false};
    PresenceProfile profile{};
    PackConfig config{};
    std::string onsetProfileSha256{};
    std::vector<WindowTruth> blind{};
    std::vector<WindowTruth> diagnostic{};
    std::vector<WindowTruth> phrases{};    // blind excerpt of every phrase: presence calibration evidence
    bool alteredMaskNovel{false};          // altered item leaves a role set that no phrase has (AG-5d-03)
    std::map<std::string, std::string> publicBlindFiles{};       // file name -> SHA-256
    std::map<std::string, std::string> publicDiagnosticFiles{};
};
[[nodiscard]] std::string truthToJson(const TruthBundle& t);
[[nodiscard]] std::optional<TruthBundle> truthFromJson(std::string_view json, std::string* error = nullptr);
// Commitment published in the blind pack instead of the seed
[[nodiscard]] std::string packSeedCommitment(std::uint64_t seed);

struct PackBuild {
    bool ok{false};
    std::vector<std::string> errors{};
    std::map<std::string, std::vector<std::uint8_t>> publicBlind{};
    std::map<std::string, std::vector<std::uint8_t>> publicDiagnostic{};
    std::map<std::string, std::vector<std::uint8_t>> sealed{};
    TruthBundle truth{};
    std::size_t references{0};
    std::size_t phrasesMeasured{0};
};
// Deterministic for a given source, seed, configuration and profile
[[nodiscard]] PackBuild buildEvaluationPack(const PackSource& src, std::uint64_t packSeed, const PackConfig& cfg = {},
                                            const PresenceProfile& profile = presenceProfileV1());

// 16-bit PCM stereo WAV with only the RIFF, fmt and data chunks (same quantization as writeWavStereo16)
[[nodiscard]] std::vector<std::uint8_t> encodeWavStereo16(std::span<const float> interleaved, std::uint32_t sampleRate);

// ---- claims_v0 checker ----

struct ClaimOutcome {
    std::size_t index{0};
    std::string windowId{};
    std::string method{};
    std::string predicate{};
    std::string outcome{};   // supported, compatible, contradicted, not_attributable, inadmissible, unverifiable, abstained
    std::string reason{};
};

struct ProportionCi {
    std::size_t successes{0};
    std::size_t trials{0};
    double value{std::numeric_limits<double>::quiet_NaN()};
    double low{std::numeric_limits<double>::quiet_NaN()};
    double high{std::numeric_limits<double>::quiet_NaN()};
};
// Wilson score interval, z = 1.96 (95 %)
[[nodiscard]] ProportionCi wilsonInterval(std::size_t successes, std::size_t trials) noexcept;

struct CheckReport {
    bool checked{false};
    std::string error{};
    std::string packMode{};
    bool resultAdmissible{false};
    std::vector<std::string> inadmissibleReasons{};
    bool blindQualified{false};
    std::vector<std::string> blindNotQualifiedReasons{};
    std::vector<ClaimOutcome> claims{};
    std::map<std::string, std::size_t> outcomeCounts{};
    // Controls (D-M2-15, 5d precisions)
    bool catchTrialFailed{false};
    std::size_t alteredPlanFollowing{0};   // true claims on the removed role of an altered item
    std::size_t duplicatePairsCompared{0};
    std::size_t duplicatePairsAgreeing{0};
    // role_signal_present against the measured truth, test/duplicate/altered windows (catch excluded)
    std::string presenceStatus{"NOT_RUN"}; // MEASURED, INSUFFICIENT_DATA or EXCLUDED_BY_CATCH_RULE
    ProportionCi sensitivity{};
    ProportionCi specificity{};
    bool aboveTrivialBaseline{false};

    [[nodiscard]] std::string toJson() const;
};

// sealedFileHashes: SHA-256 of the sealed files, so that a result declaring one of them is inadmissible
[[nodiscard]] CheckReport checkEvaluation(const TruthBundle& truth, std::string_view resultJson,
                                          const std::vector<std::string>& sealedFileHashes);

// ---- scenario ----

struct EvaluationPackRun {
    bool ok{false};
    std::string status{"NOT_RUN"};
    std::vector<std::string> errors{};
    std::uint64_t packSeed{0};
    bool defaultSeed{false};
    std::string captureInvalid{};
    std::size_t blindWindows{0};
    std::size_t diagnosticWindows{0};
    std::size_t references{0};
    std::size_t publicBlindFiles{0};
    std::size_t publicDiagnosticFiles{0};
    bool repeatIdentical{false};
    std::string publicBlindDir{};
    std::string publicDiagnosticDir{};
    std::string sealedDir{};
    std::string manifestFilePath{};
    bool manifestWriteSuccess{false};

    [[nodiscard]] std::string toJson() const;
};

// Renders fixture_probe_v1 (seed 1337) with stems, builds the pack, writes public_blind/ and
// public_diagnostic/ under publicRoot and the sealed part under sealedDir (outside artifacts/cacp by default),
// then renders and builds again and checks that every file hash is identical.
[[nodiscard]] EvaluationPackRun runEvaluationPackV1(const std::string& publicRoot, const std::string& sealedDir,
                                                    std::uint64_t packSeed);

// Reads the sealed truth (verified against sealed/SHA256SUMS) and a result file, checks it, writes the report
[[nodiscard]] CheckReport runEvaluationCheck(const std::string& resultPath, const std::string& sealedDir,
                                             const std::string& reportPath);

} // namespace cymatica::cacp
