#pragma once

#include "device_loopback_tap.h"

namespace cymatica::audio {

// Master volume of the default render endpoint, read only (CP-1, R-M25-12). Windows Core Audio; on other platforms
// the reading is invalid. Never called from the audio callback.
[[nodiscard]] EndpointVolumeReading readDefaultEndpointVolume() noexcept;

// Audio sessions of the current process on the default render endpoint (CP-1, H2), read only; at most 4
[[nodiscard]] SessionVolumeReading readProcessSessionVolumes() noexcept;

} // namespace cymatica::audio
