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
    const auto boundary = core::multiplyDivide(elapsedBeats, K_num, K_den);

    // Explicit Policy (§7.5, DIF-M2-02/03, DIF-M2-R3-03):
    // 1. Target frame must fall exactly on a beat boundary (reconciled with frameAtBeat)
    if (deltaF != boundary.value) {
        return false;
    }
    const std::uint64_t beatsInBarBefore = prevSeg.beatOffsetInBar + elapsedBeats;

    // 2. A meter change (time signature / beatsPerBar) must fall on a bar boundary
    const bool meterChanged = (map.beatsPerBar != prevSeg.tempoMap.beatsPerBar ||
                               map.timeSignatureDen != prevSeg.tempoMap.timeSignatureDen ||
                               map.timeSignatureNum != prevSeg.tempoMap.timeSignatureNum);
    if (meterChanged && (beatsInBarBefore % prevSeg.tempoMap.beatsPerBar != 0)) {
        return false; // Meter change intra-measure is rejected
    }

    const std::uint32_t newBeatOffsetInBar = meterChanged
        ? 0
        : static_cast<std::uint32_t>(beatsInBarBefore % prevSeg.tempoMap.beatsPerBar);
    const std::uint64_t barsElapsed = beatsInBarBefore / prevSeg.tempoMap.beatsPerBar;
    const std::uint64_t totalBars = prevSeg.startBar + barsElapsed;
    const std::uint64_t totalBeats = prevSeg.startBeat + elapsedBeats;

    segments_[segmentCount_] = TempoSegment{
        .startFrame = targetSampleFrame,
        .startBeat = totalBeats,
        .startBar = totalBars,
        .beatOffsetInBar = newBeatOffsetInBar,
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
    const std::uint64_t deltaBeats = totalBeats >= seg.startBeat ? (totalBeats - seg.startBeat) : 0;
    const std::uint64_t K_num = 60ULL * sampleRate_ * seg.tempoMap.bpm.denominator * 4ULL;
    const std::uint64_t K_den = static_cast<std::uint64_t>(seg.tempoMap.bpm.numerator) * seg.tempoMap.timeSignatureDen;

    const std::uint64_t deltaFrames = core::multiplyDivide(deltaBeats, K_num, K_den).value;
    return core::saturatingAdd(seg.startFrame, deltaFrames);
}

std::uint64_t MusicClock::frameAtBeatStep(std::uint64_t totalBeats,
                                          std::uint64_t stepIndex,
                                          std::uint32_t stepsPerBeat) const noexcept {
    if (stepsPerBeat == 0) return frameAtBeat(totalBeats);
    if (segmentCount_ == 0 || sampleRate_ == 0) return 0;

    const std::uint64_t wholeBeats = core::saturatingAdd(totalBeats, stepIndex / stepsPerBeat);
    const std::uint64_t remSteps = stepIndex % stepsPerBeat;

    std::size_t segIdx = 0;
    for (std::size_t i = 1; i < segmentCount_; ++i) {
        if (segments_[i].startBeat <= wholeBeats) {
            segIdx = i;
        } else {
            break;
        }
    }

    const auto& seg = segments_[segIdx];
    const std::uint64_t deltaBeats = wholeBeats >= seg.startBeat ? (wholeBeats - seg.startBeat) : 0;
    const std::uint64_t deltaSteps = core::saturatingAdd(core::saturatingMul(deltaBeats, stepsPerBeat), remSteps);
    const std::uint64_t K_num = 60ULL * sampleRate_ * seg.tempoMap.bpm.denominator * 4ULL;
    const std::uint64_t K_den = static_cast<std::uint64_t>(seg.tempoMap.bpm.numerator) *
                                seg.tempoMap.timeSignatureDen * stepsPerBeat;

    const std::uint64_t deltaFrames = core::multiplyDivide(deltaSteps, K_num, K_den).value;
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

    // If segIdx started intra-bar (beatOffsetInBar != 0) and totalBars == seg.startBar,
    // then the bar totalBars actually started in the previous segment!
    while (segIdx > 0 && segments_[segIdx].startBar == totalBars && segments_[segIdx].beatOffsetInBar != 0) {
        --segIdx;
    }

    const auto& seg = segments_[segIdx];
    const std::uint64_t beatsPerBar = seg.tempoMap.beatsPerBar;
    if (beatsPerBar == 0) return seg.startFrame;

    const std::uint64_t deltaBars = (totalBars >= seg.startBar) ? (totalBars - seg.startBar) : 0;
    const std::uint64_t beatsToNextBar = (seg.beatOffsetInBar == 0) ? 0 : (beatsPerBar - seg.beatOffsetInBar);
    const std::uint64_t remainingBars = (deltaBars == 0) ? 0 : (deltaBars - (seg.beatOffsetInBar == 0 ? 0 : 1));

    // Overflow-safe saturating arithmetic (DIF-M2-review)
    const std::uint64_t deltaBeats = (deltaBars == 0)
        ? 0
        : core::saturatingAdd(beatsToNextBar, core::saturatingMul(remainingBars, beatsPerBar));

    const std::uint64_t K_num = 60ULL * sampleRate_ * seg.tempoMap.bpm.denominator * 4ULL;
    const std::uint64_t K_den = static_cast<std::uint64_t>(seg.tempoMap.bpm.numerator) * seg.tempoMap.timeSignatureDen;

    const std::uint64_t deltaFrames = core::multiplyDivide(deltaBeats, K_num, K_den).value;
    return core::saturatingAdd(seg.startFrame, deltaFrames);
}

std::uint64_t MusicClock::frameAtPhrase(std::uint64_t totalPhrases) const noexcept {
    if (segmentCount_ == 0 || sampleRate_ == 0) return 0;
    const auto& seg = segments_[0];
    const std::uint64_t totalBars = core::multiplyDivide(totalPhrases, seg.tempoMap.barsPerPhrase, 1).value;
    return frameAtBar(totalBars);
}

std::uint64_t MusicClock::nextBeatBoundary(std::uint64_t frame) const noexcept {
    if (segmentCount_ == 0 || sampleRate_ == 0) return frame;
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
    if (map.bpm.numerator == 0 || map.bpm.denominator == 0 || map.timeSignatureDen == 0) return frame;

    const std::uint64_t deltaF = frame - seg.startFrame;
    const std::uint64_t K_num = 60ULL * sampleRate_ * map.bpm.denominator * 4ULL;
    const std::uint64_t K_den = static_cast<std::uint64_t>(map.bpm.numerator) * map.timeSignatureDen;

    const auto inverse = core::multiplyDivide(deltaF, K_den, K_num);
    const std::uint64_t elapsedBeats = inverse.value +
        static_cast<std::uint64_t>(inverse.remainder >= K_num - (K_den - 1));
    const auto boundary = core::multiplyDivide(elapsedBeats, K_num, K_den);

    const std::uint64_t totalBeats = seg.startBeat + elapsedBeats;
    if (deltaF == boundary.value) {
        return frame; // Exactly on beat boundary
    }
    return frameAtBeat(core::saturatingAdd(totalBeats, 1ULL));
}

std::uint64_t MusicClock::nextBarBoundary(std::uint64_t frame) const noexcept {
    if (segmentCount_ == 0 || sampleRate_ == 0) return frame;
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
    if (map.beatsPerBar == 0 || map.bpm.numerator == 0 || map.bpm.denominator == 0 || map.timeSignatureDen == 0) return frame;

    const std::uint64_t deltaF = frame - seg.startFrame;
    const std::uint64_t K_num = 60ULL * sampleRate_ * map.bpm.denominator * 4ULL;
    const std::uint64_t K_den = static_cast<std::uint64_t>(map.bpm.numerator) * map.timeSignatureDen;

    const auto inverse = core::multiplyDivide(deltaF, K_den, K_num);
    const std::uint64_t elapsedBeats = inverse.value +
        static_cast<std::uint64_t>(inverse.remainder >= K_num - (K_den - 1));
    const auto boundary = core::multiplyDivide(elapsedBeats, K_num, K_den);

    const std::uint64_t totalBeats = seg.startBeat + elapsedBeats;
    const std::uint64_t beatsInBar = seg.beatOffsetInBar + elapsedBeats;
    const std::uint64_t beatOffset = beatsInBar % map.beatsPerBar;

    if (beatOffset == 0 && deltaF == boundary.value) {
        return frame; // Exactly on bar boundary
    }
    const std::uint64_t beatsToNextBar = map.beatsPerBar - beatOffset;
    return frameAtBeat(core::saturatingAdd(totalBeats, beatsToNextBar));
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

    const std::uint64_t beatsInBar = seg.beatOffsetInBar + elapsedBeats;
    const auto beatInBar = static_cast<std::uint32_t>(beatsInBar % beatsPerBar);
    const std::uint64_t barsInSeg = beatsInBar / beatsPerBar;
    const std::uint64_t totalBars = seg.startBar + barsInSeg;
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
    // Ceiling division so an event is never triggered before its logical internal time (DIF-M2-review)
    const auto res = core::multiplyDivide(internalFrames, deviceRate, sampleRate_);
    return core::saturatingAdd(res.value, static_cast<std::uint64_t>(res.remainder > 0));
}

} // namespace cymatica::audio
