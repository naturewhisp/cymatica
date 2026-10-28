#include "stereo.h"

#include <cmath>

namespace cymatica::cacp {

SideMidResult sideMid(std::span<const float> interleavedStereo) noexcept {
    SideMidResult r;
    if (interleavedStereo.empty() || interleavedStereo.size() % 2 != 0) {
        r.status = AnalysisStatus::InvalidConfig;
        return r;
    }
    if (!allFinite(interleavedStereo)) {
        r.status = AnalysisStatus::NonFiniteInput;
        return r;
    }
    double side = 0.0;
    double mid = 0.0;
    for (std::size_t i = 0; i + 1 < interleavedStereo.size(); i += 2) {
        const double l = interleavedStereo[i];
        const double rr = interleavedStereo[i + 1];
        const double s = 0.5 * (l - rr);
        const double m = 0.5 * (l + rr);
        side += s * s;
        mid += m * m;
    }
    r.sideIsZero = !(side > 0.0);
    r.midIsZero = !(mid > 0.0);
    if (r.sideIsZero && r.midIsZero) {
        r.status = AnalysisStatus::ExactSilence;
        return r;
    }
    if (r.sideIsZero || r.midIsZero) return r; // Ok, value undefined (-inf or +inf), flags say which
    const double db = 10.0 * std::log10(side / mid);
    if (std::isfinite(db)) r.db = db;
    return r;
}

std::optional<double> sideMidDb(std::span<const float> interleavedStereo) noexcept {
    const SideMidResult r = sideMid(interleavedStereo);
    return r.status == AnalysisStatus::Ok ? r.db : std::nullopt;
}

} // namespace cymatica::cacp
