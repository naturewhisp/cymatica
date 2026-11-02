#include "staff_view.h"

#include "onset.h"
#include "png_writer.h"
#include "stft.h"

#include <algorithm>
#include <cmath>

namespace cymatica::cacp {

namespace {

constexpr std::uint8_t kBackground = 0;
constexpr std::uint8_t kLane = 22;
constexpr std::uint8_t kSeparatorGrey = 70;
constexpr std::uint8_t kBarGrey = 55;
constexpr std::size_t kNoteThickness = 4;

struct Canvas {
    std::size_t width;
    std::size_t height;
    std::vector<std::uint8_t>& rgb;

    void set(std::size_t x, std::size_t y, const std::array<std::uint8_t, 3>& c) {
        if (x >= width || y >= height) return;
        const std::size_t o = (y * width + x) * 3;
        rgb[o] = c[0];
        rgb[o + 1] = c[1];
        rgb[o + 2] = c[2];
    }
    void fillRect(std::size_t x0, std::size_t y0, std::size_t x1, std::size_t y1, const std::array<std::uint8_t, 3>& c) {
        for (std::size_t y = y0; y < y1 && y < height; ++y) {
            for (std::size_t x = x0; x < x1 && x < width; ++x) set(x, y, c);
        }
    }
};

} // namespace

std::vector<StaffOnset> classifyRoleOnsets(std::size_t role, std::span<const float> stem, const RoleOnsetProfile& profile,
                                           const std::vector<double>& refsS, const std::vector<double>& releasesS,
                                           std::uint32_t sampleRate, double toleranceS) {
    std::vector<StaffOnset> out;
    OnsetParams p{};
    p.delta = profile.delta;
    const OnsetDetection det = detectOnsetsSeconds(monoFromStereo(stem), p, sampleRate);
    if (det.status != AnalysisStatus::Ok) return out;
    std::vector<double> est = det.timesS;
    for (auto& t : est) t -= profile.offsetS;
    std::sort(est.begin(), est.end());
    const OnsetScore sc = matchOnsetsMaxCardinality(refsS, est, toleranceS); // indices refer to the sorted lists
    std::vector<bool> matched(est.size(), false);
    for (const auto& pr : sc.pairs) matched[pr.estimate] = true;
    for (std::size_t i = 0; i < est.size(); ++i) {
        if (est[i] < 0.0) continue;
        StaffOnsetKind kind = StaffOnsetKind::Matched;
        if (!matched[i]) {
            const auto rel = std::lower_bound(releasesS.begin(), releasesS.end(), est[i] - toleranceS);
            kind = (rel != releasesS.end() && *rel <= est[i] + toleranceS) ? StaffOnsetKind::Release : StaffOnsetKind::Unmatched;
        }
        out.push_back({role, static_cast<std::uint64_t>(std::llround(est[i] * sampleRate)), kind});
    }
    return out;
}

std::size_t staffLaneTop(std::size_t role) noexcept { return kStaffSeparator + role * (kStaffLaneHeight + kStaffSeparator); }

StaffViewImage renderStaffView(const StaffViewInput& in) {
    StaffViewImage img;
    if (in.endFrame <= in.startFrame || in.sampleRate == 0 || !(in.pixelsPerSecond > 0.0) || !std::isfinite(in.pixelsPerSecond)) {
        return img;
    }
    const double frames = static_cast<double>(in.endFrame - in.startFrame);
    const double pxPerFrame = in.pixelsPerSecond / static_cast<double>(in.sampleRate);
    const auto width = static_cast<std::size_t>(std::ceil(frames * pxPerFrame));
    const std::size_t height = kStaffSeparator + kRoleCount * (kStaffLaneHeight + kStaffSeparator);
    if (width == 0 || width > 65535) return img;
    img.width = static_cast<std::uint32_t>(width);
    img.height = static_cast<std::uint32_t>(height);
    img.rgb.assign(width * height * 3, kBackground);
    Canvas cv{width, height, img.rgb};

    auto xOf = [&](std::uint64_t frame) -> std::size_t {
        if (frame <= in.startFrame) return 0;
        return static_cast<std::size_t>(std::floor(static_cast<double>(frame - in.startFrame) * pxPerFrame));
    };
    const std::array<std::uint8_t, 3> laneColour{kLane, kLane, kLane};
    const std::array<std::uint8_t, 3> separatorColour{kSeparatorGrey, kSeparatorGrey, kSeparatorGrey};
    const std::array<std::uint8_t, 3> barColour{kBarGrey, kBarGrey, kBarGrey};
    for (std::size_t r = 0; r < kRoleCount; ++r) {
        const std::size_t top = staffLaneTop(r);
        cv.fillRect(0, top - kStaffSeparator, width, top, separatorColour);
        cv.fillRect(0, top, width, top + kStaffLaneHeight, laneColour);
    }
    cv.fillRect(0, height - kStaffSeparator, width, height, separatorColour);
    for (const auto b : in.barFrames) {
        if (b < in.startFrame || b > in.endFrame) continue;
        const std::size_t x = (std::min)(xOf(b), width - 1);
        for (std::size_t r = 0; r < kRoleCount; ++r) {
            const std::size_t top = staffLaneTop(r);
            cv.fillRect(x, top, x + 1, top + kStaffLaneHeight, barColour);
        }
    }

    // Planned notes, placed by log-pitch inside the note area of the lane
    const std::size_t noteAreaHeight = kStaffLaneHeight - 2 * kStaffMarkerStrip;
    for (const auto& n : in.planned) {
        if (n.role >= kRoleCount || n.endFrame <= in.startFrame || n.startFrame >= in.endFrame) continue;
        const auto [lo, hi] = in.pitchRange[n.role];
        double t = 0.5;
        if (lo > 0.0f && hi > lo && n.pitchHz > 0.0f && std::isfinite(n.pitchHz)) {
            t = std::clamp((std::log2(static_cast<double>(n.pitchHz)) - std::log2(static_cast<double>(lo))) /
                               (std::log2(static_cast<double>(hi)) - std::log2(static_cast<double>(lo))),
                           0.0, 1.0);
        }
        const std::size_t areaTop = staffLaneTop(n.role) + kStaffMarkerStrip;
        const auto y = areaTop + static_cast<std::size_t>(std::lround((1.0 - t) * static_cast<double>(noteAreaHeight - kNoteThickness)));
        const std::size_t x0 = xOf(n.startFrame);
        const std::size_t x1 = (std::max)(x0 + 1, (std::min)(xOf(n.endFrame), width));
        cv.fillRect(x0, y, x1, y + kNoteThickness, kStaffRoleColour[n.role]);
    }
    // Applied NoteOns: top strip
    for (const auto& [role, frame] : in.appliedNoteOns) {
        if (role >= kRoleCount || frame < in.startFrame || frame >= in.endFrame) continue;
        const std::size_t top = staffLaneTop(role);
        cv.fillRect(xOf(frame), top, xOf(frame) + 1, top + kStaffMarkerStrip, kStaffAppliedColour);
    }
    // Detected onsets: bottom strip
    for (const auto& o : in.detected) {
        if (o.role >= kRoleCount || o.frame < in.startFrame || o.frame >= in.endFrame) continue;
        const std::size_t bottom = staffLaneTop(o.role) + kStaffLaneHeight;
        const std::size_t x = xOf(o.frame);
        cv.fillRect(x, bottom - kStaffMarkerStrip, x + 2, bottom, kStaffOnsetColour[static_cast<std::size_t>(o.kind)]);
    }
    img.png = encodePngRgb8(img.width, img.height, img.rgb);
    return img;
}

} // namespace cymatica::cacp
