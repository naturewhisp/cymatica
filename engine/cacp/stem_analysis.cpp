#include "stem_analysis.h"

#include "stft.h"

#include <algorithm>
#include <cmath>

namespace cymatica::cacp {

StemCapture::StemCapture(std::size_t capacityFrames) {
    for (std::size_t r = 0; r < kRoleCount; ++r) {
        stems_[r].assign(capacityFrames * 2, 0.0f);
        sink_.stems[r] = stems_[r].data();
    }
    busMix_.assign(capacityFrames * 2, 0.0f);
    sink_.busMix = busMix_.data();
    sink_.capacityFrames = capacityFrames;
}

std::size_t StemCapture::bytesAllocated() const noexcept {
    std::size_t b = busMix_.size() * sizeof(float);
    for (const auto& s : stems_) b += s.size() * sizeof(float);
    return b;
}

ReconstructionResult reconstructBusMix(const StemCapture& capture) noexcept {
    ReconstructionResult r;
    r.frames = capture.frames();
    const auto mix = capture.busMix();
    for (std::size_t i = 0; i < mix.size(); ++i) {
        // Same order and type as RoleBusProcessor::processAndMix
        const float sum = capture.stem(0)[i] + capture.stem(1)[i] + capture.stem(2)[i] + capture.stem(3)[i];
        if (!std::isfinite(sum) || !std::isfinite(mix[i])) {
            r.status = AnalysisStatus::NonFiniteInput;
            return r;
        }
        if (sum != mix[i]) {
            ++r.mismatchedSamples;
            r.maxAbsError = (std::max)(r.maxAbsError, std::abs(static_cast<double>(sum) - static_cast<double>(mix[i])));
        }
    }
    return r;
}

StemPresence measureStemPresence(std::span<const float> s, std::size_t startFrame, std::size_t endFrame) noexcept {
    StemPresence p;
    const std::size_t frames = s.size() / 2;
    if (startFrame >= endFrame || endFrame > frames) {
        p.status = AnalysisStatus::InvalidConfig;
        return p;
    }
    double sumSq = 0.0;
    double peak = 0.0;
    for (std::size_t i = startFrame * 2; i < endFrame * 2; ++i) {
        const double v = s[i];
        if (!std::isfinite(v)) {
            p.status = AnalysisStatus::NonFiniteInput;
            return p;
        }
        sumSq += v * v;
        peak = (std::max)(peak, std::abs(v));
    }
    if (peak == 0.0) {
        p.status = AnalysisStatus::ExactSilence;
        p.exactSilence = true;
        return p;
    }
    p.peakDbfs = 20.0 * std::log10(peak);
    p.rmsDbfs = 10.0 * std::log10(sumSq / static_cast<double>((endFrame - startFrame) * 2));
    return p;
}

std::vector<double> roleReferenceOnsetsS(std::span<const AppliedEventRecord> applied, audio::MusicRole role,
                                         std::uint32_t sampleRate, double mergeS) {
    std::vector<std::uint64_t> frames;
    for (const auto& rec : applied) {
        if (rec.id.type == audio::AudioEventType::NoteOn && rec.id.role == role &&
            rec.disposition == EventDisposition::Applied) {
            frames.push_back(rec.id.sampleFrame);
        }
    }
    std::sort(frames.begin(), frames.end());
    std::vector<double> out;
    const double fs = static_cast<double>(sampleRate);
    for (const auto f : frames) {
        const double t = static_cast<double>(f) / fs;
        if (!out.empty() && t - out.back() < mergeS) continue; // fused into the first NoteOn of the cluster
        out.push_back(t);
    }
    return out;
}

double percentileLinear(std::vector<double> v, double q) {
    if (v.empty()) return 0.0;
    std::sort(v.begin(), v.end());
    const double pos = q / 100.0 * static_cast<double>(v.size() - 1);
    const auto lo = static_cast<std::size_t>(std::floor(pos));
    const auto hi = (std::min)(lo + 1, v.size() - 1);
    return v[lo] + (v[hi] - v[lo]) * (pos - static_cast<double>(lo));
}

SignedErrorStats nearestSignedErrors(std::span<const double> referenceS, std::span<const double> estimateS, double windowS) {
    SignedErrorStats st;
    std::vector<double> est(estimateS.begin(), estimateS.end());
    std::sort(est.begin(), est.end());
    std::vector<double> errs;
    for (const double r : referenceS) {
        const auto it = std::lower_bound(est.begin(), est.end(), r);
        double best = windowS + 1.0;
        double signedErr = 0.0;
        if (it != est.end() && std::abs(*it - r) < best) { best = std::abs(*it - r); signedErr = *it - r; }
        if (it != est.begin() && std::abs(*(it - 1) - r) < best) { best = std::abs(*(it - 1) - r); signedErr = *(it - 1) - r; }
        if (best <= windowS) errs.push_back(signedErr);
    }
    st.pairs = errs.size();
    if (!errs.empty()) {
        st.medianS = percentileLinear(errs, 50.0);
        st.p05S = percentileLinear(errs, 5.0);
        st.p95S = percentileLinear(errs, 95.0);
    }
    return st;
}

std::vector<double> roleReleaseTimesS(std::span<const AppliedEventRecord> applied, audio::MusicRole role, std::uint32_t sampleRate) {
    std::vector<double> out;
    for (const auto& rec : applied) {
        if (rec.id.type == audio::AudioEventType::NoteOff && rec.id.role == role &&
            rec.disposition == EventDisposition::Applied) {
            out.push_back(static_cast<double>(rec.consumedFrame) / static_cast<double>(sampleRate));
        }
    }
    std::sort(out.begin(), out.end());
    return out;
}

RoleOnsetEvaluation evaluateRoleOnsets(std::span<const float> stemInterleaved, std::span<const double> referenceS,
                                       const RoleOnsetProfile& profile, std::uint32_t sampleRate,
                                       std::span<const double> releaseS) {
    RoleOnsetEvaluation ev;
    ev.references = referenceS.size();
    if (!std::isfinite(profile.offsetS) || !validTimeParam(std::abs(profile.offsetS))) {
        ev.status = AnalysisStatus::InvalidConfig;
        return ev;
    }
    const std::vector<float> mono = monoFromStereo(stemInterleaved);
    OnsetParams p{};
    p.delta = profile.delta;
    const OnsetDetection det = detectOnsetsSeconds(mono, p, sampleRate);
    if (det.status != AnalysisStatus::Ok) {
        ev.status = det.status;
        return ev;
    }
    std::vector<double> corrected = det.timesS;
    for (auto& t : corrected) t -= profile.offsetS;
    ev.detections = corrected.size();
    ev.at50 = matchOnsetsMaxCardinality(referenceS, corrected, 0.050);
    ev.at25 = matchOnsetsMaxCardinality(referenceS, corrected, 0.025);
    ev.errors = nearestSignedErrors(referenceS, corrected);

    std::vector<bool> matched(corrected.size(), false);
    std::vector<double> sortedEst = corrected;
    std::sort(sortedEst.begin(), sortedEst.end());
    for (const auto& pr : ev.at50.pairs) matched[pr.estimate] = true; // indices refer to the sorted estimates
    std::vector<double> refs(referenceS.begin(), referenceS.end());
    std::sort(refs.begin(), refs.end());
    std::vector<double> lags;
    for (std::size_t i = 0; i < sortedEst.size(); ++i) {
        if (matched[i]) continue;
        const auto it = std::upper_bound(refs.begin(), refs.end(), sortedEst[i]);
        if (it != refs.begin()) lags.push_back(sortedEst[i] - *(it - 1));
        const auto rel = std::lower_bound(releaseS.begin(), releaseS.end(), sortedEst[i] - 0.050);
        if (rel != releaseS.end() && *rel <= sortedEst[i] + 0.050) ++ev.releaseDetections;
    }
    const std::size_t counted = ev.detections - ev.releaseDetections;
    ev.precisionExclReleases = counted > 0 ? static_cast<double>(ev.at50.matches) / static_cast<double>(counted) : 0.0;
    ev.unmatchedLag.pairs = lags.size();
    if (!lags.empty()) {
        ev.unmatchedLag.medianS = percentileLinear(lags, 50.0);
        ev.unmatchedLag.p05S = percentileLinear(lags, 5.0);
        ev.unmatchedLag.p95S = percentileLinear(lags, 95.0);
    }
    return ev;
}

} // namespace cymatica::cacp
