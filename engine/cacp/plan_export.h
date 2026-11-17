#pragma once

#include <string>

namespace cymatica::cacp {

// Scenario plan_export_v1: deterministic CSV of the fixture-m2-4-v1 intents (seed 1337), in the format of
// docs/research/experiments/cacp_methods/fixture_intents_seed1337.csv plus the timbre column, and the explicit NoteOffs
// of open pads as '#noteoff,<event_id>,<role>,<frame>' rows. Used to compare plans before and after a planner change.
// Returns the path written, empty on failure.
[[nodiscard]] std::string runPlanExportV1(const std::string& outputDir = "artifacts/cacp");

} // namespace cymatica::cacp
