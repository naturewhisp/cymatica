#include <catch2/catch_test_macros.hpp>

#include "allocation_tracker.h"
#include "audio_engine.h"
#include "voice_allocator.h"

#include <cmath>
#include <limits>
#include <vector>

using namespace cymatica::audio;
using cymatica::test::AllocationScope;

TEST_CASE("VoiceAllocator provides static 8-voice pool, 2 per role", "[voice_allocator][static_pool]") {
    VoiceAllocator allocator;
    allocator.init(48000);

    REQUIRE(VoiceAllocator::kTotalSlots == 8);
    REQUIRE(VoiceAllocator::kSlotsPerRole == 2);
    REQUIRE(VoiceAllocator::kRoleCount == 4);
    REQUIRE(allocator.activeVoiceCount() == 0);

    for (std::size_t i = 0; i < VoiceAllocator::kTotalSlots; ++i) {
        const auto& slot = allocator.slot(i);
        REQUIRE(slot.slotIndex == static_cast<std::uint8_t>(i));
        REQUIRE(slot.state == VoiceState::Idle);
        REQUIRE(slot.envelopeLevel == 0.0f);
        REQUIRE_FALSE(slot.stealTail.active);
    }

    // Role slot bounds
    REQUIRE(VoiceAllocator::roleFirstSlot(MusicRole::Pulso) == 0);
    REQUIRE(VoiceAllocator::roleFirstSlot(MusicRole::Corpo) == 2);
    REQUIRE(VoiceAllocator::roleFirstSlot(MusicRole::Trama) == 4);
    REQUIRE(VoiceAllocator::roleFirstSlot(MusicRole::Vettore) == 6);

    REQUIRE(allocator.slot(0).role == MusicRole::Pulso);
    REQUIRE(allocator.slot(1).role == MusicRole::Pulso);
    REQUIRE(allocator.slot(2).role == MusicRole::Corpo);
    REQUIRE(allocator.slot(3).role == MusicRole::Corpo);
    REQUIRE(allocator.slot(4).role == MusicRole::Trama);
    REQUIRE(allocator.slot(5).role == MusicRole::Trama);
    REQUIRE(allocator.slot(6).role == MusicRole::Vettore);
    REQUIRE(allocator.slot(7).role == MusicRole::Vettore);
}

TEST_CASE("VoiceAllocator enforces deterministic slot selection rules (Rules 0, 1, 2)", "[voice_allocator][deterministic_selection]") {
    VoiceAllocator allocator;
    allocator.init(48000);

    // Rule 0: Free (Idle) slots. Slot 0 chosen before slot 1.
    ScheduledAudioEvent e1{
        .eventId = 1,
        .sampleFrame = 1000,
        .transportEpoch = 1,
        .type = AudioEventType::NoteOn,
        .role = MusicRole::Pulso,
        .valueA = 220.0f,
        .valueB = 0.8f
    };
    int slotIdx1 = allocator.onNoteOn(e1);
    REQUIRE(slotIdx1 == 0);
    REQUIRE(allocator.activeVoiceCount() == 1);

    ScheduledAudioEvent e2{
        .eventId = 2,
        .sampleFrame = 2000,
        .transportEpoch = 1,
        .type = AudioEventType::NoteOn,
        .role = MusicRole::Pulso,
        .valueA = 330.0f,
        .valueB = 0.8f
    };
    int slotIdx2 = allocator.onNoteOn(e2);
    REQUIRE(slotIdx2 == 1);
    REQUIRE(allocator.activeVoiceCount() == 2);

    // Rule 1: Slot in Release picked over active slot.
    // Transition slot 0 to Release:
    ScheduledAudioEvent off1{
        .eventId = 1,
        .sampleFrame = 3000,
        .transportEpoch = 1,
        .type = AudioEventType::NoteOff,
        .role = MusicRole::Pulso
    };
    int relIdx = allocator.onNoteOff(off1);
    REQUIRE(relIdx == 0);
    REQUIRE(allocator.slot(0).state == VoiceState::Release);

    ScheduledAudioEvent e3{
        .eventId = 3,
        .sampleFrame = 3500,
        .transportEpoch = 1,
        .type = AudioEventType::NoteOn,
        .role = MusicRole::Pulso,
        .valueA = 440.0f,
        .valueB = 0.8f
    };
    int slotIdx3 = allocator.onNoteOn(e3);
    REQUIRE(slotIdx3 == 0); // Slot 0 reused because it was in Release!
    REQUIRE(allocator.voiceDropCount() == 0); // Not a steal!

    // Rule 2: Voice Stealing when both slots are active.
    // Slot 0 has startFrame = 3500. Slot 1 has startFrame = 2000.
    // Both in active state. Older voice (slot 1, startFrame 2000 < 3500) must be stolen!
    ScheduledAudioEvent e4{
        .eventId = 4,
        .sampleFrame = 5000,
        .transportEpoch = 1,
        .type = AudioEventType::NoteOn,
        .role = MusicRole::Pulso,
        .valueA = 550.0f,
        .valueB = 0.8f
    };
    int slotIdx4 = allocator.onNoteOn(e4);
    REQUIRE(slotIdx4 == 1); // Older slot 1 stolen!
    REQUIRE(allocator.voiceDropCount() == 1); // Rule 2 increments voiceDropCount
    REQUIRE(allocator.slot(1).stealTail.active); // Steal tail active
}

TEST_CASE("VoiceAllocator handles rapid successive voice steals with composite snapshot decay", "[voice_allocator][rapid_steals]") {
    VoiceAllocator allocator;
    allocator.init(48000);

    // Fill both slots of Vettore (slots 6 and 7)
    ScheduledAudioEvent v1{
        .eventId = 10,
        .sampleFrame = 1000,
        .type = AudioEventType::NoteOn,
        .role = MusicRole::Vettore,
        .valueA = 440.0f,
        .valueB = 0.8f
    };
    ScheduledAudioEvent v2{
        .eventId = 11,
        .sampleFrame = 2000,
        .type = AudioEventType::NoteOn,
        .role = MusicRole::Vettore,
        .valueA = 550.0f,
        .valueB = 0.8f
    };
    REQUIRE(allocator.onNoteOn(v1) == 6);
    REQUIRE(allocator.onNoteOn(v2) == 7);

    // Advance 100 samples to reach active state
    float out[200 * 2]{};
    allocator.renderBlock(out, 100, 2000, false);

    // First steal: Note 12 steals older slot 6
    ScheduledAudioEvent v3{
        .eventId = 12,
        .sampleFrame = 2100,
        .type = AudioEventType::NoteOn,
        .role = MusicRole::Vettore,
        .valueA = 660.0f,
        .valueB = 0.9f
    };
    REQUIRE(allocator.onNoteOn(v3) == 6);
    REQUIRE(allocator.voiceDropCount() == 1);
    REQUIRE(allocator.slot(6).stealTail.active);
    REQUIRE(allocator.slot(6).stealTail.remainingSamples == StealTail::kTailLength);

    // Render 30 frames (< 96 frame tail length): tail is still active!
    allocator.renderBlock(out, 30, 2100, false);
    REQUIRE(allocator.slot(6).stealTail.remainingSamples == 66);

    // Fast consecutive steal: Note 13 arrives at 2130, slot 7 has startFrame 2000 < 2100 so slot 7 is stolen!
    ScheduledAudioEvent v4{
        .eventId = 13,
        .sampleFrame = 2130,
        .type = AudioEventType::NoteOn,
        .role = MusicRole::Vettore,
        .valueA = 770.0f,
        .valueB = 0.9f
    };
    REQUIRE(allocator.onNoteOn(v4) == 7);
    REQUIRE(allocator.voiceDropCount() == 2);

    // Now steal slot 6 again rapidly (< 96 samples):
    ScheduledAudioEvent v5{
        .eventId = 14,
        .sampleFrame = 2140,
        .type = AudioEventType::NoteOn,
        .role = MusicRole::Vettore,
        .valueA = 880.0f,
        .valueB = 0.9f
    };
    // Slot 6 has startFrame 2100, slot 7 has startFrame 2130 -> Slot 6 stolen again while tail was active!
    REQUIRE(allocator.onNoteOn(v5) == 6);
    REQUIRE(allocator.voiceDropCount() == 3);
    REQUIRE(allocator.slot(6).stealTail.active);
    REQUIRE(allocator.slot(6).stealTail.remainingSamples == StealTail::kTailLength); // Reset to 96
    REQUIRE(allocator.dspAnomalyCount() == 0); // No NaN/Inf
}

TEST_CASE("VoiceAllocator correlates NoteOff strictly by (role, eventId)", "[voice_allocator][note_identity_matching]") {
    VoiceAllocator allocator;
    allocator.init(48000);

    // Start 2 distinct notes on Corpo
    ScheduledAudioEvent c1{
        .eventId = 1001,
        .sampleFrame = 1000,
        .type = AudioEventType::NoteOn,
        .role = MusicRole::Corpo,
        .valueA = 220.0f,
        .valueB = 0.7f
    };
    ScheduledAudioEvent c2{
        .eventId = 1002,
        .sampleFrame = 2000,
        .type = AudioEventType::NoteOn,
        .role = MusicRole::Corpo,
        .valueA = 277.18f, // C#
        .valueB = 0.7f
    };
    REQUIRE(allocator.onNoteOn(c1) == 2);
    REQUIRE(allocator.onNoteOn(c2) == 3);

    // Send NoteOff targeting event 1002:
    ScheduledAudioEvent off2{
        .eventId = 1002,
        .sampleFrame = 3000,
        .type = AudioEventType::NoteOff,
        .role = MusicRole::Corpo
    };
    REQUIRE(allocator.onNoteOff(off2) == 3);
    REQUIRE(allocator.slot(3).state == VoiceState::Release);
    // Note 1001 on slot 2 must remain active!
    REQUIRE(allocator.slot(2).state == VoiceState::Attack);
    REQUIRE(allocator.slot(2).eventId == 1001);

    // Send unmatched / stale NoteOff with eventId 9999:
    ScheduledAudioEvent staleOff{
        .eventId = 9999,
        .sampleFrame = 4000,
        .type = AudioEventType::NoteOff,
        .role = MusicRole::Corpo
    };
    REQUIRE(allocator.onNoteOff(staleOff) == -1); // Safely rejected without muting slot 2!
    REQUIRE(allocator.slot(2).state != VoiceState::Release);
    REQUIRE(allocator.slot(2).state != VoiceState::Idle);

    // Send NoteOff with eventId 0: must NOT act as a wildcard! (strict (role, eventId) matching)
    ScheduledAudioEvent zeroOff{
        .eventId = 0,
        .sampleFrame = 4500,
        .type = AudioEventType::NoteOff,
        .role = MusicRole::Corpo
    };
    REQUIRE(allocator.onNoteOff(zeroOff) == -1); // Wildcard rejected!
    REQUIRE(allocator.slot(2).state != VoiceState::Release);
    REQUIRE(allocator.slot(2).state != VoiceState::Idle);
}

TEST_CASE("VoiceAllocator enforces open pad safety lease backstop", "[voice_allocator][open_pad_lease_backstop]") {
    VoiceAllocator allocator;
    allocator.init(48000);

    // Corpo pad with durationFrames == 0 (open pad)
    ScheduledAudioEvent openPad{
        .eventId = 500,
        .sampleFrame = 1000,
        .type = AudioEventType::NoteOn,
        .role = MusicRole::Corpo,
        .valueA = 110.0f,
        .valueB = 0.8f,
        .durationFrames = 0 // open
    };
    REQUIRE(allocator.onNoteOn(openPad) == 2);

    // Advance 50,000 frames: pad remains active
    allocator.checkLifetimes(51000);
    REQUIRE(allocator.slot(2).state != VoiceState::Release);
    REQUIRE(allocator.orphanNoteRecoveredCount() == 0);

    // Advance to 1000 + kMaxLeaseDurationFrames - 1: still active
    allocator.checkLifetimes(1000 + VoiceAllocator::kMaxLeaseDurationFrames - 1);
    REQUIRE(allocator.slot(2).state != VoiceState::Release);
    REQUIRE(allocator.orphanNoteRecoveredCount() == 0);

    // Advance to exact lease backstop expiration:
    allocator.checkLifetimes(1000 + VoiceAllocator::kMaxLeaseDurationFrames);
    REQUIRE(allocator.slot(2).state == VoiceState::Release);
    REQUIRE(allocator.orphanNoteRecoveredCount() == 1);
}

TEST_CASE("VoiceAllocator enforces finite duration failsafe timeout", "[voice_allocator][finite_duration_failsafe]") {
    VoiceAllocator allocator;
    allocator.init(48000);

    // Note with explicit duration of 4800 frames (100 ms)
    ScheduledAudioEvent shortNote{
        .eventId = 700,
        .sampleFrame = 10000,
        .type = AudioEventType::NoteOn,
        .role = MusicRole::Trama,
        .valueA = 440.0f,
        .valueB = 0.6f,
        .durationFrames = 4800
    };
    REQUIRE(allocator.onNoteOn(shortNote) == 4);

    // At frame 14799 (< 14800): still active
    allocator.checkLifetimes(14799);
    REQUIRE(allocator.slot(4).state != VoiceState::Release);

    // At frame 14800 (startFrame + durationFrames): failsafe triggers release!
    allocator.checkLifetimes(14800);
    REQUIRE(allocator.slot(4).state == VoiceState::Release);
}

TEST_CASE("VoiceAllocator provides physical 2ms transport de-click and frozen timeline on Pause/Resume/Stop", "[voice_allocator][pause_physical_declick]") {
    VoiceAllocator allocator;
    allocator.init(48000);

    ScheduledAudioEvent note{
        .eventId = 1,
        .sampleFrame = 0,
        .type = AudioEventType::NoteOn,
        .role = MusicRole::Pulso,
        .valueA = 440.0f,
        .valueB = 0.8f
    };
    REQUIRE(allocator.onNoteOn(note) == 0);

    // Render 500 frames while running: active signal
    float buf[1000 * 2]{};
    allocator.renderBlock(buf, 500, 0, false);
    float maxBefore = 0.0f;
    for (int i = 0; i < 500 * 2; ++i) maxBefore = (std::max)(maxBefore, std::abs(buf[i]));
    REQUIRE(maxBefore > 0.1f);

    // Trigger Pause: 2 ms fade-out
    allocator.onPause();
    // Render 96 frames of de-click tail while paused:
    allocator.renderBlock(buf, 96, 500, true);
    // After 96 frames, output must be strictly silent (0.0f)!
    float silentBuf[500 * 2]{};
    allocator.renderBlock(silentBuf, 500, 500, true);
    float maxSilent = 0.0f;
    for (int i = 0; i < 500 * 2; ++i) maxSilent = (std::max)(maxSilent, std::abs(silentBuf[i]));
    REQUIRE(maxSilent == 0.0f);

    // Resume: 2 ms fade-in
    allocator.onResume();
    allocator.renderBlock(buf, 200, 500, false);
    float maxResumed = 0.0f;
    for (int i = 0; i < 200 * 2; ++i) maxResumed = (std::max)(maxResumed, std::abs(buf[i]));
    REQUIRE(maxResumed > 0.1f);

    // Stop: destructive reset to Idle with 2 ms tail
    allocator.onStop();
    REQUIRE(allocator.activeVoiceCount() == 0);
    for (std::size_t i = 0; i < VoiceAllocator::kTotalSlots; ++i) {
        REQUIRE(allocator.slot(i).state == VoiceState::Idle);
    }

    // Render 96 frames of physical de-click tail while stopped
    allocator.renderBlock(buf, 96, 500, true);
    // After 96 frames, output must be strictly silent (0.0f)
    allocator.renderBlock(silentBuf, 500, 500, true);
    maxSilent = 0.0f;
    for (int i = 0; i < 500 * 2; ++i) maxSilent = (std::max)(maxSilent, std::abs(silentBuf[i]));
    REQUIRE(maxSilent == 0.0f);
}

TEST_CASE("VoiceAllocator preserves active voices across epoch changes", "[voice_allocator][cross_epoch_preservation]") {
    VoiceAllocator allocator;
    allocator.init(48000);

    // Note started in epoch 1
    ScheduledAudioEvent padEpoch1{
        .eventId = 88,
        .sampleFrame = 1000,
        .transportEpoch = 1,
        .type = AudioEventType::NoteOn,
        .role = MusicRole::Corpo,
        .valueA = 220.0f,
        .valueB = 0.8f,
        .durationFrames = 0
    };
    int slot = allocator.onNoteOn(padEpoch1);
    REQUIRE(slot == 2);

    // Render block crossing epoch boundary into epoch 2:
    float buf[512 * 2]{};
    allocator.renderBlock(buf, 512, 1000, false);
    REQUIRE(allocator.slot(2).state != VoiceState::Idle);
    REQUIRE(allocator.slot(2).state != VoiceState::Release);

    // Cross-epoch NoteOff from epoch 1 arrives in epoch 2:
    ScheduledAudioEvent offEpoch1{
        .eventId = 88,
        .sampleFrame = 2000,
        .transportEpoch = 1,
        .type = AudioEventType::NoteOff,
        .role = MusicRole::Corpo
    };
    REQUIRE(allocator.onNoteOff(offEpoch1) == 2);
    REQUIRE(allocator.slot(2).state == VoiceState::Release);
}

TEST_CASE("VoiceAllocator guarantees zero dynamic heap allocation in realtime synthesis", "[voice_allocator][realtime_zero_alloc]") {
    VoiceAllocator allocator;
    allocator.init(48000);

    std::vector<float> out(512 * 2, 0.0f);

    {
        AllocationScope scope;

        // Sequence of NoteOns, steals, NoteOffs, pauses, and block renderings
        for (std::uint64_t i = 0; i < 64; ++i) {
            ScheduledAudioEvent onEvt{
                .eventId = i + 1,
                .sampleFrame = i * 100,
                .type = AudioEventType::NoteOn,
                .role = static_cast<MusicRole>(i % 4),
                .valueA = 220.0f + static_cast<float>(i * 10),
                .valueB = 0.8f
            };
            (void)allocator.onNoteOn(onEvt);

            allocator.renderBlock(out.data(), 512, i * 100, false);

            if (i % 8 == 7) {
                ScheduledAudioEvent offEvt{
                    .eventId = i + 1,
                    .sampleFrame = i * 100 + 50,
                    .type = AudioEventType::NoteOff,
                    .role = static_cast<MusicRole>(i % 4)
                };
                (void)allocator.onNoteOff(offEvt);
            }
        }

        allocator.onPause();
        allocator.renderBlock(out.data(), 96, 6400, true);
        allocator.onResume();
        allocator.renderBlock(out.data(), 512, 6496, false);
        allocator.onStop();

        REQUIRE(scope.count() == 0);
    }

    // Inc-2 synthesis paths (timbreParam > 0): Pulso sweep + click band-pass, Corpo PolyBLEP/SVF, Trama FM with
    // the stereo delay and Corpo ducking active, Vettore glide/vibrato, steal tails into the buses, frozen pause
    allocator.reset();
    {
        AllocationScope scope;
        for (std::uint64_t i = 0; i < 64; ++i) {
            ScheduledAudioEvent onEvt{
                .eventId = 1000 + i,
                .sampleFrame = 7000 + i * 100,
                .type = AudioEventType::NoteOn,
                .role = static_cast<MusicRole>(i % 4),
                .valueA = 110.0f + static_cast<float>(i * 7),
                .valueB = 0.9f,
                .durationFrames = (i % 3 == 0) ? 0u : 900u,
                .timbreParam = 0.1f + 0.014f * static_cast<float>(i)
            };
            (void)allocator.onNoteOn(onEvt);
            allocator.renderBlock(out.data(), 512, 7000 + i * 100, false);
        }
        REQUIRE(allocator.voiceDropCount() > 0);
        allocator.onPause();
        allocator.renderBlock(out.data(), 512, 13400, true);
        allocator.renderBlock(out.data(), 512, 13400, true); // frozen early-out
        allocator.onResume();
        allocator.renderBlock(out.data(), 512, 13400, false);
        allocator.onStop();
        allocator.renderBlock(out.data(), 512, 13912, true);
        allocator.onStart();

        REQUIRE(scope.count() == 0);
    }
    REQUIRE(allocator.dspAnomalyCount() == 0);
}

TEST_CASE("VoiceAllocator Stop -> Start + NoteOn restores full output and linear stop tail (AUD-01, AUD-02)", "[voice_allocator][stop_start_recovery]") {
    VoiceAllocator allocator;
    allocator.init(48000);

    ScheduledAudioEvent note1{
        .eventId = 1,
        .sampleFrame = 0,
        .transportEpoch = 1,
        .type = AudioEventType::NoteOn,
        .role = MusicRole::Pulso,
        .valueA = 440.0f,
        .valueB = 1.0f
    };
    REQUIRE(allocator.onNoteOn(note1) == 0);

    // Advance into sustain and stop at a non-zero phase
    float buf[256 * 2]{};
    allocator.renderBlock(buf, 113, 0, false);
    const float preStopOut = allocator.slot(0).currentVoiceOutput();
    REQUIRE(std::abs(preStopOut) > 0.01f);

    allocator.onStop();
    REQUIRE(allocator.slot(0).stealTail.active);
    const float tailStart = allocator.slot(0).stealTail.startAmplitude[0];
    REQUIRE(std::abs(tailStart - preStopOut) < 1e-6f);

    // Render 96 frames of stop tail: verify strictly linear decay (not quadratic!)
    allocator.renderBlock(buf, 96, 113, true);
    // At mid-point f = 48 (remainingSamples = 96 - 48 = 48), linear factor is 48/96 = 0.5f (quadratic would be 0.25f)
    const float midExpected = tailStart * (48.0f / 96.0f);
    REQUIRE(std::abs(buf[48 * 2] - midExpected) < 1e-5f);

    // Render 100 more frames after tail completion: strictly 0.0f
    allocator.renderBlock(buf, 100, 113, true);
    for (int i = 0; i < 100 * 2; ++i) {
        REQUIRE(buf[i] == 0.0f);
    }

    // Start new run and trigger NoteOn: must produce full-amplitude signal (not muted at 0.0f!)
    allocator.onStart();
    ScheduledAudioEvent note2{
        .eventId = 2,
        .sampleFrame = 1000,
        .transportEpoch = 2,
        .type = AudioEventType::NoteOn,
        .role = MusicRole::Corpo,
        .valueA = 220.0f,
        .valueB = 0.8f
    };
    REQUIRE(allocator.onNoteOn(note2) == 2);
    allocator.renderBlock(buf, 200, 1000, false);
    float maxNewRun = 0.0f;
    for (int i = 0; i < 200 * 2; ++i) {
        maxNewRun = (std::max)(maxNewRun, std::abs(buf[i]));
    }
    REQUIRE(maxNewRun > 0.10f);

    // Also verify Pause -> wait until silent -> Stop does NOT re-emit a phantom tail (AUD-02)
    allocator.onPause();
    allocator.renderBlock(buf, 120, 1200, true); // Fully faded out to 0.0f
    allocator.onStop();
    REQUIRE_FALSE(allocator.slot(2).stealTail.active);
    allocator.renderBlock(buf, 96, 1200, true);
    for (int i = 0; i < 96 * 2; ++i) {
        REQUIRE(buf[i] == 0.0f);
    }
}

TEST_CASE("VoiceAllocator sanitizes out-of-range roles, NaN/Inf inputs, and non-finite envelopes (AUD-03, AUD-04)", "[voice_allocator][sanitization]") {
    VoiceAllocator allocator;
    allocator.init(48000);

    const float nanVal = std::numeric_limits<float>::quiet_NaN();
    const float infVal = std::numeric_limits<float>::infinity();

    // 1. Out-of-range MusicRole must be rejected safely (-1) without out-of-bounds array access
    ScheduledAudioEvent badRoleOn{
        .eventId = 1,
        .sampleFrame = 0,
        .type = AudioEventType::NoteOn,
        .role = static_cast<MusicRole>(99),
        .valueA = 440.0f,
        .valueB = 0.8f
    };
    REQUIRE(allocator.onNoteOn(badRoleOn) == -1);
    badRoleOn.type = AudioEventType::NoteOff;
    REQUIRE(allocator.onNoteOff(badRoleOn) == -1);

    // 2. Non-finite pitch (+Inf), velocity (NaN), timbreParam (-Inf) sanitized and counted in dspAnomalyCount
    ScheduledAudioEvent badParams{
        .eventId = 2,
        .sampleFrame = 100,
        .type = AudioEventType::NoteOn,
        .role = MusicRole::Pulso,
        .valueA = infVal,
        .valueB = nanVal,
        .timbreParam = -infVal
    };
    REQUIRE(allocator.onNoteOn(badParams) == 0);
    REQUIRE(allocator.dspAnomalyCount() >= 3);
    REQUIRE(std::isfinite(allocator.slot(0).pitchHz));
    REQUIRE(std::isfinite(allocator.slot(0).velocity));
    REQUIRE(std::isfinite(allocator.slot(0).timbreParam));

    float buf[128 * 2]{};
    allocator.renderBlock(buf, 128, 100, false);
    for (float s : buf) {
        REQUIRE(std::isfinite(s));
    }

    // 3. Non-finite envelopeLevel in Release comparator sanitized to 0.0f and counted (ADR-0003 D-M2-04 §5)
    ScheduledAudioEvent secondPulso{
        .eventId = 3,
        .sampleFrame = 300,
        .type = AudioEventType::NoteOn,
        .role = MusicRole::Pulso,
        .valueA = 330.0f,
        .valueB = 0.8f
    };
    REQUIRE(allocator.onNoteOn(secondPulso) == 1);
    allocator.renderBlock(buf, 64, 300, false);

    ScheduledAudioEvent off0{.eventId = 2, .sampleFrame = 400, .type = AudioEventType::NoteOff, .role = MusicRole::Pulso};
    ScheduledAudioEvent off1{.eventId = 3, .sampleFrame = 400, .type = AudioEventType::NoteOff, .role = MusicRole::Pulso};
    REQUIRE(allocator.onNoteOff(off0) == 0);
    REQUIRE(allocator.onNoteOff(off1) == 1);

    // Inject NaN into slot 1's envelopeLevel while both are in Release
    allocator.slot(0).envelopeLevel = 0.5f;
    allocator.slot(1).envelopeLevel = nanVal;
    const std::uint32_t anomaliesBefore = allocator.dspAnomalyCount();

    ScheduledAudioEvent thirdPulso{
        .eventId = 4,
        .sampleFrame = 450,
        .type = AudioEventType::NoteOn,
        .role = MusicRole::Pulso,
        .valueA = 440.0f,
        .valueB = 0.8f
    };
    // Slot 1 had NaN -> sanitized to 0.0f < 0.5f -> Slot 1 must be selected and dspAnomalyCount incremented!
    REQUIRE(allocator.onNoteOn(thirdPulso) == 1);
    REQUIRE(allocator.dspAnomalyCount() == anomaliesBefore + 1);
}

TEST_CASE("VoiceAllocator adapts 2ms de-click frames to sample rate and saturates uint64_t lifetimes (AUD-05, AUD-06)", "[voice_allocator][sample_rate_saturation]") {
    VoiceAllocator allocator44;
    allocator44.init(44100);
    // floor(0.002 * 44100) = 88 frames
    REQUIRE(allocator44.declickFrames() == 88);
    REQUIRE(allocator44.slot(0).releaseSamples == 88);

    VoiceAllocator allocator48;
    allocator48.init(48000);
    REQUIRE(allocator48.declickFrames() == 96);
    REQUIRE(allocator48.slot(0).releaseSamples == 96);

    // Test uint64_t saturation in checkLifetimes near UINT64_MAX
    ScheduledAudioEvent nearMaxNote{
        .eventId = 77,
        .sampleFrame = std::numeric_limits<std::uint64_t>::max() - 10,
        .type = AudioEventType::NoteOn,
        .role = MusicRole::Trama,
        .valueA = 440.0f,
        .valueB = 0.8f,
        .durationFrames = 1000 // Raw addition would overflow to 989!
    };
    REQUIRE(allocator48.onNoteOn(nearMaxNote) == 4);
    // At frame 2000 (which would be >= 989 if wrapped!), saturatingAdd keeps expireFrame at UINT64_MAX
    allocator48.checkLifetimes(2000);
    REQUIRE(allocator48.slot(4).state != VoiceState::Release);
}

TEST_CASE("VoiceAllocator guarantees independent Left and Right continuity at Stop and voice stealing for stereo voices", "[voice_allocator][stereo_stop_continuity]") {
    VoiceAllocator allocator;
    allocator.init(48000);

    // 1. Start an asymmetric stereo voice on Trama (slot 4: Left = scaled, Right = scaled * 0.5f)
    ScheduledAudioEvent tramaNote1{
        .eventId = 101,
        .sampleFrame = 0,
        .transportEpoch = 1,
        .type = AudioEventType::NoteOn,
        .role = MusicRole::Trama,
        .valueA = 440.0f,
        .valueB = 0.8f,
        .timbreParam = 0.5f
    };
    REQUIRE(allocator.onNoteOn(tramaNote1) == 4);

    // Render 1500 frames so the voice reaches sustain
    float buf[256 * 2]{};
    for (int b = 0; b < 6; ++b) {
        allocator.renderBlock(buf, 250, b * 250, false);
    }

    const float preStopL = allocator.slot(4).currentVoiceOutputL();
    const float preStopR = allocator.slot(4).currentVoiceOutputR();

    // Verify both channels are active and strictly asymmetric (L != R)
    REQUIRE(std::abs(preStopL) > 0.005f);
    REQUIRE(std::abs(preStopR) > 0.002f);
    REQUIRE(std::abs(preStopL - preStopR) > 0.002f);

    // Trigger onStop()
    allocator.onStop();
    REQUIRE(allocator.slot(4).stealTail.active);

    // Verify tail amplitudes are matched independently to L and R without cross-bleeding
    REQUIRE(std::abs(allocator.slot(4).stealTail.startAmplitude[0] - preStopL) < 1e-6f);
    REQUIRE(std::abs(allocator.slot(4).stealTail.startAmplitude[1] - preStopR) < 1e-6f);

    // 2. Test voice stealing on stereo voices preserves independent L and R in stealTail
    allocator.onStart();
    ScheduledAudioEvent tramaNote2{
        .eventId = 201,
        .sampleFrame = 1000,
        .transportEpoch = 2,
        .type = AudioEventType::NoteOn,
        .role = MusicRole::Trama,
        .valueA = 330.0f,
        .valueB = 0.8f,
        .timbreParam = 0.5f
    };
    REQUIRE(allocator.onNoteOn(tramaNote2) == 4);
    for (int b = 0; b < 6; ++b) {
        allocator.renderBlock(buf, 250, 1000 + b * 250, false);
    }

    ScheduledAudioEvent tramaNote3{
        .eventId = 202,
        .sampleFrame = 2500,
        .transportEpoch = 2,
        .type = AudioEventType::NoteOn,
        .role = MusicRole::Trama,
        .valueA = 550.0f,
        .valueB = 0.8f,
        .timbreParam = 0.5f
    };
    REQUIRE(allocator.onNoteOn(tramaNote3) == 5);
    for (int b = 0; b < 6; ++b) {
        allocator.renderBlock(buf, 250, 2500 + b * 250, false);
    }

    const float preStealL = allocator.slot(4).currentVoiceOutputL();
    const float preStealR = allocator.slot(4).currentVoiceOutputR();
    REQUIRE(std::abs(preStealL) > 0.005f);
    REQUIRE(std::abs(preStealR) > 0.002f);
    REQUIRE(std::abs(preStealL - preStealR) > 0.002f);

    // Steal slot 4 with a third note on Trama
    ScheduledAudioEvent tramaNote4{
        .eventId = 203,
        .sampleFrame = 4000,
        .transportEpoch = 2,
        .type = AudioEventType::NoteOn,
        .role = MusicRole::Trama,
        .valueA = 660.0f,
        .valueB = 0.8f,
        .timbreParam = 0.5f
    };
    REQUIRE(allocator.onNoteOn(tramaNote4) == 4);
    REQUIRE(allocator.slot(4).stealTail.active);
    REQUIRE(std::abs(allocator.slot(4).stealTail.startAmplitude[0] - preStealL) < 1e-6f);
    REQUIRE(std::abs(allocator.slot(4).stealTail.startAmplitude[1] - preStealR) < 1e-6f);
}

