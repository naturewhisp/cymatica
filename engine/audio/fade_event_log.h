#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace cymatica::audio {

// Measurement package 2b (docs/research/M2_5_Pkg2b_Fade_Event_Log_Preregistration.md): every point where a voice
// stops sounding, written by the VoiceAllocator. Offline diagnostics only: installed in quiescence (offline engine
// or device not started), fixed capacity allocated by the owner, push never allocates.
enum class FadeCause : std::uint8_t {
    Release = 0,     // matched NoteOff: envelope release of releaseSamples
    Steal = 1,       // NoteOn on an active voice: oscillation cut, linear tail of the last sample
    ReleaseCut = 2,  // NoteOn on a voice still in Release: oscillation cut, linear tail of the last sample
    Stop = 3,        // transport Stop: oscillation cut, linear tail of the last sample
    Pause = 4,       // transport Pause: 2 ms physical ramp on the whole voice output
    Safety = 5       // checkLifetimes expiry: envelope release of the de-click length
};
inline constexpr const char* kFadeCauseNames[] = {"release", "steal", "release_cut", "stop", "pause", "safety"};

struct FadeEventRecord {
    std::uint64_t frame{0};        // logical frame of the event
    std::uint8_t role{0};
    std::uint8_t slot{0};
    FadeCause cause{FadeCause::Release};
    float envAmplitude{0.0f};      // envelopeLevel * velocity * kNominalVoiceScale at the event (pre-bus)
    float busGain{0.0f};           // current smoothed bus gain of the role at the event
    std::uint32_t fadeFrames{0};   // frames over which the oscillation goes to zero (0 = cut)
};

class FadeEventLog {
public:
    explicit FadeEventLog(std::size_t capacity) : records_(capacity) {}
    void push(const FadeEventRecord& r) noexcept {
        if (count_ < records_.size()) {
            records_[count_++] = r;
        } else {
            ++overflow_;
        }
    }
    void reset() noexcept {
        count_ = 0;
        overflow_ = 0;
    }
    [[nodiscard]] std::span<const FadeEventRecord> records() const noexcept { return {records_.data(), count_}; }
    [[nodiscard]] std::uint64_t overflow() const noexcept { return overflow_; }

private:
    std::vector<FadeEventRecord> records_;
    std::size_t count_{0};
    std::uint64_t overflow_{0};
};

} // namespace cymatica::audio
