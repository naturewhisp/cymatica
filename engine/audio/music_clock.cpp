#include "music_clock.h"
#include "unsigned_math.h"
#include <cmath>
#include <limits>

#include <algorithm>
#include <cstdint>

namespace cymatica::audio {

namespace {

[[nodiscard]] bool isSupportedTempoMap(const TempoMap& map, std::uint64_t rate) noexcept {
    if (map.bpm.numerator == 0 || map.bpm.denominator == 0) {
        return false;
    }
    if (map.timeSignatureNum == 0 || map.timeSignatureDen == 0) {
        return false;
    }
    if (map.beatsPerBar == 0 || map.barsPerPhrase == 0 || map.subdivisionsPerBeat == 0) {
        return false;
    }
    // Consistency: beatsPerBar must match timeSignatureNum (§7.5, DIF-M1-22)
    if (map.beatsPerBar != map.timeSignatureNum) {
        return false;
    }
    // Spec §7.5 prototype scope: 4/4 constant tempo required, unsupported configs must fail explicitly
    if (map.timeSignatureNum != 4 || map.timeSignatureDen != 4) {
        return false;
    }
    // Supported exact profile: >= one frame per beat and phrase numerator representable.
    if (rate == 0 || rate > UINT64_MAX / 60 / map.bpm.denominator) return false;
    const auto n = rate * 60 * map.bpm.denominator;
    if (map.bpm.numerator > n || n > UINT64_MAX / map.beatsPerBar / map.barsPerPhrase) return false;
    return true;
}

} // namespace

MusicClock::MusicClock() noexcept
    : sampleRate_(48000), tempoMap_(TempoMap{}) {}

MusicClock::MusicClock(std::uint64_t internalSampleRate, const TempoMap& tempoMap, DirectTag) noexcept
    : sampleRate_(internalSampleRate), tempoMap_(tempoMap) {}

std::optional<MusicClock> MusicClock::create(std::uint64_t internalSampleRate, const TempoMap& tempoMap) noexcept {
    if (internalSampleRate == 0) {
        return std::nullopt;
    }
    if (!isSupportedTempoMap(tempoMap, internalSampleRate)) {
        return std::nullopt;
    }
    return MusicClock(internalSampleRate, tempoMap, DirectTag{});
}

bool MusicClock::setTempoMap(const TempoMap& map) noexcept {
    if (!isSupportedTempoMap(map, sampleRate_)) {
        return false; // Explicit failure on unsupported / inconsistent configs (§7.5, DIF-M1-22)
    }
    tempoMap_ = map;
    return true;
}

std::uint64_t MusicClock::frameAtBeat(std::uint64_t totalBeats) const noexcept {
    return core::multiplyDivide(totalBeats, 60ULL * sampleRate_ * tempoMap_.bpm.denominator,
                                tempoMap_.bpm.numerator).value;
}

std::uint64_t MusicClock::frameAtBar(std::uint64_t totalBars) const noexcept {
    return core::multiplyDivide(totalBars, 60ULL * sampleRate_ * tempoMap_.bpm.denominator * tempoMap_.beatsPerBar,
                                tempoMap_.bpm.numerator).value;
}

std::uint64_t MusicClock::frameAtPhrase(std::uint64_t totalPhrases) const noexcept {
    return core::multiplyDivide(totalPhrases, 60ULL * sampleRate_ * tempoMap_.bpm.denominator *
                                tempoMap_.beatsPerBar * tempoMap_.barsPerPhrase, tempoMap_.bpm.numerator).value;
}

MusicPosition MusicClock::positionAtFrame(std::uint64_t frame, std::uint64_t epoch) const noexcept {
    if (sampleRate_ == 0 || tempoMap_.beatsPerBar == 0 || tempoMap_.barsPerPhrase == 0 ||
        tempoMap_.bpm.numerator == 0 || tempoMap_.bpm.denominator == 0) {
        return MusicPosition{.sampleFrame = frame, .transportEpoch = epoch};
    }

    const std::uint64_t num = tempoMap_.bpm.numerator;
    const std::uint64_t den = tempoMap_.bpm.denominator;
    const std::uint64_t framesNumerator = 60ULL * sampleRate_ * den;

    // Invert floor boundary exactly, including UINT64_MAX, without frame+1.
    const auto inverse = core::multiplyDivide(frame, num, framesNumerator);
    const std::uint64_t totalBeats = inverse.value +
        static_cast<std::uint64_t>(inverse.remainder >= framesNumerator - (num - 1));
    const auto boundary = core::multiplyDivide(totalBeats, framesNumerator, num);
    const auto beatDurationFrames = framesNumerator / num +
        static_cast<std::uint64_t>(boundary.remainder + framesNumerator % num >= num);
    const auto frameInBeat = frame - boundary.value;
    const float beatPhase = std::min(static_cast<float>(frameInBeat) / static_cast<float>(beatDurationFrames),
                                     std::nextafter(1.0f, 0.0f));
    const auto sub = static_cast<std::uint32_t>(std::min(
        static_cast<double>(beatPhase) * tempoMap_.subdivisionsPerBeat,
        static_cast<double>(tempoMap_.subdivisionsPerBeat - 1)));

    const std::uint64_t beatsPerBar = tempoMap_.beatsPerBar;
    const std::uint64_t barsPerPhrase = tempoMap_.barsPerPhrase;

    const auto beatInBar = static_cast<std::uint32_t>(totalBeats % beatsPerBar);
    const std::uint64_t totalBars = totalBeats / beatsPerBar;
    const auto barInPhrase = static_cast<std::uint32_t>(totalBars % barsPerPhrase);
    const auto phrase = static_cast<std::uint32_t>(std::min<std::uint64_t>(totalBars / barsPerPhrase, UINT32_MAX));

    const auto totalBeatsInPhrase = static_cast<float>(beatsPerBar * barsPerPhrase);
    const float beatsElapsedInPhrase = static_cast<float>(barInPhrase * beatsPerBar + beatInBar) + beatPhase;
    const float phrasePhase = totalBeatsInPhrase > 0.0f ? beatsElapsedInPhrase / totalBeatsInPhrase : 0.0f;

    return MusicPosition{
        .sampleFrame = frame,
        .transportEpoch = epoch,
        .phrase = phrase,
        .bar = barInPhrase,
        .beat = beatInBar,
        .subdivision = sub,
        .beatPhase = beatPhase,
        .phrasePhase = phrasePhase,
    };
}

std::uint64_t MusicClock::deviceToInternalFrames(std::uint64_t deviceFrames, std::uint64_t deviceRate) const noexcept {
    if (deviceRate == 0 || sampleRate_ == 0) {
        return 0; // Explicitly fail on zero sample rates by returning 0 (DIF-M1-22)
    }
    if (deviceRate == sampleRate_) {
        return deviceFrames;
    }
    return core::multiplyDivide(deviceFrames, sampleRate_, deviceRate).value;
}

std::uint64_t MusicClock::internalToDeviceFrames(std::uint64_t internalFrames, std::uint64_t deviceRate) const noexcept {
    if (deviceRate == 0 || sampleRate_ == 0) {
        return 0; // Explicitly fail on zero sample rates by returning 0 (DIF-M1-22)
    }
    if (deviceRate == sampleRate_) {
        return internalFrames;
    }
    return core::multiplyDivide(internalFrames, deviceRate, sampleRate_).value;
}

} // namespace cymatica::audio
