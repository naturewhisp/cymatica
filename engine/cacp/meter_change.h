#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace cymatica::cacp {

// Scenario meter_change_v1 (ADR-0007 D-M2-21, specification 0.8.5 §32; pre-registration
// docs/research/M2_5_3_Meter_Change_Scenario_Preregistration.md): technical qualification of a time-signature change
// via SetTempoMap, distinct from the musical fixture. 24 bars, A 4/4, B 3/4 with a 12-step motif, A' 4/4, 120 BPM,
// seed 1, offline chain at 48 kHz. Binding checks M1-M6; verdict PASS only when all pass.

// Golden of the scenario PCM (FNV-1a of the float samples), recorded at the first run on 2026-11-17.
// History: 4922831055463163399 -> 58770584194502935 (M2.5.3-4 F3: the 3/4 section's first phrase keeps the
// Pulso/Trama patterns of the previous phrase; every binding check M1-M6 still passes) -> 9026598872691749635
// (M2.5.3-4 contract point 1, C1c: Corpo dyad repaired after selection; D-M2-41) -> 16485786595596680340
// (M2.5.3-4 contract point 2: Vettore reads the chord; D-M2-42) -> 16313085494776772472 (M2.5.3-4 contract point 3:
// dominant register; D-M2-43).
inline constexpr std::uint64_t kMeterChangeGoldenPcmHash = 16313085494776772472ULL;

struct MeterChangeResult {
    std::string verdict{"NOT_RUN"};             // PASS or FAIL
    std::vector<std::string> failures{};
    std::uint64_t bar8Frame{0}, bar16Frame{0}, bar24Frame{0};                     // M1
    std::string tempoAck1{}, tempoAck2{}, stopAck{};                               // M2
    std::uint64_t tempoAck1Frame{0}, tempoAck2Frame{0}, stopAckFrame{0};
    std::uint64_t droppedEvents{0}, stagedAtEnd{0}, dispatched{0}, generated{0};    // M3
    std::size_t phrasesPlanned{0};
    std::size_t fallbacks{0}, invalidCandidates{0}, offGridMiddle{0}, corpoDownbeatsMissing{0};  // M4
    bool finite{false};                                                            // M5
    double energyA{0.0}, energyB{0.0}, energyA2{0.0};
    std::uint64_t planHash{0}, pcmHash{0};                                         // M6
    bool repeatIdentical{false};
    std::string goldenPcmStatus{"NOT_APPLICABLE"};
    std::string manifestFilePath{};
    bool manifestWriteSuccess{false};
    [[nodiscard]] std::string toJson() const;
};

[[nodiscard]] MeterChangeResult runMeterChangeV1(const std::string& outputDir = "artifacts/cacp", bool writeFiles = true);

} // namespace cymatica::cacp
