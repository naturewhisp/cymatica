#pragma once

#include <string>

namespace cymatica::cacp {

// Scenario warped_plan_render_v1 (CP-3 I5, ADR-0007 D-M2-51; pre-registration
// docs/research/M2_5_CP3_I5_Tempo_Ramp_Prototype_Preregistration.md). Offline prototype tool: renders through the real
// engine every <outputDir>/tempo_ramp_v1/<name>.csv (id,role,frame,duration_frames,pitch_hz,velocity,timbre,noteoff_frame;
// noteoff_frame is empty except for open pads), written by tempo_ramp.py from the plan with the event times moved.
// MusicClock and the game are untouched. Returns the manifest JSON, empty on failure. MEASURED, never a gate.
[[nodiscard]] std::string runWarpedPlanRenderV1(const std::string& outputDir = "artifacts/cacp");

} // namespace cymatica::cacp
