#pragma once

#include "stft.h"

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace cymatica::cacp {

// Onset detection of descriptors_v1 (ADR-0006 D-M2-14; Bello et al. 2005, Dixon 2006). Parameters fixed a priori
// in the CACP methods dossier §3.2; delta and the constant offset are calibrated per role in 5c, never here.
struct OnsetParams {
    double logGamma{100.0};          // log(1 + gamma |X|) compression
    double thresholdHalfWindowS{0.100}; // moving-average threshold half window
    double delta{0.10};              // threshold offset on the novelty normalised to max 1
    double minSeparationS{0.030};    // detections closer than this are merged, keeping the larger peak
};

// Spectral flux of log(1 + gamma |X|): per frame, sum over bins of the half-wave rectified frame difference;
// frame 0 is 0; normalised to max 1 (all-zero novelty stays zero). The status of the spectrogram propagates:
// an invalid spectrogram gives no values, never an empty curve presented as valid.
struct NoveltyCurve {
    AnalysisStatus status{AnalysisStatus::Ok};
    std::vector<double> values{};
};

[[nodiscard]] NoveltyCurve spectralFluxNovelty(const MagnitudeSpectrogram& mag, double logGamma);

// Peak picking: local maximum (n[i] > n[i-1] and n[i] >= n[i+1]) above the zero-padded centred moving average
// of 2w+1 frames plus delta, w = round(halfWindow * fs / hop); detections closer than minSeparation merged.
// Returns frame indices; a non-finite novelty value gives NonFiniteInput and no frame.
struct OnsetFrames {
    AnalysisStatus status{AnalysisStatus::Ok};
    std::vector<std::size_t> frames{};
};

[[nodiscard]] OnsetFrames pickOnsetFrames(std::span<const double> novelty, const OnsetParams& p, const StftConfig& cfg,
                                          std::uint32_t sampleRate);

// Detected onset times (frame centres, seconds) of a mono signal. An empty list is a valid "no onset" only with
// status Ok (e.g. silence); NaN/inf input gives NonFiniteInput (DIF-5b-02).
struct OnsetDetection {
    AnalysisStatus status{AnalysisStatus::Ok};
    std::vector<double> timesS{};
};

[[nodiscard]] OnsetDetection detectOnsetsSeconds(std::span<const float> mono, const OnsetParams& p,
                                                 std::uint32_t sampleRate, const StftConfig& cfg = kDescriptorsV1Stft);

struct OnsetMatch {
    std::size_t reference{0};
    std::size_t estimate{0};
    double errorS{0.0}; // estimate - reference
};

struct OnsetScore {
    AnalysisStatus status{AnalysisStatus::Ok}; // NonFiniteInput when a time is NaN/inf: no pair, P = R = F = 0
    std::size_t references{0};
    std::size_t estimates{0};
    std::size_t matches{0};
    double precision{0.0};
    double recall{0.0};
    double fMeasure{0.0};
    std::vector<OnsetMatch> pairs{};
};

// One-to-one matching of maximum cardinality with an inclusive tolerance |e - r| <= tolerance (mir_eval
// convention, Raffel et al. 2014). Both lists are sorted internally. On a line with a symmetric window the
// earliest-feasible two-pointer assignment is maximum.
[[nodiscard]] OnsetScore matchOnsetsMaxCardinality(std::span<const double> referenceS, std::span<const double> estimateS,
                                                   double toleranceS = 0.050);

// Greedy matching of the E1 research script (each reference, in time order, takes the closest unused estimate
// within the tolerance). Kept only to reproduce E1 for the parity record; not used by any gate.
[[nodiscard]] OnsetScore matchOnsetsGreedyNearest(std::span<const double> referenceS, std::span<const double> estimateS,
                                                  double toleranceS = 0.050);

} // namespace cymatica::cacp
