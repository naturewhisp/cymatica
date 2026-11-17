#include <catch2/catch_test_macros.hpp>

#include "discontinuity.h"

#include <cmath>
#include <vector>

using namespace cymatica::cacp;

namespace {
std::vector<float> sine(std::size_t n, double amp = 0.5) {
    std::vector<float> x(n);
    for (std::size_t i = 0; i < n; ++i) x[i] = static_cast<float>(amp * std::sin(2.0 * 3.14159265358979323846 * 440.0 * static_cast<double>(i) / 48000.0));
    return x;
}
} // namespace

TEST_CASE("package 2: a pure sine has no discontinuity, an added impulse is found where it is", "[discontinuity]") {
    auto x = sine(48000);
    REQUIRE(discontinuityCandidates(x).empty());
    x[20000] += 0.01f;   // -40 dBFS impulse
    const auto c = discontinuityCandidates(x);
    REQUIRE(c.size() == 1);
    REQUIRE(c[0] == 20000);
}

TEST_CASE("package 2: candidates within 2 ms are fused into the first one", "[discontinuity]") {
    auto x = sine(48000);
    x[10000] += 0.1f;
    x[10050] += 0.1f;    // 50 frames later: same cluster
    x[30000] += 0.1f;
    const auto c = discontinuityCandidates(x);
    REQUIRE(c.size() == 2);
    REQUIRE(c[0] == 10000);
    REQUIRE(c[1] == 30000);
}

TEST_CASE("package 2: A2 finds only what differs between two renders", "[discontinuity]") {
    std::vector<float> a(2 * 48000);
    const auto m = sine(48000);
    for (std::size_t i = 0; i < m.size(); ++i) { a[2 * i] = m[i]; a[2 * i + 1] = m[i]; }
    REQUIRE(differentialCandidates(a, a).empty());
    auto b = a;
    b[2 * 12345 + 1] += 0.01f;
    const auto d = differentialCandidates(a, b);
    REQUIRE(d.size() == 1);
    REQUIRE(d[0] == 12345);
    REQUIRE(differentialCandidates(a, std::vector<float>(10)).empty());   // length mismatch: nothing claimed
}

TEST_CASE("package 2: candidates are annotated with the nearest applied event of the role within 5 ms", "[discontinuity]") {
    std::vector<AppliedEventRecord> log(2);
    log[0].id.type = cymatica::audio::AudioEventType::NoteOn;
    log[0].id.role = cymatica::audio::MusicRole::Pulso;
    log[0].id.sampleFrame = 1000;
    log[1].id.type = cymatica::audio::AudioEventType::NoteOff;
    log[1].id.role = cymatica::audio::MusicRole::Corpo;
    log[1].id.sampleFrame = 5000;
    REQUIRE(annotateCandidate(1100, log, 0) == CandidateCategory::Attack);
    REQUIRE(annotateCandidate(1300, log, 0) == CandidateCategory::Unexplained);   // 300 frames > 5 ms
    REQUIRE(annotateCandidate(5100, log, 0) == CandidateCategory::Unexplained);   // Corpo event, Pulso filter
    REQUIRE(annotateCandidate(5100, log, -1) == CandidateCategory::Release);
}
