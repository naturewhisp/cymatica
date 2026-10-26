#include "plan_content_fingerprint.h"

#include <algorithm>
#include <cmath>
#include <map>
#include <tuple>

namespace cymatica::audio {

namespace {

struct Fnv1a64 {
    std::uint64_t h{0xcbf29ce484222325ULL};
    void byte(std::uint8_t b) noexcept { h ^= b; h *= 0x100000001b3ULL; }
    void u64(std::uint64_t v) noexcept { for (int i = 0; i < 8; ++i) byte(static_cast<std::uint8_t>(v >> (8 * i))); }
    void u32(std::uint32_t v) noexcept { for (int i = 0; i < 4; ++i) byte(static_cast<std::uint8_t>(v >> (8 * i))); }
    void i32(std::int32_t v) noexcept { u32(static_cast<std::uint32_t>(v)); }
};

struct NoteRecord {
    std::uint8_t role{0};
    std::uint64_t frame{0};
    std::int32_t pitch{0};
    std::int32_t velocity{0};
    std::int32_t timbre{0};
    std::uint32_t durationFrames{0};
    std::vector<std::uint64_t> explicitReleases{};

    [[nodiscard]] auto key() const noexcept {
        return std::tie(frame, role, pitch, velocity, timbre, durationFrames, explicitReleases);
    }
};

// round(x * 1000) for a finite value; -0 maps to 0 through lround
std::optional<std::int32_t> quantizeMilli(float x) noexcept {
    if (!std::isfinite(x)) return std::nullopt;
    return static_cast<std::int32_t>(std::lround(static_cast<double>(x) * 1000.0));
}

std::optional<std::int32_t> quantizeCents(float hz) noexcept {
    if (!std::isfinite(hz) || hz < 0.0f) return std::nullopt;
    if (hz == 0.0f) return kFingerprintPitchNotApplicable;
    return static_cast<std::int32_t>(std::lround(1200.0 * std::log2(static_cast<double>(hz) / 440.0)));
}

} // namespace

MusicalContentFingerprint musicalContentFingerprint(const std::vector<std::vector<MusicIntentEvent>>& intentsPerPhrase,
                                                    const std::vector<PlannedNoteOff>& noteOffs,
                                                    std::uint32_t sampleRate, std::optional<MusicRole> roleFilter) {
    MusicalContentFingerprint fp;

    // Explicit releases keyed by (eventId, role): the join key between a NoteOn and its planner NoteOff
    std::map<std::pair<std::uint64_t, std::uint8_t>, std::vector<std::uint64_t>> releases;
    for (const auto& off : noteOffs) {
        releases[{off.eventId, static_cast<std::uint8_t>(off.role)}].push_back(off.sampleFrame);
    }

    std::vector<NoteRecord> notes;
    for (const auto& phrase : intentsPerPhrase) {
        for (const auto& e : phrase) {
            const auto pitch = quantizeCents(e.pitchHz);
            const auto velocity = quantizeMilli(e.velocity);
            const auto timbre = quantizeMilli(e.timbreParam);
            if (!pitch || !velocity || !timbre) {
                fp.error = "non-finite field in intent eventId " + std::to_string(e.eventId);
                return fp;
            }
            const auto roleKey = static_cast<std::uint8_t>(e.role);
            NoteRecord n{.role = roleKey, .frame = e.sampleFrame, .pitch = *pitch, .velocity = *velocity,
                         .timbre = *timbre, .durationFrames = e.durationFrames};
            const auto it = releases.find({e.eventId, roleKey});
            if (it != releases.end()) {
                n.explicitReleases = std::move(it->second);
                std::sort(n.explicitReleases.begin(), n.explicitReleases.end());
                releases.erase(it); // consumed: what remains are orphan releases
            }
            if (!roleFilter || *roleFilter == e.role) {
                notes.push_back(std::move(n));
            }
        }
    }

    std::vector<std::pair<std::uint64_t, std::uint8_t>> orphans; // (frame, role)
    for (const auto& [key, frames] : releases) {
        if (roleFilter && static_cast<std::uint8_t>(*roleFilter) != key.second) continue;
        for (const auto f : frames) orphans.emplace_back(f, key.second);
    }

    std::sort(notes.begin(), notes.end(), [](const NoteRecord& a, const NoteRecord& b) { return a.key() < b.key(); });
    std::sort(orphans.begin(), orphans.end());

    Fnv1a64 f;
    f.u32(kMusicalContentFingerprintFormatVersion);
    f.u32(sampleRate);
    f.u64(notes.size());
    f.u64(orphans.size());
    for (const auto& n : notes) {
        f.byte(n.role);
        f.u64(n.frame);
        f.i32(n.pitch);
        f.i32(n.velocity);
        f.i32(n.timbre);
        f.u32(n.durationFrames);
        f.u32(static_cast<std::uint32_t>(n.explicitReleases.size()));
        for (const auto r : n.explicitReleases) f.u64(r);
        fp.explicitReleaseCount += n.explicitReleases.size();
    }
    for (const auto& [frame, role] : orphans) {
        f.byte(role);
        f.u64(frame);
    }

    fp.valid = true;
    fp.hash = f.h;
    fp.noteCount = notes.size();
    fp.orphanReleaseCount = orphans.size();
    return fp;
}

} // namespace cymatica::audio
