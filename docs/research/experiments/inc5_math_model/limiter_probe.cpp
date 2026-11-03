// Analysis tool (outside the repository): check AG-MM-10 on the real MasterLimiter of df0bcbe.
#include "audio_probe.h"
#include "master_limiter.h"

#include <cmath>
#include <cstdio>
#include <vector>

using namespace cymatica::audio;

static void run(const char* name, double amp, double freq, double phase, std::size_t frames) {
    std::vector<float> x(2 * frames);
    for (std::size_t n = 0; n < frames; ++n) {
        const float v = static_cast<float>(amp * std::sin(2.0 * 3.14159265358979323846 * freq * n / 48000.0 + phase));
        x[2 * n] = v;
        x[2 * n + 1] = v;
    }
    const float tpIn = analysis::estimateTruePeakDbtp(x.data(), frames, 2);
    MasterLimiter lim;
    lim.init(48000);
    lim.processBlock(x.data(), static_cast<std::uint32_t>(frames));
    float samplePeak = 0.0f;
    for (const float v : x) samplePeak = std::max(samplePeak, std::abs(v));
    const float tpOut = analysis::estimateTruePeakDbtp(x.data(), frames, 2);
    // steady-state part only (skip the first 4800 frames)
    const float tpSteady = analysis::estimateTruePeakDbtp(x.data() + 2 * 4800, frames - 4800, 2);
    std::printf("%-44s TP in %+6.2f dBTP | out: sample peak %+6.2f dBFS, TP %+6.2f dBTP (steady %+6.2f), max GR %.2f dB\n", name, tpIn,
                20.0 * std::log10(samplePeak), tpOut, tpSteady, lim.maxGainReductionDb());
}

int main() {
    const double C = std::pow(10.0, -1.0 / 20.0);
    // AG-MM-10 case: fs/4 sine, phase -pi/4 -> samples {+C,+C,-C,-C}, i.e. amplitude C*sqrt(2)
    run("fs/4, phase -pi/4, samples at +-C (-1 dBFS)", C * std::sqrt(2.0), 12000.0, -3.14159265358979323846 / 4.0, 48000);
    run("fs/4, phase 0, amplitude C", C, 12000.0, 0.0, 48000);
    run("11.9 kHz, amplitude 1.0", 1.0, 11900.0, 0.3, 48000);
    run("1 kHz, amplitude 1.0", 1.0, 1000.0, 0.0, 48000);
    run("fs/4, phase -pi/4, amplitude 2.0 (overdrive)", 2.0, 12000.0, -3.14159265358979323846 / 4.0, 48000);
    return 0;
}
