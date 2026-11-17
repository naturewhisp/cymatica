#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "fade_event_log.h"
#include "voice_allocator.h"

#include <vector>

using namespace cymatica::audio;

namespace {

ScheduledAudioEvent on(std::uint64_t id, std::uint64_t frame, MusicRole role, float pitch, float velocity = 0.8f) {
    return ScheduledAudioEvent{.eventId = id, .sampleFrame = frame, .transportEpoch = 1, .type = AudioEventType::NoteOn,
                               .role = role, .valueA = pitch, .valueB = velocity, .timbreParam = 0.5f};
}
ScheduledAudioEvent off(std::uint64_t id, std::uint64_t frame, MusicRole role) {
    return ScheduledAudioEvent{.eventId = id, .sampleFrame = frame, .transportEpoch = 1, .type = AudioEventType::NoteOff, .role = role};
}
void render(VoiceAllocator& a, std::uint64_t from, std::uint64_t frames) {
    float buf[256 * 2]{};
    for (std::uint64_t f = 0; f < frames; f += 256) a.renderBlock(buf, 256, from + f, false);
}

} // namespace

TEST_CASE("package 2b E1: a Corpo note at sustain released by its NoteOff logs one 2 ms release with the exact envelope amplitude",
          "[fade_event_log]") {
    VoiceAllocator a;
    a.init(48000);
    FadeEventLog log(16);
    a.setFadeEventLog(&log);
    REQUIRE(a.onNoteOn(on(1, 0, MusicRole::Corpo, 220.0f)) >= 0);
    render(a, 0, 48000);   // 1 s: well past the 15 ms attack and the 100 ms decay
    REQUIRE(a.onNoteOff(off(1, 48000, MusicRole::Corpo)) >= 0);
    REQUIRE(log.records().size() == 1);
    const auto& r = log.records()[0];
    REQUIRE(r.cause == FadeCause::Release);
    REQUIRE(r.fadeFrames == 96);
    REQUIRE(r.frame == 48000);
    REQUIRE(r.role == static_cast<std::uint8_t>(MusicRole::Corpo));
    REQUIRE(r.envAmplitude == 0.75f * 0.8f * VoiceAllocator::kNominalVoiceScale);
    REQUIRE_THAT(r.busGain, Catch::Matchers::WithinRel(RoleBusProcessor::kCorpoGain, 1e-4f));
}

TEST_CASE("package 2b E1: with the offline release override the logged release lasts 30 ms", "[fade_event_log]") {
    VoiceAllocator a;
    a.init(48000);
    FadeEventLog log(16);
    a.setFadeEventLog(&log);
    // The override is written only by AudioEngine in production code; here the release length follows the NoteOn
    // through noteReleaseFrames(), so this case checks that the log reports the slot's own release length.
    REQUIRE(a.onNoteOn(on(1, 0, MusicRole::Corpo, 220.0f)) >= 0);
    const std::size_t slot = VoiceAllocator::roleFirstSlot(MusicRole::Corpo);
    a.slot(slot).releaseSamples = 1440;
    render(a, 0, 48000);
    REQUIRE(a.onNoteOff(off(1, 48000, MusicRole::Corpo)) >= 0);
    REQUIRE(log.records().size() == 1);
    REQUIRE(log.records()[0].fadeFrames == 1440);
}

TEST_CASE("package 2b E2: a steal and a release cut are logged as cuts with the envelope of the moment", "[fade_event_log]") {
    VoiceAllocator a;
    a.init(48000);
    FadeEventLog log(16);
    a.setFadeEventLog(&log);
    REQUIRE(a.onNoteOn(on(1, 0, MusicRole::Corpo, 220.0f)) >= 0);
    REQUIRE(a.onNoteOn(on(2, 0, MusicRole::Corpo, 330.0f)) >= 0);
    render(a, 0, 24000);
    REQUIRE(a.onNoteOn(on(3, 24000, MusicRole::Corpo, 440.0f)) >= 0);   // both slots active: steal
    REQUIRE(log.records().size() == 1);
    REQUIRE(log.records()[0].cause == FadeCause::Steal);
    REQUIRE(log.records()[0].fadeFrames == 0);
    REQUIRE(log.records()[0].envAmplitude == 0.75f * 0.8f * VoiceAllocator::kNominalVoiceScale);
    render(a, 24000, 24000);
    // Release both voices, then a new NoteOn while they are still releasing: a release cut
    (void)a.onNoteOff(off(2, 48000, MusicRole::Corpo));
    (void)a.onNoteOff(off(3, 48000, MusicRole::Corpo));
    render(a, 48000, 0);
    const std::size_t before = log.records().size();
    REQUIRE(a.onNoteOn(on(4, 48010, MusicRole::Corpo, 220.0f)) >= 0);
    REQUIRE(log.records().size() == before + 1);
    const auto& cut = log.records().back();
    REQUIRE(cut.cause == FadeCause::ReleaseCut);
    REQUIRE(cut.fadeFrames == 0);
    REQUIRE(cut.envAmplitude > 0.0f);
    REQUIRE(a.releaseCutCount(MusicRole::Corpo) == 1);
    REQUIRE(a.voiceDropCount(MusicRole::Corpo) == 1);
}

TEST_CASE("package 2b E3: a Pulso note already decayed to zero logs nothing at its NoteOff", "[fade_event_log]") {
    VoiceAllocator a;
    a.init(48000);
    FadeEventLog log(16);
    a.setFadeEventLog(&log);
    REQUIRE(a.onNoteOn(on(1, 0, MusicRole::Pulso, 55.0f)) >= 0);
    render(a, 0, 24000);   // 500 ms: the 1 ms attack and <= 120 ms decay with sustain 0 are over
    REQUIRE(a.onNoteOff(off(1, 24000, MusicRole::Pulso)) < 0);
    REQUIRE(log.records().empty());
}

TEST_CASE("package 2b: a full log counts overflow instead of growing", "[fade_event_log]") {
    FadeEventLog log(1);
    log.push(FadeEventRecord{});
    log.push(FadeEventRecord{});
    REQUIRE(log.records().size() == 1);
    REQUIRE(log.overflow() == 1);
}

TEST_CASE("F4b: a Pulso note is one-shot: its NoteOff is acknowledged but the envelope runs to its end", "[fade_event_log][pulso_one_shot]") {
    VoiceAllocator a;
    a.init(48000);
    FadeEventLog log(16);
    a.setFadeEventLog(&log);
    ScheduledAudioEvent n = on(1, 0, MusicRole::Pulso, 55.0f);
    n.durationFrames = 2400;                       // 50 ms planned, shorter than the designed decay
    n.timbreParam = 0.30f;                         // decay 75 + 105 * 0.30 = 106.5 ms (F4c, D-M2-37)
    const int slot = a.onNoteOn(n);
    REQUIRE(slot >= 0);
    render(a, 0, 2400);
    a.checkLifetimes(2400);                         // the duration expiry must not truncate it
    REQUIRE(a.onNoteOff(off(1, 2400, MusicRole::Pulso)) == slot);   // acknowledged (Applied), not ignored
    REQUIRE(a.slot(static_cast<std::size_t>(slot)).state != VoiceState::Release);
    REQUIRE(a.slot(static_cast<std::size_t>(slot)).state != VoiceState::Idle);
    REQUIRE(log.records().empty());                // no release fade logged
    render(a, 2400, 2400);                          // 100 ms: still sounding (1 ms + 106.5 ms)
    REQUIRE(a.slot(static_cast<std::size_t>(slot)).state != VoiceState::Idle);
    render(a, 4800, 2400);                          // 150 ms: past the end of the envelope
    REQUIRE(a.slot(static_cast<std::size_t>(slot)).state == VoiceState::Idle);
}
