#include "stereo.h"

#include <cmath>

namespace cymatica::cacp {

std::optional<double> sideMidDb(std::span<const float> interleavedStereo) noexcept {
    double side = 0.0;
    double mid = 0.0;
    for (std::size_t i = 0; i + 1 < interleavedStereo.size(); i += 2) {
        const double l = interleavedStereo[i];
        const double r = interleavedStereo[i + 1];
        const double s = 0.5 * (l - r);
        const double m = 0.5 * (l + r);
        side += s * s;
        mid += m * m;
    }
    if (!(side > 0.0) || !(mid > 0.0)) return std::nullopt;
    const double db = 10.0 * std::log10(side / mid);
    if (!std::isfinite(db)) return std::nullopt;
    return db;
}

} // namespace cymatica::cacp
