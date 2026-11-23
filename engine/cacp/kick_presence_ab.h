#pragma once

#include <string>

namespace cymatica::cacp {

// Scenario kick_presence_ab_v1 (CP-3 I1-I2, ADR-0007 D-M2-51; pre-registration
// docs/research/M2_5_CP3_I1_I2_Kick_Preregistration.md): fixture-m2-4-v1 (seed 1337) rendered with a fixed Pulso pitch
// (five MIDI notes) and with a scaled Pulso click (+3, +6, +9 dB), offline overrides only. Writes the stems and the mix
// per variant and <outputDir>/kick_presence_ab_v1/kick_presence_ab_v1_manifest.json, and returns its JSON (empty on
// failure). MEASURED, never a gate; the game keeps the production kick.
[[nodiscard]] std::string runKickPresenceAbV1(const std::string& outputDir = "artifacts/cacp");

} // namespace cymatica::cacp
