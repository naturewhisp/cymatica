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
constexpr std::size_t kNoteThickness = 4;
constexpr std::array<std::uint8_t, 3> kTextGrey{170, 170, 170};
constexpr std::array<std::uint8_t, 3> kTextDim{120, 120, 120};

// Glyph rows: '#' = ink. Order: A-Z, 0-9, then the symbols of staffGlyph.
struct GlyphRows {
    char c;
    const char* rows[7];
};
constexpr GlyphRows kGlyphs[] = {
    {'A', {".###.", "#...#", "#...#", "#####", "#...#", "#...#", "#...#"}},
    {'B', {"####.", "#...#", "#...#", "####.", "#...#", "#...#", "####."}},
    {'C', {".###.", "#...#", "#....", "#....", "#....", "#...#", ".###."}},
    {'D', {"####.", "#...#", "#...#", "#...#", "#...#", "#...#", "####."}},
    {'E', {"#####", "#....", "#....", "####.", "#....", "#....", "#####"}},
    {'F', {"#####", "#....", "#....", "####.", "#....", "#....", "#...."}},
    {'G', {".###.", "#...#", "#....", "#.###", "#...#", "#...#", ".####"}},
    {'H', {"#...#", "#...#", "#...#", "#####", "#...#", "#...#", "#...#"}},
    {'I', {".###.", "..#..", "..#..", "..#..", "..#..", "..#..", ".###."}},
    {'J', {"..###", "...#.", "...#.", "...#.", "...#.", "#..#.", ".##.."}},
    {'K', {"#...#", "#..#.", "#.#..", "##...", "#.#..", "#..#.", "#...#"}},
    {'L', {"#....", "#....", "#....", "#....", "#....", "#....", "#####"}},
    {'M', {"#...#", "##.##", "#.#.#", "#.#.#", "#...#", "#...#", "#...#"}},
    {'N', {"#...#", "#...#", "##..#", "#.#.#", "#..##", "#...#", "#...#"}},
    {'O', {".###.", "#...#", "#...#", "#...#", "#...#", "#...#", ".###."}},
    {'P', {"####.", "#...#", "#...#", "####.", "#....", "#....", "#...."}},
    {'Q', {".###.", "#...#", "#...#", "#...#", "#.#.#", "#..#.", ".##.#"}},
    {'R', {"####.", "#...#", "#...#", "####.", "#.#..", "#..#.", "#...#"}},
    {'S', {".####", "#....", "#....", ".###.", "....#", "....#", "####."}},
    {'T', {"#####", "..#..", "..#..", "..#..", "..#..", "..#..", "..#.."}},
    {'U', {"#...#", "#...#", "#...#", "#...#", "#...#", "#...#", ".###."}},
    {'V', {"#...#", "#...#", "#...#", "#...#", "#...#", ".#.#.", "..#.."}},
    {'W', {"#...#", "#...#", "#...#", "#.#.#", "#.#.#", "#.#.#", ".#.#."}},
    {'X', {"#...#", "#...#", ".#.#.", "..#..", ".#.#.", "#...#", "#...#"}},
    {'Y', {"#...#", "#...#", ".#.#.", "..#..", "..#..", "..#..", "..#.."}},
    {'Z', {"#####", "....#", "...#.", "..#..", ".#...", "#....", "#####"}},
    {'0', {".###.", "#...#", "#..##", "#.#.#", "##..#", "#...#", ".###."}},
    {'1', {"..#..", ".##..", "..#..", "..#..", "..#..", "..#..", ".###."}},
    {'2', {".###.", "#...#", "....#", "...#.", "..#..", ".#...", "#####"}},
    {'3', {"#####", "...#.", "..#..", "...#.", "....#", "#...#", ".###."}},
    {'4', {"...#.", "..##.", ".#.#.", "#..#.", "#####", "...#.", "...#."}},
    {'5', {"#####", "#....", "####.", "....#", "....#", "#...#", ".###."}},
    {'6', {"..##.", ".#...", "#....", "####.", "#...#", "#...#", ".###."}},
    {'7', {"#####", "....#", "...#.", "..#..", ".#...", ".#...", ".#..."}},
    {'8', {".###.", "#...#", "#...#", ".###.", "#...#", "#...#", ".###."}},
    {'9', {".###.", "#...#", "#...#", ".####", "....#", "...#.", ".##.."}},
    {'+', {".....", "..#..", "..#..", "#####", "..#..", "..#..", "....."}},
    {'/', {"....#", "....#", "...#.", "..#..", ".#...", "#....", "#...."}},
    {':', {".....", "..#..", ".....", ".....", ".....", "..#..", "....."}},
    {'-', {".....", ".....", ".....", "#####", ".....", ".....", "....."}},
    {'(', {"...#.", "..#..", ".#...", ".#...", ".#...", "..#..", "...#."}},
    {')', {".#...", "..#..", "...#.", "...#.", "...#.", "..#..", ".#..."}},
    {'.', {".....", ".....", ".....", ".....", ".....", ".##..", ".##.."}},
    {'=', {".....", ".....", "#####", ".....", "#####", ".....", "....."}},
    {'\'', {"..#..", "..#..", ".#...", ".....", ".....", ".....", "....."}},
    {',', {".....", ".....", ".....", ".....", "..##.", "..#..", ".#..."}},
};

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
    // Text in the 5x7 font, each pixel a scale x scale block, 1 blank column between glyphs; returns the end x
    std::size_t text(std::size_t x, std::size_t y, const std::string& s, const std::array<std::uint8_t, 3>& c,
                     std::size_t scale = 1) {
        for (const char ch : s) {
            const auto g = staffGlyph(ch);
            for (std::size_t row = 0; row < 7; ++row) {
                for (std::size_t col = 0; col < 5; ++col) {
                    if ((g[row] >> (4 - col)) & 1u) fillRect(x + col * scale, y + row * scale, x + (col + 1) * scale, y + (row + 1) * scale, c);
                }
            }
            x += 6 * scale;
        }
        return x;
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

std::array<std::uint8_t, 7> staffGlyph(char c) noexcept {
    if (c >= 'a' && c <= 'z') c = static_cast<char>(c - 'a' + 'A');
    for (const auto& g : kGlyphs) {
        if (g.c != c) continue;
        std::array<std::uint8_t, 7> out{};
        for (std::size_t row = 0; row < 7; ++row) {
            for (std::size_t col = 0; col < 5; ++col) {
                if (g.rows[row][col] == '#') out[row] = static_cast<std::uint8_t>(out[row] | (1u << (4 - col)));
            }
        }
        return out;
    }
    return {};
}

std::size_t staffLaneTop(std::size_t role) noexcept {
    return kStaffHeaderHeight + kStaffSeparator + role * (kStaffLaneHeight + kStaffSeparator);
}

std::size_t staffViewHeight() noexcept {
    return kStaffHeaderHeight + kStaffSeparator + kRoleCount * (kStaffLaneHeight + kStaffSeparator) + kStaffFooterHeight;
}

StaffViewImage renderStaffView(const StaffViewInput& in) {
    StaffViewImage img;
    if (in.endFrame <= in.startFrame || in.sampleRate == 0 || !(in.pixelsPerSecond > 0.0) || !std::isfinite(in.pixelsPerSecond)) {
        return img;
    }
    const double frames = static_cast<double>(in.endFrame - in.startFrame);
    const double pxPerFrame = in.pixelsPerSecond / static_cast<double>(in.sampleRate);
    const auto timeWidth = static_cast<std::size_t>(std::ceil(frames * pxPerFrame));
    const std::size_t width = kStaffLabelWidth + timeWidth;
    const std::size_t height = staffViewHeight();
    if (timeWidth == 0 || width > 65535) return img;
    img.width = static_cast<std::uint32_t>(width);
    img.height = static_cast<std::uint32_t>(height);
    img.rgb.assign(width * height * 3, kBackground);
    Canvas cv{width, height, img.rgb};

    auto xOf = [&](std::uint64_t frame) -> std::size_t {
        if (frame <= in.startFrame) return kStaffLabelWidth;
        return kStaffLabelWidth + static_cast<std::size_t>(std::floor(static_cast<double>(frame - in.startFrame) * pxPerFrame));
    };
    const std::array<std::uint8_t, 3> laneColour{kLane, kLane, kLane};
    const std::array<std::uint8_t, 3> separatorColour{kSeparatorGrey, kSeparatorGrey, kSeparatorGrey};
    const std::size_t lanesBottom = staffLaneTop(kRoleCount - 1) + kStaffLaneHeight;
    for (std::size_t r = 0; r < kRoleCount; ++r) {
        const std::size_t top = staffLaneTop(r);
        cv.fillRect(0, top - kStaffSeparator, width, top, separatorColour);
        cv.fillRect(kStaffLabelWidth, top, width, top + kStaffLaneHeight, laneColour);
        // Label column: instrument name in the role colour, internal role name below it
        cv.text(6, top + 10, kStaffInstrumentNames[r], kStaffRoleColour[r], 2);
        cv.text(6, top + 34, std::string("(") + kRoleNames[r] + ")", kTextDim);
    }
    cv.fillRect(0, lanesBottom, width, lanesBottom + kStaffSeparator, separatorColour);
    // Grid, finest first so that coarser lines stay on top: steps, beats, bars
    const auto gridLines = [&](const std::vector<std::uint64_t>& frames, const std::array<std::uint8_t, 3>& colour) {
        for (const auto f : frames) {
            if (f < in.startFrame || f > in.endFrame) continue;
            const std::size_t x = (std::min)(xOf(f), width - 1);
            for (std::size_t r = 0; r < kRoleCount; ++r) {
                const std::size_t top = staffLaneTop(r);
                cv.fillRect(x, top, x + 1, top + kStaffLaneHeight, colour);
            }
        }
    };
    gridLines(in.stepFrames, kStaffStepLineColour);
    gridLines(in.beatFrames, kStaffBeatLineColour);
    gridLines(in.barFrames, kStaffBarLineColour);
    // Header: caption over the label column, bar numbers right of each bar line except the closing one
    cv.text(6, 4, in.caption, kTextGrey);
    for (std::size_t i = 0; i + 1 < in.barFrames.size(); ++i) {
        const auto b = in.barFrames[i];
        if (b < in.startFrame || b >= in.endFrame) continue;
        const std::size_t x = xOf(b);
        cv.fillRect(x, 2, x + 1, kStaffHeaderHeight, kStaffBarLineColour);
        cv.text(x + 3, 4, std::to_string(in.firstBarNumber + i), kTextGrey);
    }
    // Footer: colour legend
    std::size_t lx = 6;
    const std::size_t ly = lanesBottom + kStaffSeparator + 4;
    lx = cv.text(lx, ly, "BARS: PLANNED NOTES, HEIGHT = PITCH   ", kTextGrey);
    lx = cv.text(lx, ly, "TOP WHITE: NOTE STARTED   ", kStaffAppliedColour);
    (void)cv.text(lx, ly, "GRID: BAR, BEAT, STEP", kTextGrey);
    lx = 6;
    const std::size_t ly2 = ly + 10;
    lx = cv.text(lx, ly2, "BOTTOM, DETECTED ONSET   ", kTextGrey);
    lx = cv.text(lx, ly2, "GREEN: ON A NOTE   ", kStaffOnsetColour[0]);
    lx = cv.text(lx, ly2, "RED: NO NOTE   ", kStaffOnsetColour[1]);
    (void)cv.text(lx, ly2, "ORANGE: AT A RELEASE", kStaffOnsetColour[2]);

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
