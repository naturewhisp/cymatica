#pragma once

#include <string>

namespace cymatica::cacp {

// Scenario form_investigation_v1 (CP-3 I4, ADR-0007 D-M2-51; pre-registration
// docs/research/M2_5_CP3_I4_Form_Investigation_Preregistration.md): exports the plan tokens (sixteenths) of the reference
// fixture (seed 1337) and of the procedural paths of seeds 1..64 as CSV
// (plan,role,onset,midi,duration_class; plan "ref" or the seed). No render. Returns the CSV path, empty on failure.
[[nodiscard]] std::string runFormInvestigationV1(const std::string& outputDir = "artifacts/cacp");

} // namespace cymatica::cacp
