#include "music_clock.h"

#include <algorithm>

namespace cymatica::audio {

namespace {

void sanitizeTempoMap(TempoMap& map) noexcept {
    if (map.bpm.numerator == 0) {
        map.bpm.numerator = 120;
    }
    if (map.bpm.denominator == 0) {
        map.bpm.denominator = 1;
    }
    if (map.beatsPerBar == 0) {
        map.beatsPerBar = 4;
    }
    if (map.barsPerPhrase == 0) {
        map.barsPerPhrase = 4;
    }
    if (map.subdivisionsPerBeat == 0) {
        map.subdivisionsPerBeat = 4;
    }
}

} // namespace

MusicClock::MusicClock(std::uint64_t internalSampleRate, const TempoMap& tempoMap) noexcept
    : sampleRate_(internalSampleRate != 0 ? internalSampleRate : 48000), tempoMap_(tempoMap) {
    sanitizeTempoMap(tempoMap_);
}

void MusicClock::setTempoMap(const TempoMap& map) noexcept {
    tempoMap_ = map;
    sanitizeTempoMap(tempoMap_);
}

std::uint64_t MusicClock::frameAtBeat(std::uint64_t totalBeats) const noexcept {
    // Exact integer formula: floor(b * 60 * sampleRate * bpmDen / bpmNum)
    const std::uint64_t num = tempoMap_.bpm.numerator;
    const std::uint64_t den = tempoMap_.bpm.denominator;
    // (60 * sampleRate * den) fits comfortably in uint64
    const std::uint64_t framesNumerator = 60ULL * sampleRate_ * den;
    return (totalBeats * framesNumerator) / num;
}

std::uint64_t MusicClock::frameAtBar(std::uint64_t totalBars) const noexcept {
    return frameAtBeat(totalBars * tempoMap_.beatsPerBar);
}

std::uint64_t MusicClock::frameAtPhrase(std::uint64_t totalPhrases) const noexcept {
    return frameAtBar(totalPhrases * tempoMap_.barsPerPhrase);
}

MusicPosition MusicClock::positionAtFrame(std::uint64_t frame, std::uint64_t epoch) const noexcept {
    const std::uint64_t num = tempoMap_.bpm.numerator;
    const std::uint64_t den = tempoMap_.bpm.denominator;
    const std::uint64_t framesNumerator = 60ULL * sampleRate_ * den;

    // Determine beat in O(1) exactly (§7.5, ADR-0002, DIF-M1-01)
    // For fractional BPM, (frame * num) / framesNumerator can yield b - 1 at frameAtBeat(b).
    // By checking if frameAtBeat(totalBeats + 1) <= frame, we guarantee totalBeats == b when frame == frameAtBeat(b).
    std::uint64_t totalBeats = (frame * num) / framesNumerator;
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
    if (deviceRate == 0 || deviceRate == sampleRate_) {
        return deviceFrames;
    }
    // Rational exact conversion: (deviceFrames * sampleRate_) / deviceRate
    // For 44100 -> 48000: 48000 / 44100 = 160 / 147
    return (deviceFrames * sampleRate_) / deviceRate;
}

std::uint64_t MusicClock::internalToDeviceFrames(std::uint64_t internalFrames, std::uint64_t deviceRate) const noexcept {
    if (deviceRate == 0 || deviceRate == sampleRate_) {
        return internalFrames;
    }
    return (internalFrames * deviceRate) / sampleRate_;
}

} // namespace cymatica::audio
