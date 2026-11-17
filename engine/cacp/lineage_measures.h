#pragma once

#include <string>

namespace cymatica::cacp {

// Scenario lineage_measures_v1 (M2.5.3-4 contract point 5, D-M2-45; pre-registration
// docs/research/M2_5_3_C5_Derived_Variation_Preregistration.md): criteria K3 (develops), K4 (remembers) and K5 (returns)
// on the Vettore role of fixture-m2-4-v1 (seed 1337), with the admitted M5r measure of measurement package 6 and the
// declared parents of the decision traces. MEASURED, never a gate, never part of the planner score. Writes
// <outputDir>/lineage_measures_v1_manifest.json and returns its JSON (empty on failure).
[[nodiscard]] std::string runLineageMeasuresV1(const std::string& outputDir = "artifacts/cacp");

} // namespace cymatica::cacp
