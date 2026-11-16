#pragma once

#include "audio_probe.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace cymatica::cacp {

// Scenario kick_level_ab_v1 (M2.5.2 F4a, ADR-0007 D-M2-19/D-M2-26): controlled experiment on the level of the kick.
// Only the Pulso bus gain varies, through an offline-only override; the ducking reference follows the effective
// Pulso gain, so only the kick level changes. Production keeps kPulsoGain (-6 dB) until the CP-2 listening.
// Predictions P1-P7 and their outcome rules are pre-registered in
// docs/research/M2_5_2_F4_Kick_Level_Preregistration.md (SHA-256 recorded in progress.md before the first run).
// MEASURED, never a gate.

inline constexpr std::array<double, 5> kKickOffsetsDb{0.0, 3.0, 6.0, 9.0, 12.0};
// P5: mix loudness rise predicted for +3, +6, +9, +12 dB (power sum of uncorrelated signals, p = 0.0474)
inline constexpr std::array<double, 4> kP5PredictedRiseLu{0.20, 0.57, 1.24, 2.32};
inline constexpr double kP5ToleranceLu = 0.3;
inline constexpr double kP4ToleranceLu = 0.05;
inline constexpr double kP3StemTolerance = 1e-6;
inline constexpr double kP3BusTolerance = 2e-6;
inline constexpr double kP6MaxPredictedOffsetDb = 9.0;   // limiter reduction 0 dB predicted up to +9 dB
// Full piece written for the CP-2 listening (free description, scales, Q3 per section): the ladder midpoint, chosen
// before listening as working material, not as the candidate value (the level is decided by the section rankings)
inline constexpr double kFullPieceOffsetDb = 6.0;
// Same golden as the fixture (the scenario's offset 0 is the current production)
inline constexpr std::uint64_t kFixtureGoldenPcm = audio::kFixtureProbeV1GoldenPcmHash;

// Linear scale of an offset: exactly 1 for 0 dB (production path, no override)
[[nodiscard]] float kickGainScale(double offsetDb) noexcept;

struct KickNullTest {
    std::array<double, 4> stemMaxAbsResidual{};  // pulso vs k * pulso_0; corpo, trama, vettore vs offset 0
    std::array<std::size_t, 4> stemWorstSample{};
    double busMaxAbsResidual{0.0};               // bus - (bus_0 + (k - 1) * pulso_0), before the master
    std::size_t busWorstSample{0};
    bool computed{false};
};

struct KickVariantMeasure {
    double offsetDb{0.0};
    float scale{1.0f};
    std::string captureInvalid{};                // empty when the capture is valid
    std::string goldenPcmStatus{"NOT_RUN"};      // PASS / FAIL for offset 0, NOT_APPLICABLE with an override
    std::uint64_t pcmContentHash{0};
    std::uint64_t eventsHash{0};
    std::optional<double> mixLufs{};             // final PCM, integrated
    std::optional<double> busLufs{};             // bus mix before the master
    std::array<std::optional<double>, 4> stemLufs{};
    std::vector<std::optional<double>> sectionMixLufs{};   // A, B, A'
    std::vector<std::optional<double>> sectionPulsoLufs{};
    double truePeakDbtp{-120.0};
    double maxGainReductionDb{0.0};
    KickNullTest nullTest{};
};

struct KickOutcome {
    std::string outcome{"NOT_RUN"};              // SUPPORTED, NOT_SUPPORTED, NO_PREDICTION, INCONCLUSIVE
    std::string detail{};
};

struct KickLevelAbResult {
    std::string status{"NOT_RUN"};               // MEASURED, INVALID (P1 or P2 failed) or INCONCLUSIVE
    std::vector<std::string> reasons{};
    std::vector<KickVariantMeasure> measures{};
    KickOutcome p1{}, p2{}, p7{};
    std::vector<KickOutcome> p3{}, p4{}, p5{}, p6{};   // one per offset > 0, in kKickOffsetsDb order
    std::vector<std::string> limiterConfounded{};      // offsets with a limiter reduction > 0
    std::vector<std::string> clips{};
    bool repeatIdentical{false};
    std::string manifestFilePath{};
    bool manifestWriteSuccess{false};
    [[nodiscard]] std::string measuresJson() const;  // deterministic part (compared on repeat)
    [[nodiscard]] std::string toJson() const;
};

// Null test of a variant against the offset-0 stems and bus (interleaved stereo, same length); computed is false on
// a length mismatch
[[nodiscard]] KickNullTest kickNullTest(float scale, const std::array<std::span<const float>, 4>& stems0,
                                        std::span<const float> bus0, const std::array<std::span<const float>, 4>& stems,
                                        std::span<const float> bus);

// Pre-registered decision rules on recorded measures (pure, testable). measures[0] must be the offset-0 variant.
[[nodiscard]] KickOutcome evaluateKickP1(const std::vector<KickVariantMeasure>& measures);
[[nodiscard]] KickOutcome evaluateKickP2(const std::vector<KickVariantMeasure>& measures);
[[nodiscard]] KickOutcome evaluateKickP3(const KickVariantMeasure& m);
[[nodiscard]] KickOutcome evaluateKickP4(const KickVariantMeasure& base, const KickVariantMeasure& m);
[[nodiscard]] KickOutcome evaluateKickP5(const KickVariantMeasure& base, const KickVariantMeasure& m, double predictedRiseLu);
[[nodiscard]] KickOutcome evaluateKickP6(const KickVariantMeasure& m);

// Scalar gain that brings a clip to a target integrated loudness; nullopt when either loudness is undefined
[[nodiscard]] std::optional<double> loudnessAlignGain(std::optional<double> clipLufs, std::optional<double> targetLufs) noexcept;

[[nodiscard]] KickLevelAbResult runKickLevelAbV1(const std::string& outputDir = "artifacts/cacp", bool writeFiles = true);

} // namespace cymatica::cacp
