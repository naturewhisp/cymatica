#include <catch2/catch_test_macros.hpp>

#include "audio_engine.h"
#include "audio_scheduler.h"
#include "event_chain.h"
#include "music_planner.h"
#include "plan_canonical_hash.h"
#include "plan_content_fingerprint.h"

#include <cmath>
#include <limits>
#include <vector>

using namespace cymatica::audio;

namespace {

MusicIntentEvent note(std::uint64_t id, std::uint64_t frame, MusicRole role, float hz, std::uint32_t duration) {
    return MusicIntentEvent{.eventId = id, .sampleFrame = frame, .tick = frame / 400, .transportEpoch = 1,
                            .role = role, .pitchHz = hz, .velocity = 0.8f, .durationFrames = duration,
                            .timbreParam = 0.5f};
}

using Plan = std::vector<std::vector<MusicIntentEvent>>;

// Two open Corpo pads (220 and 330 Hz) released by explicit planner NoteOffs, plus a timed Pulso note
struct PadPlan {
    Plan intents;
    std::vector<PlannedNoteOff> noteOffs;
};

PadPlan padPlan(std::uint64_t release220, std::uint64_t release330) {
    PadPlan p;
    p.intents = {{note(10, 0, MusicRole::Corpo, 220.0f, 0), note(11, 0, MusicRole::Corpo, 330.0f, 0),
                  note(12, 0, MusicRole::Pulso, 55.0f, 4800)}};
    p.noteOffs = {PlannedNoteOff{.eventId = 10, .role = MusicRole::Corpo, .sampleFrame = release220, .transportEpoch = 1},
                  PlannedNoteOff{.eventId = 11, .role = MusicRole::Corpo, .sampleFrame = release330, .transportEpoch = 1}};
    return p;
}

std::vector<SchedulerEventRecord> allSent(const std::vector<EventIdentity>& expected) {
    std::vector<SchedulerEventRecord> log;
    for (const auto& id : expected) log.push_back({.id = id, .kind = SchedulerEventKind::Sent});
    return log;
}

std::vector<AppliedEventRecord> allApplied(const std::vector<EventIdentity>& expected) {
    std::vector<AppliedEventRecord> log;
    for (const auto& id : expected) {
        log.push_back({.id = id, .consumedFrame = id.sampleFrame, .disposition = EventDisposition::Applied, .slot = 0});
    }
    return log;
}

ChainReconcileResult reconcile(const std::vector<EventIdentity>& expected, const std::vector<SchedulerEventRecord>& sched,
                               const std::vector<AppliedEventRecord>& applied, ChainPath path = ChainPath::Nominal,
                               std::uint64_t cut = 0, std::vector<EventIdentity> ignored = {}) {
    return reconcileEventChain({.expected = expected, .schedulerLog = sched, .appliedLog = applied,
                                .expectedIgnored = ignored, .path = path, .faultCutFrame = cut, .spscDrained = true});
}

} // namespace

TEST_CASE("Content fingerprint ignores provenance and keeps every musical field (D-M2-12)", "[event_chain][content_fingerprint]") {
    const PadPlan base = padPlan(96000, 144000);
    const auto fp = musicalContentFingerprint(base.intents, base.noteOffs);
    REQUIRE(fp.valid);
    CHECK(fp.noteCount == 3);
    CHECK(fp.explicitReleaseCount == 2);
    CHECK(fp.orphanReleaseCount == 0);

    // Same music, different provenance: event ids (consistently re-keyed), ticks, epochs and seed
    PadPlan renamed = base;
    for (auto& e : renamed.intents[0]) { e.eventId += 1000; e.tick += 7; e.transportEpoch = 3; }
    for (auto& o : renamed.noteOffs) { o.eventId += 1000; o.transportEpoch = 3; }
    std::swap(renamed.intents[0][0], renamed.intents[0][2]); // emission order does not matter either
    const auto fpRenamed = musicalContentFingerprint(renamed.intents, renamed.noteOffs);
    CHECK(fpRenamed.sameContent(fp));

    PhraseDecisionTrace traceA{};
    traceA.runSeed = 1337;
    PhraseDecisionTrace traceB = traceA;
    traceB.runSeed = 4242;
    CHECK(canonicalPlanHash(base.intents, base.noteOffs, {traceA}) != canonicalPlanHash(base.intents, base.noteOffs, {traceB}));
    CHECK(canonicalPlanHash(base.intents, base.noteOffs, {traceA}) != canonicalPlanHash(renamed.intents, renamed.noteOffs, {traceA}));

    // One musical field of one note changes the fingerprint
    PadPlan oneNote = base;
    oneNote.intents[0][2].pitchHz = 58.27f;
    CHECK_FALSE(musicalContentFingerprint(oneNote.intents, oneNote.noteOffs).sameContent(fp));
    PadPlan oneVelocity = base;
    oneVelocity.intents[0][1].velocity = 0.6f;
    CHECK_FALSE(musicalContentFingerprint(oneVelocity.intents, oneVelocity.noteOffs).sameContent(fp));
    PadPlan oneDuration = base;
    oneDuration.intents[0][2].durationFrames = 4801;
    CHECK_FALSE(musicalContentFingerprint(oneDuration.intents, oneDuration.noteOffs).sameContent(fp));
}

TEST_CASE("Content fingerprint binds explicit releases to their notes (DIF-5.0-02)", "[event_chain][content_fingerprint]") {
    const PadPlan base = padPlan(96000, 144000);
    const auto fp = musicalContentFingerprint(base.intents, base.noteOffs);

    // Swapping which pad is released first changes the music: same NoteOn and NoteOff multisets, different notes
    const PadPlan swapped = padPlan(144000, 96000);
    CHECK_FALSE(musicalContentFingerprint(swapped.intents, swapped.noteOffs).sameContent(fp));

    // Swapping the releases of two identical notes leaves the music, and the fingerprint, unchanged
    PadPlan twins = base;
    twins.intents[0][1].pitchHz = 220.0f;
    PadPlan twinsSwapped = twins;
    std::swap(twinsSwapped.noteOffs[0].sampleFrame, twinsSwapped.noteOffs[1].sampleFrame);
    CHECK(musicalContentFingerprint(twins.intents, twins.noteOffs)
              .sameContent(musicalContentFingerprint(twinsSwapped.intents, twinsSwapped.noteOffs)));

    // Derived release (durationFrames) and explicit release at the same frame are different contracts
    const Plan derived{{note(1, 0, MusicRole::Corpo, 220.0f, 48000)}};
    const Plan open{{note(1, 0, MusicRole::Corpo, 220.0f, 0)}};
    const std::vector<PlannedNoteOff> explicitOff{{.eventId = 1, .role = MusicRole::Corpo, .sampleFrame = 48000, .transportEpoch = 1}};
    CHECK_FALSE(musicalContentFingerprint(derived, {}).sameContent(musicalContentFingerprint(open, explicitOff)));

    // A planner NoteOff without a NoteOn is an orphan release, still part of the content
    const std::vector<PlannedNoteOff> orphan{{.eventId = 99, .role = MusicRole::Trama, .sampleFrame = 5000, .transportEpoch = 1}};
    const auto fpOrphan = musicalContentFingerprint(base.intents, [&] { auto v = base.noteOffs; v.push_back(orphan[0]); return v; }());
    CHECK(fpOrphan.orphanReleaseCount == 1);
    CHECK_FALSE(fpOrphan.sameContent(fp));
}

TEST_CASE("Content fingerprint: special values, role filter and real planner determinism", "[event_chain][content_fingerprint]") {
    PadPlan nan = padPlan(96000, 144000);
    nan.intents[0][0].velocity = std::numeric_limits<float>::quiet_NaN();
    const auto bad = musicalContentFingerprint(nan.intents, nan.noteOffs);
    CHECK_FALSE(bad.valid);
    CHECK_FALSE(bad.error.empty());

    const Plan zeroPitch{{note(1, 0, MusicRole::Pulso, 0.0f, 100)}};
    CHECK(musicalContentFingerprint(zeroPitch, {}).valid);
    const Plan negZero{{note(1, 0, MusicRole::Pulso, 55.0f, 100)}};
    Plan negZeroTimbre = negZero;
    negZeroTimbre[0][0].timbreParam = -0.0f;
    Plan posZeroTimbre = negZero;
    posZeroTimbre[0][0].timbreParam = 0.0f;
    CHECK(musicalContentFingerprint(negZeroTimbre, {}).sameContent(musicalContentFingerprint(posZeroTimbre, {})));

    const PadPlan base = padPlan(96000, 144000);
    const auto corpo = musicalContentFingerprint(base.intents, base.noteOffs, 48000, MusicRole::Corpo);
    CHECK(corpo.noteCount == 2);
    CHECK(corpo.explicitReleaseCount == 2);
    const auto pulso = musicalContentFingerprint(base.intents, base.noteOffs, 48000, MusicRole::Pulso);
    CHECK(pulso.noteCount == 1);
    CHECK(pulso.explicitReleaseCount == 0);

    // Two plannings of fixture-m2-4-v1 with the same seed have the same content
    const auto optClock = MusicPlanner::makeFixtureM24V1Clock();
    REQUIRE(optClock.has_value());
    const auto phrases = MusicPlanner::makeFixtureM24V1Phrases(*optClock);
    auto planOnce = [&] {
        MusicPlanner planner({.runSeed = 1337, .candidateCount = 8, .sampleRate = 48000});
        planner.setFormPlan(MusicPlanner::makeFixtureM24V1Form());
        Plan intents;
        for (const auto& p : phrases) {
            std::vector<MusicIntentEvent> out;
            planner.planPhrase(p, *optClock, 0, p.sectionIndex + 1ULL, out, nullptr);
            intents.push_back(out);
        }
        return musicalContentFingerprint(intents, planner.plannedNoteOffs());
    };
    const auto a = planOnce();
    const auto b = planOnce();
    REQUIRE(a.valid);
    CHECK(a.sameContent(b));
    CHECK(a.noteCount == 1392);
}

TEST_CASE("O3 reconciliation: nominal chain and cardinalities (D-M2-12, DIF-5.0-01)", "[event_chain][o3_reconcile]") {
    const PadPlan p = padPlan(96000, 144000);
    const auto expected = expectedAudioEvents(p.intents, p.noteOffs);
    // 3 NoteOn + 1 derived NoteOff (Pulso) + 2 explicit planner NoteOffs: 2 x intents when every note has one release
    REQUIRE(expected.size() == 6);

    const auto r = reconcile(expected, allSent(expected), allApplied(expected));
    INFO(r.firstMismatch);
    CHECK(r.passed);
    CHECK(r.expected == 6);
    CHECK(r.sent == 6);
    CHECK(r.canceled == 0);
    CHECK(r.consumed == 6);
    CHECK(r.applied == 6);

    // The diagnostic tone is outside the musical chain
    auto withTone = allApplied(expected);
    withTone.push_back({.id = {.eventId = 0, .sampleFrame = 10, .type = AudioEventType::ParameterChange},
                        .consumedFrame = 10, .disposition = EventDisposition::Applied});
    const auto rTone = reconcile(expected, allSent(expected), withTone);
    CHECK(rTone.passed);
    CHECK(rTone.parameterChanges == 1);
}

TEST_CASE("O3 reconciliation rejects losses even when observed counters agree (D-M2-12)", "[event_chain][o3_reconcile]") {
    const PadPlan p = padPlan(96000, 144000);
    const auto expected = expectedAudioEvents(p.intents, p.noteOffs);
    const auto sent = allSent(expected);

    SECTION("event lost between scheduler and SPSC: never sent nor canceled") {
        auto s = sent;
        s.pop_back();
        auto a = allApplied(expected);
        a.pop_back();
        CHECK_FALSE(reconcile(expected, s, a).passed);
    }
    SECTION("lost event replaced by a foreign one: counters equal, identities differ") {
        auto a = allApplied(expected);
        a.back().id.eventId = 777;
        const auto r = reconcile(expected, sent, a);
        CHECK(r.consumed == r.sent);
        CHECK_FALSE(r.passed);
    }
    SECTION("duplicated consumption") {
        auto a = allApplied(expected);
        a.push_back(a.front());
        CHECK_FALSE(reconcile(expected, sent, a).passed);
    }
    SECTION("duplicated send") {
        auto s = sent;
        s.push_back(s.front());
        CHECK_FALSE(reconcile(expected, s, allApplied(expected)).passed);
    }
    SECTION("late event dropped on the nominal path") {
        auto a = allApplied(expected);
        a[0].disposition = EventDisposition::Dropped;
        a[0].reason = EventDropReason::StaleFrameOrEpoch;
        CHECK_FALSE(reconcile(expected, sent, a).passed);
    }
    SECTION("applied at a frame different from the planned one") {
        auto a = allApplied(expected);
        a[0].consumedFrame += 1;
        CHECK_FALSE(reconcile(expected, sent, a).passed);
    }
    SECTION("unmatched NoteOff ignored without being declared") {
        auto a = allApplied(expected);
        for (auto& rec : a) {
            if (rec.id.type == AudioEventType::NoteOff) { rec.disposition = EventDisposition::Ignored; rec.slot = -1; break; }
        }
        CHECK_FALSE(reconcile(expected, sent, a).passed);
        // Declared in advance, the same Ignored NoteOff is admitted
        std::vector<EventIdentity> declared;
        for (const auto& rec : a) if (rec.disposition == EventDisposition::Ignored) declared.push_back(rec.id);
        CHECK(reconcile(expected, sent, a, ChainPath::Nominal, 0, declared).passed);
    }
    SECTION("ignored NoteOn is never admitted") {
        auto a = allApplied(expected);
        a[0].disposition = EventDisposition::Ignored;
        CHECK_FALSE(reconcile(expected, sent, a, ChainPath::Nominal, 0, {a[0].id}).passed);
    }
    SECTION("cancellation on the nominal path") {
        auto s = sent;
        s.back().kind = SchedulerEventKind::Canceled;
        s.back().reason = EventCancelReason::Revoked;
        auto a = allApplied(expected);
        a.pop_back();
        CHECK_FALSE(reconcile(expected, s, a).passed);
    }
    SECTION("log overflow or SPSC not drained") {
        auto in = ChainReconcileInput{.expected = expected, .schedulerLog = sent};
        const auto a = allApplied(expected);
        in.appliedLog = a;
        in.spscDrained = true;
        CHECK(reconcileEventChain(in).passed);
        in.appliedLogOverflow = 1;
        CHECK_FALSE(reconcileEventChain(in).passed);
        in.appliedLogOverflow = 0;
        in.schedulerLogOverflow = 1;
        CHECK_FALSE(reconcileEventChain(in).passed);
        in.schedulerLogOverflow = 0;
        in.spscDrained = false;
        CHECK_FALSE(reconcileEventChain(in).passed);
    }
}

TEST_CASE("O3 reconciliation on the injected-fault path (D-M2-12 point 7, DIF-5.0-R2-01)", "[event_chain][o3_reconcile]") {
    const PadPlan p = padPlan(96000, 144000);
    const auto expected = expectedAudioEvents(p.intents, p.noteOffs);
    constexpr std::uint64_t kCut = 50000;

    // Two explicit releases: one revoked before being sent, one committed and invalidated by the Stop
    std::vector<SchedulerEventRecord> sched;
    std::vector<AppliedEventRecord> applied;
    for (const auto& id : expected) {
        if (id.type == AudioEventType::NoteOff && id.eventId == 10) {
            sched.push_back({.id = id, .kind = SchedulerEventKind::Canceled, .reason = EventCancelReason::Revoked});
        } else if (id.type == AudioEventType::NoteOff && id.eventId == 11) {
            sched.push_back({.id = id, .kind = SchedulerEventKind::Sent});
            applied.push_back({.id = id, .consumedFrame = kCut + 400, .disposition = EventDisposition::Dropped,
                               .reason = EventDropReason::StopDrain});
        } else {
            sched.push_back({.id = id, .kind = SchedulerEventKind::Sent});
            applied.push_back({.id = id, .consumedFrame = id.sampleFrame, .disposition = EventDisposition::Applied, .slot = 0});
        }
    }

    const auto r = reconcile(expected, sched, applied, ChainPath::InjectedFault, kCut);
    INFO(r.firstMismatch);
    CHECK(r.passed);
    CHECK(r.canceled == 1);
    CHECK(r.dropped == 1);
    CHECK(r.sent + r.canceled == r.expected);
    CHECK(r.consumed == r.sent);

    SECTION("revoked events never appear in the applied log") {
        auto a = applied;
        for (const auto& rec : sched) {
            if (rec.kind == SchedulerEventKind::Canceled) {
                a.push_back({.id = rec.id, .consumedFrame = kCut, .disposition = EventDisposition::Dropped,
                             .reason = EventDropReason::StopDrain});
            }
        }
        CHECK_FALSE(reconcile(expected, sched, a, ChainPath::InjectedFault, kCut).passed);
    }
    SECTION("drop before the fault cut frame is an arbitrary loss") {
        auto a = applied;
        for (auto& rec : a) if (rec.disposition == EventDisposition::Dropped) rec.consumedFrame = kCut - 1;
        CHECK_FALSE(reconcile(expected, sched, a, ChainPath::InjectedFault, kCut).passed);
    }
    SECTION("drop without an admitted reason") {
        auto a = applied;
        for (auto& rec : a) if (rec.disposition == EventDisposition::Dropped) rec.reason = EventDropReason::None;
        CHECK_FALSE(reconcile(expected, sched, a, ChainPath::InjectedFault, kCut).passed);
    }
    SECTION("cancellation without an admitted reason") {
        auto s = sched;
        for (auto& rec : s) if (rec.kind == SchedulerEventKind::Canceled) rec.reason = EventCancelReason::None;
        CHECK_FALSE(reconcile(expected, s, applied, ChainPath::InjectedFault, kCut).passed);
    }
}

TEST_CASE("Scheduler log: a NoteOff preserved by the revocation and cleared at Stop is canceled, not sent (DIF-5.0-R3-01)",
          "[event_chain][scheduler_log]") {
    AudioEngine engine;
    REQUIRE(engine.initOffline());
    AppliedEventLog appliedLog(64);
    REQUIRE(engine.setAppliedEventLog(&appliedLog));

    AudioScheduler scheduler({.lookaheadFrames = 7200});
    SchedulerEventLog schedLog(64);
    scheduler.setEventLog(&schedLog);

    // Open Corpo note committed at frame 100, its explicit NoteOff far in the future, a later note at 20000
    const MusicIntentEvent pad = note(1, 100, MusicRole::Corpo, 220.0f, 0);
    const MusicIntentEvent later = note(2, 20000, MusicRole::Vettore, 440.0f, 2400);
    scheduler.scheduleIntent(pad, 1);
    scheduler.scheduleNoteOff(1, MusicRole::Corpo, 60000, 1);
    scheduler.scheduleIntent(later, 1);

    REQUIRE(scheduler.dispatchDueEvents(0, engine) == 1); // only the pad NoteOn is within the lookahead
    std::vector<float> block(512 * 2);
    engine.processBlock(block.data(), 512);

    // Failure at frame 10000: the later note and its NoteOff are revoked, the pad NoteOff is preserved
    CHECK(scheduler.revokeFromFrame(10000) == 2);
    CHECK(scheduler.stagedEventCount() == 1);
    // Stop before the preserved NoteOff is dispatched
    scheduler.clear();

    REQUIRE(engine.scheduledEventQueueEmpty());
    REQUIRE(engine.setAppliedEventLog(nullptr));

    std::size_t revoked = 0;
    std::size_t cleared = 0;
    for (const auto& rec : schedLog.records()) {
        if (rec.kind != SchedulerEventKind::Canceled) continue;
        if (rec.reason == EventCancelReason::Revoked) ++revoked;
        if (rec.reason == EventCancelReason::ClearedAtStop) {
            ++cleared;
            CHECK(rec.id.type == AudioEventType::NoteOff);
            CHECK(rec.id.eventId == 1);
        }
    }
    CHECK(revoked == 2);
    CHECK(cleared == 1);
    REQUIRE(appliedLog.size() == 1);
    CHECK(appliedLog.records()[0].disposition == EventDisposition::Applied);
    CHECK(appliedLog.records()[0].consumedFrame == 100);

    const std::vector<std::vector<MusicIntentEvent>> plan{{pad, later}};
    const std::vector<PlannedNoteOff> offs{{.eventId = 1, .role = MusicRole::Corpo, .sampleFrame = 60000, .transportEpoch = 1}};
    const auto expected = expectedAudioEvents(plan, offs);
    std::vector<SchedulerEventRecord> sched(schedLog.records().begin(), schedLog.records().end());
    std::vector<AppliedEventRecord> applied(appliedLog.records().begin(), appliedLog.records().end());

    const auto r = reconcile(expected, sched, applied, ChainPath::InjectedFault, 10000);
    INFO(r.firstMismatch);
    CHECK(r.passed);
    CHECK(r.canceled == 3);

    // Classifying the preserved NoteOff as sent (it never reached the SPSC) must fail
    for (auto& rec : sched) {
        if (rec.reason == EventCancelReason::ClearedAtStop) { rec.kind = SchedulerEventKind::Sent; rec.reason = EventCancelReason::None; }
    }
    CHECK_FALSE(reconcile(expected, sched, applied, ChainPath::InjectedFault, 10000).passed);
}

TEST_CASE("Applied-event log: dispositions, drop reasons, overflow and quiescence (D-M2-12)", "[event_chain][applied_log]") {
    AudioEngine engine;
    REQUIRE(engine.initOffline());
    AppliedEventLog log(2);
    REQUIRE(engine.setAppliedEventLog(&log));

    std::vector<float> block(512 * 2);
    engine.processBlock(block.data(), 512); // cursor at 512

    // Stale (frame already passed), a NoteOff with no matching note, and one event beyond the capacity
    REQUIRE(engine.scheduleAudioEvent({.eventId = 5, .sampleFrame = 10, .transportEpoch = 1, .type = AudioEventType::NoteOn,
                                       .role = MusicRole::Pulso, .valueA = 55.0f}));
    REQUIRE(engine.scheduleAudioEvent({.eventId = 6, .sampleFrame = 600, .transportEpoch = 1, .type = AudioEventType::NoteOff,
                                       .role = MusicRole::Trama}));
    REQUIRE(engine.scheduleAudioEvent({.eventId = 7, .sampleFrame = 700, .transportEpoch = 1, .type = AudioEventType::NoteOn,
                                       .role = MusicRole::Vettore, .valueA = 440.0f, .durationFrames = 100}));
    engine.processBlock(block.data(), 512);
    REQUIRE(engine.scheduledEventQueueEmpty());

    REQUIRE(log.size() == 2);
    CHECK(log.overflow() == 1);
    CHECK(log.records()[0].id.eventId == 5);
    CHECK(log.records()[0].disposition == EventDisposition::Dropped);
    CHECK(log.records()[0].reason == EventDropReason::StaleFrameOrEpoch);
    CHECK(log.records()[1].id.eventId == 6);
    CHECK(log.records()[1].disposition == EventDisposition::Ignored);
    CHECK(log.records()[1].slot == -1);
    CHECK(log.records()[1].consumedFrame == 600);

    REQUIRE(engine.setAppliedEventLog(nullptr));
}
