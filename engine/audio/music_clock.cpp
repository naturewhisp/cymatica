#include "music_clock.h"
#include "unsigned_math.h"
#include <cmath>
#include <limits>

#include <algorithm>
#include <cstdint>

namespace cymatica::audio {

bool MusicClock::isSupportedTempoMap(const TempoMap& map, std::uint64_t rate) noexcept {
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
    // Supported time signatures: standard musical denominators (2, 4, 8, 16) and numerators in [1, 16] (Spec §32 M2)
    if (map.timeSignatureDen != 2 && map.timeSignatureDen != 4 &&
        map.timeSignatureDen != 8 && map.timeSignatureDen != 16) {
        return false;
    }
    if (map.timeSignatureNum > 16) {
        return false;
    }
    if (map.subdivisionsPerBeat > 65535 || map.barsPerPhrase > 65535) {
        return false;
    }
    // Supported exact profile: >= one frame per beat and phrase numerator representable.
    if (rate == 0 || rate > UINT64_MAX / 60 / map.bpm.denominator / 4) return false;
    const auto n = rate * 60 * map.bpm.denominator * 4;
    const auto d = static_cast<std::uint64_t>(map.bpm.numerator) * map.timeSignatureDen;
    if (d > n || n > UINT64_MAX / map.beatsPerBar / map.barsPerPhrase) return false;
    return true;
}

MusicClock::MusicClock() noexcept
    : sampleRate_(48000), segmentCount_(1) {
    segments_[0] = TempoSegment{.startFrame = 0, .startBeat = 0, .startBar = 0, .tempoMap = TempoMap{}};
}

MusicClock::MusicClock(std::uint64_t internalSampleRate, const TempoMap& tempoMap, DirectTag) noexcept
    : sampleRate_(internalSampleRate), segmentCount_(1) {
    segments_[0] = TempoSegment{.startFrame = 0, .startBeat = 0, .startBar = 0, .tempoMap = tempoMap};
}

std::optional<MusicClock> MusicClock::create(std::uint64_t internalSampleRate, const TempoMap& tempoMap) noexcept {
    if (internalSampleRate == 0) {
        return std::nullopt;
    }
    if (!isSupportedTempoMap(tempoMap, internalSampleRate)) {
        return std::nullopt;
    }
    return MusicClock(internalSampleRate, tempoMap, DirectTag{});
}

const TempoMap& MusicClock::tempoMap() const noexcept {
    return segments_[segmentCount_ - 1].tempoMap;
}

const MusicClock::TempoSegment& MusicClock::segment(std::size_t index) const noexcept {
    return segments_[std::min(index, segmentCount_ - 1)];
}

bool MusicClock::setTempoMap(const TempoMap& map) noexcept {
    return setTempoMap(map, 0);
}

bool MusicClock::setTempoMap(const TempoMap& map, std::uint64_t targetSampleFrame) noexcept {
    if (!isSupportedTempoMap(map, sampleRate_)) {
        return false; // Explicit failure on unsupported / inconsistent configs (§7.5, DIF-M1-22)
    }
    if (targetSampleFrame == 0 && segmentCount_ == 1 && segments_[0].startFrame == 0) {
        segments_[0].tempoMap = map;
        return true;
    }

    if (targetSampleFrame == segments_[segmentCount_ - 1].startFrame) {
        segments_[segmentCount_ - 1].tempoMap = map;
        return true;
    }

    if (targetSampleFrame < segments_[segmentCount_ - 1].startFrame) {
        return false; // Must be monotonically increasing
    }

    if (segmentCount_ >= kMaxTempoSegments) {
        return false; // Bounded capacity, zero RT allocation
    }

    // Determine position at targetSampleFrame in existing timeline
    std::size_t activeIdx = 0;
    for (std::size_t i = 1; i < segmentCount_; ++i) {
        if (segments_[i].startFrame <= targetSampleFrame) {
            activeIdx = i;
        } else {
            break;
        }
    }
    const auto& prevSeg = segments_[activeIdx];
    const std::uint64_t deltaF = targetSampleFrame - prevSeg.startFrame;
    const std::uint64_t K_num = 60ULL * sampleRate_ * prevSeg.tempoMap.bpm.denominator * 4ULL;
    const std::uint64_t K_den = static_cast<std::uint64_t>(prevSeg.tempoMap.bpm.numerator) * prevSeg.tempoMap.timeSignatureDen;
    const auto inverse = core::multiplyDivide(deltaF, K_den, K_num);
    const std::uint64_t elapsedBeats = inverse.value +
        static_cast<std::uint64_t>(inverse.remainder >= K_num - (K_den - 1));

    const std::uint64_t totalBeats = prevSeg.startBeat + elapsedBeats;
    const std::uint64_t elapsedBars = elapsedBeats / prevSeg.tempoMap.beatsPerBar;
    const std::uint64_t totalBars = prevSeg.startBar + elapsedBars;

    segments_[segmentCount_] = TempoSegment{
        .startFrame = targetSampleFrame,
        .startBeat = totalBeats,
        .startBar = totalBars,
        .tempoMap = map
    };
    ++segmentCount_;
    return true;
}

std::uint64_t MusicClock::frameAtBeat(std::uint64_t totalBeats) const noexcept {
    if (segmentCount_ == 0 || sampleRate_ == 0) return 0;

    std::size_t segIdx = 0;
    for (std::size_t i = 1; i < segmentCount_; ++i) {
        if (segments_[i].startBeat <= totalBeats) {
            segIdx = i;
        } else {
            break;
        }
    }

    const auto& seg = segments_[segIdx];
    const std::uint64_t deltaBeats = totalBeats - seg.startBeat;
    const std::uint64_t K_num = 60ULL * sampleRate_ * seg.tempoMap.bpm.denominator * 4ULL;
    const std::uint64_t K_den = static_cast<std::uint64_t>(seg.tempoMap.bpm.numerator) * seg.tempoMap.timeSignatureDen;

    const std::uint64_t deltaFrames = core::multiplyDivide(deltaBeats, K_num, K_den).value;
    return core::saturatingAdd(seg.startFrame, deltaFrames);
}

std::uint64_t MusicClock::frameAtBar(std::uint64_t totalBars) const noexcept {
    if (segmentCount_ == 0 || sampleRate_ == 0) return 0;

    std::size_t segIdx = 0;
    for (std::size_t i = 1; i < segmentCount_; ++i) {
        if (segments_[i].startBar <= totalBars) {
            segIdx = i;
        } else {
            break;
        }
    }

    const auto& seg = segments_[segIdx];
    const std::uint64_t deltaBars = totalBars - seg.startBar;
    const std::uint64_t K_num = 60ULL * sampleRate_ * seg.tempoMap.bpm.denominator * 4ULL * seg.tempoMap.beatsPerBar;
    const std::uint64_t K_den = static_cast<std::uint64_t>(seg.tempoMap.bpm.numerator) * seg.tempoMap.timeSignatureDen;

    const std::uint64_t deltaFrames = core::multiplyDivide(deltaBars, K_num, K_den).value;
    return core::saturatingAdd(seg.startFrame, deltaFrames);
}

std::uint64_t MusicClock::frameAtPhrase(std::uint64_t totalPhrases) const noexcept {
    if (segmentCount_ == 0 || sampleRate_ == 0) return 0;
    const auto& seg = segments_[0];
    const std::uint64_t totalBars = core::multiplyDivide(totalPhrases, seg.tempoMap.barsPerPhrase, 1).value;
    return frameAtBar(totalBars);
}

MusicPosition MusicClock::positionAtFrame(std::uint64_t frame, std::uint64_t epoch) const noexcept {
    if (sampleRate_ == 0 || segmentCount_ == 0) {
        return MusicPosition{.sampleFrame = frame, .transportEpoch = epoch};
    }

    std::size_t segIdx = 0;
    for (std::size_t i = 1; i < segmentCount_; ++i) {
        if (segments_[i].startFrame <= frame) {
            segIdx = i;
        } else {
            break;
        }
    }

    const auto& seg = segments_[segIdx];
    const auto& map = seg.tempoMap;

    if (map.beatsPerBar == 0 || map.barsPerPhrase == 0 ||
        map.bpm.numerator == 0 || map.bpm.denominator == 0 || map.timeSignatureDen == 0) {
        return MusicPosition{.sampleFrame = frame, .transportEpoch = epoch};
    }

    const std::uint64_t deltaF = frame - seg.startFrame;
    const std::uint64_t K_num = 60ULL * sampleRate_ * map.bpm.denominator * 4ULL;
    const std::uint64_t K_den = static_cast<std::uint64_t>(map.bpm.numerator) * map.timeSignatureDen;

    // Invert floor boundary exactly, including UINT64_MAX, without frame+1.
    const auto inverse = core::multiplyDivide(deltaF, K_den, K_num);
    const std::uint64_t elapsedBeats = inverse.value +
        static_cast<std::uint64_t>(inverse.remainder >= K_num - (K_den - 1));
    const auto boundary = core::multiplyDivide(elapsedBeats, K_num, K_den);
    const auto beatDurationFrames = K_num / K_den +
        static_cast<std::uint64_t>(boundary.remainder + K_num % K_den >= K_den);
    const auto frameInBeat = deltaF - boundary.value;
    const float beatPhase = (beatDurationFrames > 0)
        ? std::min(static_cast<float>(frameInBeat) / static_cast<float>(beatDurationFrames),
                   std::nextafter(1.0f, 0.0f))
        : 0.0f;
    const auto sub = static_cast<std::uint32_t>(std::min(
        static_cast<double>(beatPhase) * map.subdivisionsPerBeat,
        static_cast<double>(map.subdivisionsPerBeat - 1)));

    const std::uint64_t beatsPerBar = map.beatsPerBar;
    const std::uint64_t barsPerPhrase = map.barsPerPhrase;

    const std::uint64_t totalBeats = seg.startBeat + elapsedBeats;
    const auto beatInBar = static_cast<std::uint32_t>(elapsedBeats % beatsPerBar);
    const std::uint64_t elapsedBars = elapsedBeats / beatsPerBar;
    const std::uint64_t totalBars = seg.startBar + elapsedBars;
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
