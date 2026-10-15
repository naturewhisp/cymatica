#include <catch2/catch_test_macros.hpp>

#include "allocation_tracker.h"
#include "audio_engine.h"
#include "voice_allocator.h"

#include <cmath>
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
}
