#include "music_clock.h"

#include <algorithm>
#include <cstdint>

namespace cymatica::audio {

namespace {

[[nodiscard]] bool isSupportedTempoMap(const TempoMap& map) noexcept {
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
    if (!isSupportedTempoMap(tempoMap)) {
        return std::nullopt;
    }
    return MusicClock(internalSampleRate, tempoMap, DirectTag{});
}

bool MusicClock::setTempoMap(const TempoMap& map) noexcept {
    if (!isSupportedTempoMap(map)) {
        return false; // Explicit failure on unsupported / inconsistent configs (§7.5, DIF-M1-22)
    }
    tempoMap_ = map;
    return true;
}

std::uint64_t MusicClock::frameAtBeat(std::uint64_t totalBeats) const noexcept {
    // Exact integer formula: floor(b * 60 * sampleRate * bpmDen / bpmNum) with overflow safety
    const std::uint64_t num = tempoMap_.bpm.numerator;
    const std::uint64_t den = tempoMap_.bpm.denominator;
    if (num == 0) return 0;
    const std::uint64_t framesNumerator = 60ULL * sampleRate_ * den;
    if (framesNumerator == 0) return 0;
    if (totalBeats <= UINT64_MAX / framesNumerator) {
        return (totalBeats * framesNumerator) / num;
    }
    const std::uint64_t q = totalBeats / num;
    const std::uint64_t r = totalBeats % num;
    return q * framesNumerator + (r * framesNumerator) / num;
}

std::uint64_t MusicClock::frameAtBar(std::uint64_t totalBars) const noexcept {
    return frameAtBeat(totalBars * tempoMap_.beatsPerBar);
}

std::uint64_t MusicClock::frameAtPhrase(std::uint64_t totalPhrases) const noexcept {
    return frameAtBar(totalPhrases * tempoMap_.barsPerPhrase);
}

MusicPosition MusicClock::positionAtFrame(std::uint64_t frame, std::uint64_t epoch) const noexcept {
    if (sampleRate_ == 0 || tempoMap_.beatsPerBar == 0 || tempoMap_.barsPerPhrase == 0 ||
        tempoMap_.bpm.numerator == 0 || tempoMap_.bpm.denominator == 0) {
        return MusicPosition{.sampleFrame = frame, .transportEpoch = epoch};
    }

    const std::uint64_t num = tempoMap_.bpm.numerator;
    const std::uint64_t den = tempoMap_.bpm.denominator;
    const std::uint64_t framesNumerator = 60ULL * sampleRate_ * den;

    // Determine beat in O(1) exactly (§7.5, ADR-0002, DIF-M1-01) with overflow safety
    std::uint64_t totalBeats = 0;
    if (framesNumerator > 0) {
        if (frame <= UINT64_MAX / num) {
            totalBeats = (frame * num) / framesNumerator;
        } else {
            const std::uint64_t q = frame / framesNumerator;
            const std::uint64_t r = frame % framesNumerator;
            totalBeats = q * num + (r * num) / framesNumerator;
        }
    }
    if (frameAtBeat(totalBeats + 1) <= frame) {
        ++totalBeats;
    }

    const std::uint64_t beatStartFrame = frameAtBeat(totalBeats);
    const std::uint64_t nextBeatStartFrame = frameAtBeat(totalBeats + 1);
    const std::uint64_t beatDurationFrames = nextBeatStartFrame > beatStartFrame ? nextBeatStartFrame - beatStartFrame : 1;

    const std::uint64_t frameInBeat = frame >= beatStartFrame ? frame - beatStartFrame : 0;
    const float beatPhase = static_cast<float>(frameInBeat) / static_cast<float>(beatDurationFrames);

    const auto sub = static_cast<std::uint32_t>(beatPhase * static_cast<float>(tempoMap_.subdivisionsPerBeat));

    const std::uint64_t beatsPerBar = tempoMap_.beatsPerBar;
    const std::uint64_t barsPerPhrase = tempoMap_.barsPerPhrase;

    const auto beatInBar = static_cast<std::uint32_t>(totalBeats % beatsPerBar);
    const std::uint64_t totalBars = totalBeats / beatsPerBar;
    const auto barInPhrase = static_cast<std::uint32_t>(totalBars % barsPerPhrase);
    const auto phrase = static_cast<std::uint32_t>(totalBars / barsPerPhrase);

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
    // Rational exact conversion: (deviceFrames * sampleRate_) / deviceRate with overflow safety
    // For 44100 -> 48000: 48000 / 44100 = 160 / 147
    if (deviceFrames <= UINT64_MAX / sampleRate_) {
        return (deviceFrames * sampleRate_) / deviceRate;
    }
    const std::uint64_t q = deviceFrames / deviceRate;
    const std::uint64_t r = deviceFrames % deviceRate;
    return q * sampleRate_ + (r * sampleRate_) / deviceRate;
}

std::uint64_t MusicClock::internalToDeviceFrames(std::uint64_t internalFrames, std::uint64_t deviceRate) const noexcept {
    if (deviceRate == 0 || sampleRate_ == 0) {
        return 0; // Explicitly fail on zero sample rates by returning 0 (DIF-M1-22)
    }
    if (deviceRate == sampleRate_) {
        return internalFrames;
    }
    if (internalFrames <= UINT64_MAX / deviceRate) {
        return (internalFrames * deviceRate) / sampleRate_;
    }
    const std::uint64_t q = internalFrames / sampleRate_;
    const std::uint64_t r = internalFrames % sampleRate_;
    return q * deviceRate + (r * deviceRate) / sampleRate_;
}

} // namespace cymatica::audio
