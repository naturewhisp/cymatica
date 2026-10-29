#pragma once

#include "audio_control.h"
#include "event_chain_log.h"
#include "input_dispatcher.h"

#include <array>
#include <atomic>
#include <cstdint>
#include <string>
#include <vector>

namespace cymatica::audio {

class AudioEngine;

// AudioProbe lock-free SPSC PCM tap (Spec §31.5, ADR-0003 D-AO-02).
// Producer is the realtime audio callback (processBlock).
// Consumer is an asynchronous worker or test diagnostic runner.
// Invariants: zero heap allocation, zero blocking locks, bounded execution in realtime path.
#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable: 4324) // structure was padded due to alignment specifier
#endif
class AudioProbe {
public:
    static constexpr std::uint32_t kDefaultCapacityFrames = 24000; // 0.5 s @ 48 kHz stereo = 192 KB

    explicit AudioProbe(std::uint32_t capacityFrames = kDefaultCapacityFrames,
                        std::uint32_t channels = 2,
                        std::uint32_t sampleRate = 48000);
    ~AudioProbe() = default;

    AudioProbe(const AudioProbe&) = delete;
    AudioProbe& operator=(const AudioProbe&) = delete;

    // Realtime producer path: called from AudioEngine::Impl::processBlock.
    // If buffer is full, increments droppedFrames counter without blocking or allocating.
    void writeFrames(const float* pInterleavedInput, std::uint32_t frameCount) noexcept;

    // Consumer path: called from worker / diagnostic runner thread.
    // Reads up to maxFrames, returning the number of frames actually read.
    std::uint32_t readFrames(float* pInterleavedOutput, std::uint32_t maxFrames) noexcept;

    [[nodiscard]] std::uint64_t droppedFrames() const noexcept;
    [[nodiscard]] std::uint32_t availableFrames() const noexcept;
    [[nodiscard]] std::uint32_t capacityFrames() const noexcept;
    [[nodiscard]] std::uint32_t channels() const noexcept;
    [[nodiscard]] std::uint32_t sampleRate() const noexcept;

    void reset() noexcept;

private:
    const std::uint32_t capacityFrames_;
    const std::uint32_t channels_;
    const std::uint32_t sampleRate_;
    const std::uint32_t capacitySamples_; // capacityFrames_ * channels_

    std::vector<float> ringBuffer_;

    // SPSC cursors
    alignas(64) std::atomic<std::uint64_t> writeHead_{0};
    alignas(64) std::atomic<std::uint64_t> readHead_{0};
    alignas(64) std::atomic<std::uint64_t> droppedFrames_{0};
};
#if defined(_MSC_VER)
#pragma warning(pop)
#endif

// Quantitative analysis utilities (ADR-0003 D-AO-02)
namespace analysis {

// Computes Root-Mean-Square in dBFS (0 dBFS = full scale 1.0 sine/square)
float computeRmsDbfs(const float* pSamples, std::size_t sampleCount) noexcept;

// Estimates fundamental frequency in Hz using normalized autocorrelation with parabolic interpolation.
float estimatePitchHz(const float* pSamples,
                      std::size_t sampleCount,
                      std::uint32_t sampleRate,
                      float minFreqHz = 80.0f,
                      float maxFreqHz = 1000.0f) noexcept;

// Writes 16-bit PCM stereo WAV file
bool writeWavStereo16(const std::string& filePath,
                      const float* pInterleavedSamples,
                      std::size_t frameCount,
                      std::uint32_t sampleRate);

// Estimates True Peak level linearly (1.0 = 0 dBTP) using standard 4x oversampled polyphase FIR
// (ITU-R BS.1770-4/-5 Annex 2, 48-tap 4x polyphase FIR; estimate, not an exact reconstruction)
float estimateTruePeakLinear(const float* pInterleavedSamples,
                             std::size_t frameCount,
                             std::uint32_t channels) noexcept;

// Estimates True Peak level in dBTP (0 dBTP = 1.0 linear peak)
float estimateTruePeakDbtp(const float* pInterleavedSamples,
                           std::size_t frameCount,
                           std::uint32_t channels) noexcept;

} // namespace analysis

#ifndef CYMATICA_BUILD_ID
#define CYMATICA_BUILD_ID "unknown"
#endif

// Profile metrics for tone_probe_v0 (Spec §31.5, ADR-0003 D-AO-02)
struct PhaseMetric {
    std::uint32_t startMs{0};
    std::uint64_t startSampleFrame{0};
    std::uint32_t durationMs{250};
    float targetFreqHz{220.0f};
    float estimatedFreqHz{0.0f};
    float rmsDbfs{-120.0f};
    bool freqPassed{false};
    bool rmsPassed{false};
    bool passed{false};
};

struct ToneProbeResult {
    bool passed{false};
    PhaseMetric phase1Baseline{};
    PhaseMetric phase2Hold{};
    PhaseMetric phase3Release{};
    std::uint64_t totalCapturedFrames{0};
    std::uint64_t droppedFrames{0};
    std::string buildId{CYMATICA_BUILD_ID};
    std::uint64_t transportEpoch{1};
    std::uint64_t simulationTicks{360};
    std::uint64_t requestedSampleFrames{144000};
    std::uint64_t appliedSampleFrames{144000};
    std::string presentationQuality{"Estimated"};
    bool wavWriteSuccess{false};
    bool manifestWriteSuccess{false};
    std::string wavFilePath{};
    std::string manifestFilePath{};
    std::string failureReason{};

    [[nodiscard]] std::string toJson() const;
};

// Runs the authoritative tone_probe_v0 scenario offline in 120 Hz simulation ticks
ToneProbeResult runToneProbeV0Offline(const std::string& outputDir = "artifacts/cacp",
                                      bool writeFiles = true);

// Runs tone_probe_v0 scenario against an already initialized AudioEngine and InputDispatcher
ToneProbeResult runToneProbeV0(AudioEngine& engine,
                               cymatica::core::InputDispatcher& dispatcher,
                               const std::string& outputDir = "artifacts/cacp",
                               bool writeFiles = true);

// Profile metrics for voice_probe_v1 (ADR-0003 D-M2-03, ADR-0004 AUD-D-01)
struct VoiceProbeResult {
    bool passed{false};
    std::uint64_t totalCapturedFrames{0};
    std::uint64_t droppedFrames{0};
    std::uint32_t voiceStealsDetected{0};
    std::uint32_t activeVoicesPhase1{0};
    bool stealContinuityPassed{false};
    bool pauseSilencePassed{false};
    bool resumePassed{false};
    bool stopSilencePassed{false};
    bool dspAnomaliesPassed{false};
    std::uint32_t clippedSamples{0};
    float peakAmplitude{0.0f};
    bool headroomPassed{false};
    std::string buildId{CYMATICA_BUILD_ID};
    std::uint64_t transportEpoch{1};
    std::uint64_t simulationTicks{300};
    std::uint64_t requestedSampleFrames{120000};
    std::string presentationQuality{"Estimated"};
    bool wavWriteSuccess{false};
    bool manifestWriteSuccess{false};
    std::string wavFilePath{};
    std::string manifestFilePath{};
    std::string failureReason{};

    [[nodiscard]] std::string toJson() const;
};

// Runs the authoritative voice_probe_v1 scenario offline in 120 Hz simulation ticks (Inc-1)
VoiceProbeResult runVoiceProbeV1Offline(const std::string& outputDir = "artifacts/cacp",
                                        bool writeFiles = true);

// Profile metrics for timbre_bus_probe_v1 (Spec §16.8, §31.5, ADR-0003 D-M2-06, ADR-0004 AUD-D-01/02)
struct TimbreBusProbeResult {
    bool passed{false};
    std::uint64_t totalCapturedFrames{0};
    std::uint64_t droppedFrames{0};
    float peakAmplitude{0.0f};
    float truePeakDbtp{-120.0f};
    bool truePeakPassed{false};       // <= -1.0 dBTP verified by BS.1770-5 meter
    bool roleIsolationPassed{false};  // Per-role bus energy only inside the role's solo window (>= 60 dB isolation)
    bool duckingPassed{false};        // Corpo bus attenuated 1.5..3.0 dB during Pulso hit vs no-hit reference
    bool delayPassed{false};          // Trama echo isolated by difference arrives at 11/17 ms and decays
    bool dspAnomaliesPassed{false};   // Zero NaN/Inf/denormals (voices, buses and terminal limiter)
    std::uint32_t clippedSamples{0};
    // Measurements behind the checks (manifest "measurements" block)
    float roleIsolationWorstLeakDb{0.0f};       // Worst other-role / active-role energy ratio in solo windows
    float roleIsolationWeakestActiveDbfs{-240.0f};
    float duckingAttenuationDb{0.0f};           // Peak Corpo bus attenuation during the Pulso hit
    float duckingPreHitDeviationDb{0.0f};       // Corpo bus deviation before the hit (must be ~0)
    std::int64_t delayLagLFrames{-1};           // First echo sample after Trama onset (expected 528 + 1)
    std::int64_t delayLagRFrames{-1};           // (expected 816 + 1)
    float delayEchoRmsDbfs{-240.0f};            // Echo-path RMS while the Trama note sounds
    float delayLateEchoRmsDbfs{-240.0f};        // Echo-path RMS 5000..6000 frames after the note end
    std::string buildId{CYMATICA_BUILD_ID};
    std::uint64_t transportEpoch{1};
    std::uint64_t simulationTicks{360};
    std::uint64_t requestedSampleFrames{144000};
    std::string presentationQuality{"Estimated"};
    bool wavWriteSuccess{false};
    bool manifestWriteSuccess{false};
    std::string wavFilePath{};
    std::string manifestFilePath{};
    std::string failureReason{};

    [[nodiscard]] std::string toJson() const;
};

// Runs the authoritative timbre_bus_probe_v1 scenario offline in 120 Hz simulation ticks (Inc-2)
TimbreBusProbeResult runTimbreBusProbeV1Offline(const std::string& outputDir = "artifacts/cacp",
                                               bool writeFiles = true);

// Profile metrics for planner_probe_v1 (Spec §11.7, §16, ADR-0003 D-M2-02/04/05/06, Inc-3)
struct PlannerProbeResult {
    bool passed{false};
    std::uint64_t totalCapturedFrames{0};
    std::uint64_t droppedFrames{0};
    std::uint32_t generatedEvents{0};          // MusicIntentEvents (NoteOns) emitted by the planner
    std::uint32_t plannerNoteOffs{0};          // Explicit open-pad NoteOffs scheduled by the planner
    std::uint32_t expectedScheduledEvents{0};  // NoteOn + NoteOff SPSC events expected (2 x generatedEvents)
    std::uint32_t committedEvents{0};          // Events dispatched by the AudioScheduler into the engine SPSC
    std::uint64_t engineProcessedEvents{0};    // Scheduled events consumed by the audio callback
    std::uint64_t droppedScheduledEvents{0};
    std::uint64_t deferredScheduledEvents{0};
    std::uint64_t schedulerBackpressure{0};
    std::uint32_t orphanNoteRecoveredCount{0};
    bool schedulingAccountingPassed{false};    // every planned event committed and consumed, none dropped/deferred/orphaned
    bool noteOffPairingPassed{false};          // every NoteOn matched by exactly one NoteOff after its onset
    bool determinismPassed{false};      // Canonical equality of all intents, NoteOffs and decision traces of two independent runs
    bool streamIsolationPassed{false};  // Metamorphic seed isolation + StreamId domain separation of the tie-break key
    bool motifInvariancePassed{false};  // Winning transform invariants re-verified from the decision trace + realized Vettore notes
    bool triadBudgetPassed{false};      // Overlapping Corpo notes in the emitted intents never > 2 at any frame
    std::uint32_t maxCorpoOverlap{0};
    bool fallbackPassed{false};         // L1 / L2 / L3 fallback levels reached under constructed conditions
    bool tempoAckPassed{false};         // SetTempoMap ack Applied at frameAtBar(4) with transport epoch 2
    std::uint32_t tempoAckStatus{0};    // AudioCommandStatus of the SetTempoMap ack (0 = none received)
    std::uint64_t tempoAckFrame{0};
    std::uint64_t tempoAckEpoch{0};
    std::uint64_t tempoChangeFrame{0};  // Requested frame = clock.frameAtBar(4)
    std::uint32_t stopAckStatus{0};
    std::uint64_t stopAckFrame{0};
    std::uint64_t contentEndFrame{0};   // clock.frameAtBar(8): last planner NoteOff
    std::uint64_t stopFrame{0};         // Stop target frame (after content end + release margin)
    bool nonSilentAudioPassed{false};   // RMS energy in section A bar 1 and section B bar 1 > -40 dBFS
    float sectionARmsDbfs{-120.0f};
    float sectionBRmsDbfs{-120.0f};
    std::uint64_t winningCandidateIdA{0};
    std::uint64_t winningCandidateIdB{0};
    std::int32_t winningScoreA{0};
    std::int32_t winningScoreB{0};
    float peakAmplitude{0.0f};
    float truePeakDbtp{-120.0f};
    bool truePeakPassed{false};         // <= -1.0 dBTP verified by BS.1770-5 meter
    std::uint32_t clippedSamples{0};
    bool dspAnomaliesPassed{false};
    std::string buildId{CYMATICA_BUILD_ID};
    std::uint64_t transportEpoch{0};    // Final telemetry transport epoch (after Stop)
    std::uint64_t simulationTicks{0};
    std::uint64_t requestedSampleFrames{0};
    std::string presentationQuality{"Estimated"};
    bool wavWriteSuccess{false};
    bool manifestWriteSuccess{false};
    std::string wavFilePath{};
    std::string manifestFilePath{};
    std::string failureReason{};

    [[nodiscard]] std::string toJson() const;
};

// Regression golden of the fixture_probe_v1 decision trace (seed 1337, policy DEFAULT_POLICY_VERSION_ID,
// kPlannerScoringProfileV1). Produced by commit 9c61f88 and unchanged by the explicit PhrasePlan flags of
// the Inc-4 remediation; it may only change together with a reviewed policy or fixture version bump.
inline constexpr std::uint64_t kFixtureProbeV1GoldenTraceHash = 10949526468471789087ULL;
// Regression golden of the full canonical plan of fixture_probe_v1 (events + planner NoteOffs + decision traces,
// canonicalPlanHash format version 1, DIF-INC4-R3-03). Same value as the [fixture_golden] test, which uses the
// same shared serialization.
inline constexpr std::uint64_t kFixtureProbeV1GoldenPlanHash = 0x4f021cb9e8637598ULL;
// Minimum post-bus telemetry energy for a role to count as active in a section (D-M2-11)
inline constexpr float kFixtureRoleActivityFloor = 1.0e-4f;

// Fault injected into fixture_probe_v1 to exercise the terminal fail-closed safe stop (D-INC4-R3-01)
enum class FixtureInjectedFault : std::uint8_t {
    None            = 0,
    InvalidTempoMap = 1, // B SetTempoMap with an unsupported map: rejected on receipt, one bar before the boundary
    LateTempoMap    = 2  // B SetTempoMap sent after its boundary: rejected as late, epoch-2 events already committed
};

// Metrics of fixture_probe_v1 (ADR-0005 D-M2-08/09/11, Inc-4)
struct FixtureProbeResult {
    bool passed{false};
    std::uint64_t totalCapturedFrames{0};
    std::uint64_t droppedFrames{0};
    std::uint64_t contentEndFrame{0};
    std::uint64_t lookaheadFrames{0};
    std::uint32_t phrasesPlanned{0};
    std::uint32_t generatedEvents{0};
    std::uint32_t plannerNoteOffs{0};
    std::uint64_t engineProcessedEvents{0};
    std::uint64_t droppedScheduledEvents{0};
    std::uint64_t deferredScheduledEvents{0};
    std::uint64_t schedulerBackpressureDrops{0};
    std::size_t revokedStagedEvents{0};
    std::size_t stagedEventsAtStop{0};
    std::uint32_t orphanNoteRecoveredCount{0};
    std::uint32_t voiceDropCount{0};
    std::array<AudioCommandAck, 4> transportAcks{}; // Start, SetTempoMap B, SetTempoMap A', Stop
    std::array<std::array<float, 4>, 3> roleEnergyMax{}; // [section][Pulso, Corpo, Trama, Vettore]
    std::array<float, 3> sectionRmsDbfs{-120.0f, -120.0f, -120.0f};
    bool captureCompletePassed{false};
    bool tempoAckPassed{false};
    bool noteOffPairingPassed{false};
    bool schedulingAccountingPassed{false};
    bool nonSilentAudioPassed{false};
    bool roleActivityPassed{false};
    bool silenceBoundaryPassed{false};
    bool determinismPassed{false};
    bool goldenTracePassed{false};
    float peakAmplitude{0.0f};
    bool sampleCeilingPassed{false};
    float truePeakDbtp{-120.0f};
    bool truePeakPassed{false};
    float maxGainReductionDb{0.0f};
    std::uint32_t clippedSamples{0};
    bool dspAnomaliesPassed{false};
    std::uint64_t decisionTraceHash{0};
    std::uint64_t canonicalPlanHash{0};
    bool goldenPlanPassed{false};
    std::uint64_t expectedEngineEvents{0}; // 2 x generatedEvents: one NoteOn and one NoteOff per intent
    // O3 chain and plan identities (ADR-0006 D-M2-12, Inc-5 phase 5a)
    std::uint64_t jitPlanHash{0};             // canonicalPlanHash of the intents the Coordinator planned just in time
    std::string jitGoldenStatus{"NOT_RUN"};   // PASS / FAIL (nominal), NOT_APPLICABLE (injected fault: partial plan)
    std::uint64_t contentFingerprint{0};      // musicalContentFingerprint of the offline plan
    std::uint64_t contentFingerprintNotes{0};
    std::uint64_t contentFingerprintExplicitReleases{0};
    bool contentFingerprintJitMatch{false};   // the JIT plan has the same musical content (nominal run)
    std::uint64_t chainExpectedEvents{0};     // |E_exp| of the JIT plan
    std::uint64_t chainSentEvents{0};
    std::uint64_t chainCanceledEvents{0};
    std::uint64_t chainConsumedEvents{0};
    std::uint64_t chainAppliedEvents{0};
    std::uint64_t chainIgnoredEvents{0};
    std::uint64_t chainDroppedEvents{0};
    std::uint64_t chainFaultCutFrame{0};
    std::uint64_t chainMismatches{0};
    bool chainSpscDrained{false};
    std::string chainFirstMismatch{};
    bool eventChainPassed{false};             // identity reconciliation E_exp = E_sent (+) E_canceled, E_sent = E_cons
    // Terminal fail-closed safe stop (D-INC4-R3-01); populated in every run, exercised by injected faults
    FixtureInjectedFault injectedFault{FixtureInjectedFault::None};
    bool coordinatorFailed{false};
    std::string coordinatorFailureReason{};
    std::uint64_t failedCommandId{0};
    std::uint64_t failureDetectedAtFrame{0};  // first audio block boundary still executable after the ack
    std::uint64_t emergencyStopFrame{0};      // target frame of the emergency Stop
    std::uint32_t emergencyStopAttempts{0};   // >1 only under command-queue backpressure
    AudioCommandAck emergencyStopAck{};
    std::uint64_t dispatchedEvents{0};        // events the scheduler pushed to the engine SPSC
    std::uint64_t unexpectedDroppedEvents{0}; // drops before the failed transition took effect
    std::uint64_t invalidatedDroppedEvents{0};// drops caused by the failure and the emergency Stop
    std::uint32_t activeVoicesAfterStop{0};
    bool staleEventRejected{false};           // an old-epoch NoteOn after the Stop never sounds
    bool safeStopPassed{false};
    std::uint64_t pcmContentHash{0};
    std::string buildId{CYMATICA_BUILD_ID};
    std::uint64_t transportEpoch{0};
    std::uint64_t simulationTicks{0};
    std::uint64_t requestedSampleFrames{0};
    std::string presentationQuality{"Estimated"};
    bool wavWriteSuccess{false};
    bool manifestWriteSuccess{false};
    std::string wavFilePath{};
    std::string manifestFilePath{};
    std::string failureReason{};

    [[nodiscard]] std::string toJson() const;
};

// Benchmark profile of profile_probe_v1 (ADR-0005 D-M2-10): Release, offline, 512-frame blocks
inline constexpr std::uint32_t kProfileBlockFrames = 512;
inline constexpr std::uint32_t kProfileWarmupBlocks = 64;
inline constexpr std::uint32_t kProfileWorkloadWBlocks = 10000;
inline constexpr std::uint32_t kProfileRepetitions = 3;
// Workload W diagnostic tone amplitude (Start.paramF32): the tone is summed before the limiter and, with the voices,
// gives about 3.1 dB of sustained gain reduction (DIF-INC4-R2-01, docs/research/M2_4_DIF-INC4-R2-01_Limiter_GR_Analysis.md)
inline constexpr float kProfileWorkloadWToneVolume = 0.8f;

enum class ProfileWorkload : std::uint8_t {
    W = 0, // worst-case stress: 8 held voices, one steal per block, Trama delay, diagnostic tone, limiter in gain reduction
    F = 1  // full fixture-m2-4-v1 driven by the just-in-time Coordinator
};

// One measured repetition of a workload (warm-up blocks discarded)
struct ProfileWorkloadRun {
    ProfileWorkload workload{ProfileWorkload::W};
    bool cacpEnabled{false};
    std::uint32_t repetition{0};
    bool isRetry{false};
    double p50Ms{0.0};
    double p95Ms{0.0};
    double p99Ms{0.0};
    double p999Ms{0.0};
    double maxMs{0.0};
    double meanMs{0.0};
    std::uint64_t totalBlocks{0};
    std::uint64_t blocksExceeding2Ms{0};
    std::uint64_t blocksExceedingBlockPeriod{0};
    float maxGainReductionDb{0.0f};
    std::uint32_t voiceSteals{0};
    bool stressConditionsMet{true}; // W: 8 active voices, >= 1 steal per block, limiter GR > 0; F: fixture acks and scheduling correct
    bool p999Passed{false};
    bool blockPeriodPassed{false};
};

// Metrics of profile_probe_v1 (ADR-0005 D-M2-10/11, Inc-4): W and F, CACP OFF/ON, 3 repetitions each
struct ProfileProbeResult {
    bool passed{false};
    bool timingGated{false}; // Release only; Debug reports timings without gating
    std::vector<ProfileWorkloadRun> runs{};
    double worstP999Ms{0.0};
    double worstMaxMs{0.0};
    std::string buildId{CYMATICA_BUILD_ID};
    std::string osVersion{};
    std::string cpuInfo{};
    std::string compilerVersion{};
    std::string buildConfig{};
    bool manifestWriteSuccess{false};
    std::string manifestFilePath{};
    std::string failureReason{};

    [[nodiscard]] std::string toJson() const;
};

// Scheduling-accounting verdict of fixture_probe_v1 (DIF-INC4-R3-02): every planned intent reaches the engine as
// exactly one NoteOn and one NoteOff, none dropped or deferred, no backpressure, empty staging at Stop, no orphan
// recovery and every phrase planned. Pure function so that negative cases can be tested directly.
[[nodiscard]] bool fixtureSchedulingAccountingPassed(const FixtureProbeResult& r, std::size_t expectedPhrases) noexcept;

struct RoleStemSink;

// Optional offline capture for CACP analysis (ADR-0006 D-M2-13/16, Inc-5 phase 5c). Every pointer may be null.
// The stem sink is installed for the scenario render only (removed before the probe-only stale-event check).
struct ProbeAnalysisCapture {
    std::uint64_t fixtureSeed{1337};                       // fixture planner seed; the goldens apply to 1337 only
    RoleStemSink* stems{nullptr};                          // per-role stems + bus mix, frame-aligned with the PCM
    std::vector<float>* stereo{nullptr};                   // recorded interleaved stereo PCM
    std::vector<AppliedEventRecord>* applied{nullptr};     // records consumed by the engine during the scenario
};

FixtureProbeResult runFixtureProbeV1Offline(const std::string& outputDir = "artifacts/cacp",
                                           bool writeFiles = true,
                                           FixtureInjectedFault fault = FixtureInjectedFault::None,
                                           const ProbeAnalysisCapture* capture = nullptr);

ProfileWorkloadRun runProfileWorkload(ProfileWorkload workload, bool cacpEnabled);

ProfileProbeResult runProfileProbeV1Offline(const std::string& outputDir = "artifacts/cacp",
                                           bool writeFiles = true);

// Runs the authoritative planner_probe_v1 scenario offline in 120 Hz simulation ticks (Inc-3)
PlannerProbeResult runPlannerProbeV1Offline(const std::string& outputDir = "artifacts/cacp",
                                           bool writeFiles = true,
                                           const ProbeAnalysisCapture* capture = nullptr);

} // namespace cymatica::audio
