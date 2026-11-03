#include "trama_diagnosis.h"

#include "onset.h"
#include "stft.h"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <limits>
#include <numeric>
#include <sstream>

namespace cymatica::cacp {

using nlohmann::json;

namespace {

constexpr std::uint32_t kFs = 48000;
constexpr std::size_t kTrama = 2;

json optNum(const std::optional<double>& v) { return (v && std::isfinite(*v)) ? json(*v) : json(nullptr); }

std::string fmt3(double v) {
    std::ostringstream s;
    s << std::fixed << std::setprecision(3) << v;
    return s.str();
}

// Curves kept for the CSV (not part of the compared measures)
struct Curves {
    std::vector<double> novelty{}, threshold{}, counterfactual{};
};

// Offset-corrected detection times of picked frames
std::vector<double> detectionTimes(const std::vector<std::size_t>& frames, double offsetS) {
    std::vector<double> t;
    t.reserve(frames.size());
    for (const auto f : frames) t.push_back(frameCentreSeconds(f, kDescriptorsV1Stft, kFs) - offsetS);
    return t;
}

std::vector<bool> matchedFlags(const std::vector<double>& refs, const std::vector<double>& est, AnalysisStatus& status) {
    std::vector<bool> flags(refs.size(), false);
    const OnsetScore sc = matchOnsetsMaxCardinality(refs, est, kDiagWindowS); // refs sorted
    status = sc.status;
    for (const auto& p : sc.pairs) flags[p.reference] = true;
    return flags;
}

bool nearAny(double t, const std::vector<double>& points, std::optional<double>* nearest = nullptr) {
    double best = std::numeric_limits<double>::infinity();
    for (const double p : points) best = (std::min)(best, std::abs(t - p));
    if (nearest != nullptr && std::isfinite(best)) *nearest = best;
    return best <= kDiagWindowS;
}

TramaRenderDiagnosis analyseRender(double releaseMs, const std::vector<SynthNote>& notes, const RoleOnsetProfile& profile,
                                   StemCapture& cap, Curves& curves) {
    TramaRenderDiagnosis d;
    d.releaseMs = releaseMs;
    d.releaseFrames = releaseOverrideFrames(releaseMs);
    const SyntheticRender sr = renderSyntheticWithRelease(audio::MusicRole::Trama, notes, cap, d.releaseFrames);
    d.captureInvalid = captureInvalidReason(sr.check);
    if (!d.captureInvalid.empty()) return d;

    const auto stem = cap.stem(kTrama);
    const std::vector<double> refs = roleReferenceOnsetsS(sr.applied, audio::MusicRole::Trama, kFs);
    const std::vector<double> releases = roleReleaseTimesS(sr.applied, audio::MusicRole::Trama, kFs);
    d.references = refs.size();
    if (refs.size() != notes.size()) {
        d.analysisStatus = "REFERENCES_MISMATCH"; // one reference per note is required to attach velocity and timbre
        return d;
    }

    // Detector of descriptors_v1 with the frozen Trama profile, step by step
    OnsetParams p{};
    p.delta = profile.delta;
    const MagnitudeSpectrogram mag = stftMagnitude(monoFromStereo(stem), kDescriptorsV1Stft);
    const NoveltyCurve nov = spectralFluxNovelty(mag, p.logGamma);
    if (nov.status != AnalysisStatus::Ok) {
        d.analysisStatus = analysisStatusName(nov.status);
        return d;
    }
    const ThresholdCurve thr = onsetThresholdCurve(nov.values, p, kDescriptorsV1Stft, kFs);
    const OnsetFrames picks = pickOnsetFrames(nov.values, p, kDescriptorsV1Stft, kFs);
    if (thr.status != AnalysisStatus::Ok || picks.status != AnalysisStatus::Ok) {
        d.analysisStatus = analysisStatusName(thr.status != AnalysisStatus::Ok ? thr.status : picks.status);
        return d;
    }
    AnalysisStatus matchStatus = AnalysisStatus::Ok;
    const std::vector<bool> matched = matchedFlags(refs, detectionTimes(picks.frames, profile.offsetS), matchStatus);
    if (matchStatus != AnalysisStatus::Ok) {
        d.analysisStatus = analysisStatusName(matchStatus);
        return d;
    }
    const RoleOnsetEvaluation ev = evaluateRoleOnsets(stem, refs, profile, kFs, releases);
    if (ev.status == AnalysisStatus::Ok) d.recallFromEvaluator = ev.at50.recall;

    const std::vector<double>& n = nov.values;
    const std::size_t frames = n.size();
    d.normaliser = nov.normaliser;
    auto corrected = [&](std::size_t k) { return frameCentreSeconds(k, kDescriptorsV1Stft, kFs) - profile.offsetS; };
    const auto gmax = static_cast<std::size_t>(std::max_element(n.begin(), n.end()) - n.begin());
    d.globalMaxTimeS = corrected(gmax);
    d.globalMaxAtRelease = nearAny(d.globalMaxTimeS, releases, &d.globalMaxNearestReleaseS);

    // Counterfactual normalisation: raw flux divided by its maximum outside +-50 ms of the Trama NoteOffs
    double cfMax = 0.0;
    for (std::size_t k = 0; k < frames; ++k) {
        if (!nearAny(corrected(k), releases)) cfMax = (std::max)(cfMax, n[k] * nov.normaliser);
    }
    d.counterfactualNormaliser = cfMax;
    std::vector<double> cf(frames, 0.0);
    std::vector<bool> cfMatched(refs.size(), false);
    if (cfMax > 0.0) {
        for (std::size_t k = 0; k < frames; ++k) cf[k] = n[k] * nov.normaliser / cfMax;
        const OnsetFrames cfPicks = pickOnsetFrames(cf, p, kDescriptorsV1Stft, kFs);
        AnalysisStatus cfStatus = cfPicks.status;
        if (cfStatus == AnalysisStatus::Ok) cfMatched = matchedFlags(refs, detectionTimes(cfPicks.frames, profile.offsetS), cfStatus);
        if (cfStatus != AnalysisStatus::Ok) {
            d.analysisStatus = analysisStatusName(cfStatus);
            return d;
        }
    }
    d.counterfactualMatched = static_cast<std::size_t>(std::count(cfMatched.begin(), cfMatched.end(), true));
    d.counterfactualRecall = static_cast<double>(d.counterfactualMatched) / static_cast<double>(refs.size());

    // Per reference: window peak, threshold, margin, local maximum and merged candidates
    std::vector<bool> picked(frames, false);
    for (const auto f : picks.frames) picked[f] = true;
    auto isLocalMax = [&](std::size_t k) { return k > 0 && k + 1 < frames && n[k] > n[k - 1] && n[k] >= n[k + 1]; };
    for (std::size_t i = 0; i < refs.size(); ++i) {
        TramaReferenceDiagnosis r;
        r.index = i;
        r.timeS = refs[i];
        r.velocity = notes[i].velocity;
        r.timbre = notes[i].timbre;
        if (i > 0) r.precedingIntervalS = notes[i].timeS - notes[i - 1].timeS;
        for (const double rel : releases) {
            if (rel < refs[i]) r.sincePrevNoteOffS = refs[i] - rel; // releases sorted: keeps the latest
        }
        r.matched = matched[i];
        r.counterfactualMatched = cfMatched[i];
        std::optional<std::size_t> best;
        for (std::size_t k = 0; k < frames; ++k) {
            if (std::abs(corrected(k) - refs[i]) > kDiagWindowS) continue;
            if (!best || n[k] > n[*best]) best = k;
            if (isLocalMax(k) && n[k] > thr.values[k] && !picked[k]) r.mergedCandidate = true;
        }
        if (best) {
            r.peakNorm = n[*best];
            r.peakRaw = n[*best] * nov.normaliser;
            r.thresholdAtPeak = thr.values[*best];
            r.marginAtPeak = n[*best] - thr.values[*best];
            r.peakIsLocalMax = isLocalMax(*best);
        }
        d.refs.push_back(r);
    }
    d.matched = static_cast<std::size_t>(std::count(matched.begin(), matched.end(), true));
    d.recall = static_cast<double>(d.matched) / static_cast<double>(refs.size());
    d.analysisStatus = "Ok";
    curves = {n, thr.values, cf};
    return d;
}

std::vector<std::size_t> missedIndices(const TramaRenderDiagnosis& d) {
    std::vector<std::size_t> out;
    for (const auto& r : d.refs) {
        if (!r.matched) out.push_back(r.index);
    }
    return out;
}

json renderJson(const TramaRenderDiagnosis& d) {
    json refs = json::array();
    for (const auto& r : d.refs) {
        refs.push_back({{"index", r.index}, {"time_s", r.timeS}, {"velocity", r.velocity}, {"timbre", r.timbre},
                        {"preceding_interval_s", optNum(r.precedingIntervalS)}, {"since_prev_noteoff_s", optNum(r.sincePrevNoteOffS)},
                        {"matched", r.matched}, {"peak_norm", optNum(r.peakNorm)}, {"peak_raw", optNum(r.peakRaw)},
                        {"threshold_at_peak", optNum(r.thresholdAtPeak)}, {"margin_at_peak", optNum(r.marginAtPeak)},
                        {"peak_is_local_max", r.peakIsLocalMax}, {"merged_candidate", r.mergedCandidate},
                        {"counterfactual_matched", r.counterfactualMatched}});
    }
    return {{"release_ms", d.releaseMs}, {"release_frames", d.releaseFrames}, {"capture_valid", d.captureInvalid.empty()},
            {"analysis_status", d.analysisStatus}, {"normaliser_raw_max", d.normaliser}, {"global_max_time_s", d.globalMaxTimeS},
            {"global_max_at_release", d.globalMaxAtRelease}, {"global_max_nearest_release_s", optNum(d.globalMaxNearestReleaseS)},
            {"references", d.references}, {"matched", d.matched}, {"recall", optNum(d.recall)},
            {"recall_from_evaluator", optNum(d.recallFromEvaluator)}, {"counterfactual_normaliser", d.counterfactualNormaliser},
            {"counterfactual_matched", d.counterfactualMatched}, {"counterfactual_recall", optNum(d.counterfactualRecall)},
            {"missed", missedIndices(d)}, {"refs", refs}};
}

json predictionJson(const PredictionOutcome& p) { return {{"outcome", p.outcome}, {"detail", p.detail}}; }

} // namespace

PredictionOutcome evaluateP1(const TramaRenderDiagnosis& at2, const TramaRenderDiagnosis& at30) {
    if (!at2.valid() || !at30.valid() || !(at30.normaliser > 0.0)) return {"INCONCLUSIVE", "invalid render or analysis"};
    const double ratio = at2.normaliser / at30.normaliser;
    std::ostringstream d;
    d << "global maximum at 2 ms " << (at2.globalMaxAtRelease ? "on" : "not on") << " a Trama release; max F(2 ms) / max F(30 ms) = "
      << fmt3(ratio);
    return {(at2.globalMaxAtRelease && ratio >= kP1MinNormaliserRatio) ? "SUPPORTED" : "NOT_SUPPORTED", d.str()};
}

PredictionOutcome evaluateP2(const TramaRenderDiagnosis& at2, const TramaRenderDiagnosis& at30) {
    if (!at2.valid() || !at30.valid() || !at2.counterfactualRecall || !at30.counterfactualRecall) {
        return {"INCONCLUSIVE", "invalid render or analysis"};
    }
    std::ostringstream d;
    d << "counterfactual recall 2 ms = " << fmt3(*at2.counterfactualRecall) << " (" << at2.counterfactualMatched << "/"
      << at2.references << "); control 30 ms = " << fmt3(*at30.counterfactualRecall);
    if (at30.counterfactualMatched != at30.references) return {"INCONCLUSIVE", d.str() + "; control failed"};
    return {*at2.counterfactualRecall >= kP2MinRecall ? "SUPPORTED" : "NOT_SUPPORTED", d.str()};
}

PredictionOutcome evaluateP3(const TramaRenderDiagnosis& at2) {
    if (!at2.valid()) return {"INCONCLUSIVE", "invalid render or analysis"};
    const auto missed = missedIndices(at2);
    if (missed.size() != kExpectedMissed) {
        return {"INCONCLUSIVE", std::to_string(missed.size()) + " missed references, the rule is fixed for " +
                                    std::to_string(kExpectedMissed)};
    }
    // Rank by raw attack peak; a reference without a peak ranks lowest
    std::vector<std::size_t> order(at2.refs.size());
    std::iota(order.begin(), order.end(), std::size_t{0});
    auto key = [&](std::size_t i) { return at2.refs[i].peakRaw.value_or(-std::numeric_limits<double>::infinity()); };
    std::stable_sort(order.begin(), order.end(), [&](std::size_t a, std::size_t b) { return key(a) < key(b); });
    std::size_t among = 0;
    for (std::size_t k = 0; k < kP3LowestCount && k < order.size(); ++k) {
        if (!at2.refs[order[k]].matched) ++among;
    }
    std::ostringstream d;
    d << among << " of " << missed.size() << " missed references among the " << kP3LowestCount << " lowest raw attack peaks";
    return {among >= kP3MinMissedAmongLowest ? "SUPPORTED" : "NOT_SUPPORTED", d.str()};
}

PredictionOutcome evaluateP4(const TramaRenderDiagnosis& at2) {
    if (!at2.valid()) return {"INCONCLUSIVE", "invalid render or analysis"};
    std::size_t merged = 0;
    for (const auto& r : at2.refs) {
        if (!r.matched && r.mergedCandidate) ++merged;
    }
    const std::string d = std::to_string(merged) + " missed references with an above-threshold local maximum dropped by the merge";
    if (merged == 0) return {"NOT_SUPPORTED", d};
    return {merged >= kP4MinMerged ? "SUPPORTED" : "INCONCLUSIVE", d};
}

PredictionOutcome evaluateCausal(const PredictionOutcome& p1, const PredictionOutcome& p2, const PredictionOutcome& p3,
                                 const PredictionOutcome& p4) {
    if (p2.outcome == "NOT_SUPPORTED") return {"NOT_SUPPORTED", "the counterfactual normalisation does not recover the missed references"};
    if (p1.outcome == "SUPPORTED" && p2.outcome == "SUPPORTED" && p3.outcome == "SUPPORTED" && p4.outcome != "SUPPORTED") {
        return {"SUPPORTED", "M-NORM: global flux normalisation set by the release transients, with weak attacks"};
    }
    return {"INCONCLUSIVE", "P1 " + p1.outcome + ", P2 " + p2.outcome + ", P3 " + p3.outcome + ", P4 " + p4.outcome};
}

TramaDiagnosisResult runTramaMissDiagnosisV1(const std::string& outputDir, bool writeFiles) {
    Curves c2, c30;
    auto runOnce = [&](Curves& k2, Curves& k30) {
        TramaDiagnosisResult r;
        const auto frozen = frozenStemProfileV1();
        if (!frozen) {
            r.status = "INCONCLUSIVE";
            r.reasons.emplace_back("frozen onset profile unavailable");
            return r;
        }
        const auto notes = syntheticNotesSetB(kTrama);
        StemCapture cap(48000 * 30);
        r.at2 = analyseRender(2.0, notes, frozen->roles[kTrama], cap, k2);
        r.at30 = analyseRender(kH1ReleaseMs, notes, frozen->roles[kTrama], cap, k30);
        for (const auto* d : {&r.at2, &r.at30}) {
            const std::string tag = fmt3(d->releaseMs) + " ms: ";
            if (!d->captureInvalid.empty()) r.reasons.push_back(tag + "invalid capture: " + d->captureInvalid);
            else if (d->analysisStatus != "Ok") r.reasons.push_back(tag + "analysis " + d->analysisStatus);
            else if (!d->recallFromEvaluator || std::abs(*d->recallFromEvaluator - *d->recall) > 1e-12) {
                r.reasons.push_back(tag + "step-by-step recall differs from evaluateRoleOnsets");
            }
        }
        r.p1 = evaluateP1(r.at2, r.at30);
        r.p2 = evaluateP2(r.at2, r.at30);
        r.p3 = evaluateP3(r.at2);
        r.p4 = evaluateP4(r.at2);
        r.causal = evaluateCausal(r.p1, r.p2, r.p3, r.p4);
        r.status = r.reasons.empty() ? "MEASURED" : "INCONCLUSIVE";
        return r;
    };
    TramaDiagnosisResult r = runOnce(c2, c30);
    Curves ignore2, ignore30;
    const TramaDiagnosisResult again = runOnce(ignore2, ignore30);
    r.repeatIdentical = again.measuresJson() == r.measuresJson();
    if (!r.repeatIdentical) {
        r.reasons.emplace_back("repeat run differs");
        r.status = "INCONCLUSIVE";
    }
    if (writeFiles) {
        namespace fs = std::filesystem;
        r.curvesFilePath = (fs::path(outputDir) / "trama_miss_diagnosis_v1_curves.csv").lexically_normal().string();
        std::ofstream cf(r.curvesFilePath);
        r.curvesWriteSuccess = cf.is_open();
        if (r.curvesWriteSuccess) {
            cf << "frame,time_s,novelty_2ms,threshold_2ms,counterfactual_2ms,novelty_30ms,threshold_30ms\n" << std::setprecision(9);
            const std::size_t rows = (std::min)(c2.novelty.size(), c30.novelty.size());
            for (std::size_t k = 0; k < rows; ++k) {
                cf << k << ',' << frameCentreSeconds(k, kDescriptorsV1Stft, kFs) << ',' << c2.novelty[k] << ',' << c2.threshold[k] << ','
                   << c2.counterfactual[k] << ',' << c30.novelty[k] << ',' << c30.threshold[k] << '\n';
            }
            r.curvesWriteSuccess = cf.good();
        }
        r.manifestFilePath = (fs::path(outputDir) / "trama_miss_diagnosis_v1_manifest.json").lexically_normal().string();
        std::ofstream mf(r.manifestFilePath);
        r.manifestWriteSuccess = mf.is_open();
        if (r.manifestWriteSuccess) {
            mf << r.toJson();
            r.manifestWriteSuccess = mf.good();
        }
    }
    return r;
}

std::string TramaDiagnosisResult::measuresJson() const {
    const json j{{"render_2ms", renderJson(at2)},
                 {"render_30ms", renderJson(at30)},
                 {"predictions", {{"p1_normaliser_set_by_releases", predictionJson(p1)},
                                  {"p2_counterfactual_normalisation", predictionJson(p2)},
                                  {"p3_weak_attacks", predictionJson(p3)},
                                  {"p4_min_separation_merge", predictionJson(p4)},
                                  {"causal_m_norm", predictionJson(causal)}}}};
    return j.dump();
}

std::string TramaDiagnosisResult::toJson() const {
    json j = json::parse(measuresJson());
    j["manifest_version"] = "1.0";
    j["scenario"] = "trama_miss_diagnosis_v1";
    j["build_id"] = CYMATICA_BUILD_ID;
    j["verdict"] = status;
    j["gate"] = false;
    j["reasons"] = reasons;
    j["repeat_identical"] = repeatIdentical;
    j["curves_file"] = curvesFilePath;
    j["rules"] = {
        {"source", "ADR-0006 5e.3b, fixed before measuring"},
        {"detector", "descriptors_v1 unchanged, frozen stem_onset_profile_v1 (Trama delta and offset); +-50 ms windows"},
        {"p1", "SUPPORTED when the 2 ms global flux maximum is within +-50 ms of a Trama NoteOff and max F(2 ms) >= 1.2 max F(30 ms)"},
        {"p2", "SUPPORTED when the 2 ms recall after renormalising by the max outside +-50 ms of the NoteOffs is >= 0.95; "
               "the 30 ms control must keep 1.000"},
        {"p3", "SUPPORTED when >= 8 of the 10 references missed at 2 ms are among the 12 lowest raw attack peaks"},
        {"p4", "NOT_SUPPORTED with no missed reference having a dropped above-threshold local maximum; SUPPORTED with >= 5"},
        {"causal", "SUPPORTED when P1, P2, P3 SUPPORTED and P4 not; NOT_SUPPORTED when P2 NOT_SUPPORTED; else INCONCLUSIVE"},
        {"provenance", "hypothesis formed a posteriori from 5e.3 data (velocity and timbre of the missed notes); those "
                       "associations are not a test"}};
    return j.dump(2) + "\n";
}

} // namespace cymatica::cacp
