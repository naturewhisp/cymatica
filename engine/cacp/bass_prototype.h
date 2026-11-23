#pragma once

#include <string>

namespace cymatica::cacp {

// Scenario bass_prototype_v1 (CP-3 I3, ADR-0007 D-M2-51; pre-registration
// docs/research/M2_5_CP3_I3_Bass_Prototype_Preregistration.md). Offline only, no fifth role in the engine. Renders
// fixture-m2-4-v1 (seed 1337) as V0 (no bass) and V1 (Corpo bass voice one octave down), each with the production kick
// and with the candidate kick (C2, click +3 dB), and the V2 bass lines read from <outputDir>/bass_prototype_v1/
// bass_<kick>_<s0|s1>.csv (time_s,pitch_hz,velocity,duration_s,timbre, written by bass_prototype.py) through a separate
// engine with the Corpo voice. Returns the manifest JSON, empty on failure. MEASURED, never a gate.
[[nodiscard]] std::string runBassPrototypeV1(const std::string& outputDir = "artifacts/cacp");

} // namespace cymatica::cacp
