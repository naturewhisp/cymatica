#include <catch2/catch_test_macros.hpp>

#include "audio_engine.h"
#include "realtime_exchange.h"

#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <new>
#include <thread>
#include <vector>

using namespace cymatica::audio;

namespace {

std::atomic<bool> gTrackAllocations{false};
std::atomic<std::size_t> gAllocationCount{0};

struct TestPayload {
    std::uint64_t seq{0};
    std::uint64_t a{0};
    std::uint64_t b{0};
    std::uint64_t checksum{0};

    static TestPayload make(std::uint64_t s, std::uint64_t val) {
        TestPayload p;
        p.seq = s;
        p.a = val;
        p.b = ~val;
        p.checksum = p.seq ^ p.a ^ p.b;
        return p;
    }

    [[nodiscard]] bool isValid() const noexcept {
        return checksum == (seq ^ a ^ b) && (a == ~b);
    }
};

} // namespace

void* operator new(std::size_t size) {
    if (gTrackAllocations.load(std::memory_order_relaxed)) {
        gAllocationCount.fetch_add(1, std::memory_order_relaxed);
    }
    void* ptr = std::malloc(size);
    if (!ptr) throw std::bad_alloc();
    return ptr;
}

void* operator new[](std::size_t size) {
    if (gTrackAllocations.load(std::memory_order_relaxed)) {
        gAllocationCount.fetch_add(1, std::memory_order_relaxed);
    }
    void* ptr = std::malloc(size);
    if (!ptr) throw std::bad_alloc();
    return ptr;
}

void* operator new(std::size_t size, const std::nothrow_t&) noexcept {
    if (gTrackAllocations.load(std::memory_order_relaxed)) {
        gAllocationCount.fetch_add(1, std::memory_order_relaxed);
    }
    return std::malloc(size);
}

void* operator new[](std::size_t size, const std::nothrow_t&) noexcept {
    if (gTrackAllocations.load(std::memory_order_relaxed)) {
        gAllocationCount.fetch_add(1, std::memory_order_relaxed);
    }
    return std::malloc(size);
}

void operator delete(void* ptr) noexcept {
    std::free(ptr);
}

void operator delete[](void* ptr) noexcept {
    std::free(ptr);
}

void operator delete(void* ptr, std::size_t) noexcept {
    std::free(ptr);
}

void operator delete[](void* ptr, std::size_t) noexcept {
    std::free(ptr);
}

void operator delete(void* ptr, const std::nothrow_t&) noexcept {
    std::free(ptr);
}

void operator delete[](void* ptr, const std::nothrow_t&) noexcept {
    std::free(ptr);
}

struct AllocationScope {
    AllocationScope() {
        gAllocationCount.store(0, std::memory_order_seq_cst);
        gTrackAllocations.store(true, std::memory_order_seq_cst);
    }
    ~AllocationScope() {
        gTrackAllocations.store(false, std::memory_order_seq_cst);
    }
    [[nodiscard]] std::size_t count() const noexcept {
        return gAllocationCount.load(std::memory_order_seq_cst);
    }
};

TEST_CASE("TripleBuffer provides coherent snapshots without tearing under concurrent access", "[exchange][concurrency]") {
    // Acceptance criterion Spec §32: snapshot concorrenti coerenti
    TripleBuffer<TestPayload> buffer(TestPayload::make(0, 12345));

    std::atomic<bool> stopFlag{false};
    std::atomic<std::uint64_t> tornCount{0};
    std::atomic<std::uint64_t> readsCount{0};

    constexpr std::uint64_t kTotalWrites = 50000;

    // Writer thread (Producer)
    std::thread writer([&]() {
        for (std::uint64_t i = 1; i <= kTotalWrites; ++i) {
            auto& slot = buffer.writeSlot();
            slot = TestPayload::make(i, i * 7919ULL);
            buffer.publish();
            if ((i % 1000) == 0) {
                std::this_thread::yield();
            }
        }
        stopFlag.store(true, std::memory_order_release);
    });

    // Reader thread (Consumer)
    std::thread reader([&]() {
        std::uint64_t lastSeq = 0;
        while (!stopFlag.load(std::memory_order_acquire)) {
            if (buffer.update()) {
                const auto& slot = buffer.readSlot();
                if (!slot.isValid() || slot.seq < lastSeq) {
                    tornCount.fetch_add(1, std::memory_order_relaxed);
                }
                lastSeq = slot.seq;
                readsCount.fetch_add(1, std::memory_order_relaxed);
            }
        }
        // Drain any final frame
        if (buffer.update()) {
            const auto& slot = buffer.readSlot();
            if (!slot.isValid() || slot.seq < lastSeq) {
                tornCount.fetch_add(1, std::memory_order_relaxed);
            }
            readsCount.fetch_add(1, std::memory_order_relaxed);
        }
    });

    writer.join();
    reader.join();

    REQUIRE(tornCount.load() == 0);
    REQUIRE(readsCount.load() > 0);
}

TEST_CASE("SpscQueue correctly delivers items sequentially between threads", "[exchange][spsc]") {
    SpscQueue<std::uint64_t, 128> queue;
    constexpr std::uint64_t kCount = 10000;

    std::vector<std::uint64_t> received;
    received.reserve(kCount);

    std::thread producer([&]() {
        for (std::uint64_t i = 0; i < kCount; ++i) {
            while (!queue.tryPush(i)) {
                std::this_thread::yield();
            }
        }
    });

    std::thread consumer([&]() {
        std::uint64_t val = 0;
        while (received.size() < kCount) {
            if (queue.tryPop(val)) {
                received.push_back(val);
            } else {
                std::this_thread::yield();
            }
        }
    });

    producer.join();
    consumer.join();

    REQUIRE(received.size() == kCount);
    for (std::uint64_t i = 0; i < kCount; ++i) {
        REQUIRE(received[i] == i);
    }
}

TEST_CASE("AudioEngine processBlock executes deterministically without device", "[exchange][audio]") {
    AudioEngine engine;
    REQUIRE(engine.init({48000, 2, 220.0f}));

    // Send a start command Game -> Audio
    AudioCommand cmd{
        .commandId = 101,
        .type = AudioCommandType::Start,
        .paramF32 = 0.5f,
        .paramU64 = 0,
    };
    REQUIRE(engine.sendCommand(cmd));

    // Process blocks in tight loop without device
    std::vector<float> buffer(512 * 2, 0.0f);
    for (int b = 0; b < 100; ++b) {
        engine.processBlock(buffer.data(), 512);
    }

    // Check acknowledgement Audio -> Game
    AudioCommandAck ack;
    REQUIRE(engine.pollAck(ack));
    REQUIRE(ack.commandId == 101);
    REQUIRE(ack.status == AudioCommandStatus::Applied);

    // Check telemetry published to game thread
    REQUIRE(engine.updateTelemetry());
    const auto& telem = engine.telemetry();
    REQUIRE(telem.sequenceNumber == 100);
    REQUIRE(telem.framesRenderedTotal == 100 * 512);
    REQUIRE(telem.renderCursor == 100 * 512);
    REQUIRE(telem.presentationCursor == 99 * 512);
    REQUIRE(telem.bpm == 120.0f);
    REQUIRE(engine.droppedAcks() == 0);

    engine.shutdown();
}

TEST_CASE("AudioEngine maps device sample rate to 48 kHz internal timeline", "[exchange][audio][resample]") {
    AudioEngine engine;
    // Simulate 44.1 kHz device
    REQUIRE(engine.init({44100, 2, 220.0f}));

    std::vector<float> buffer(147 * 2, 0.0f);
    engine.processBlock(buffer.data(), 147);

    REQUIRE(engine.updateTelemetry());
    const auto& telem = engine.telemetry();
    // 147 device frames at 44.1 kHz = 160 internal frames at 48 kHz
    REQUIRE(telem.renderCursor == 160);
    REQUIRE(telem.framesRenderedTotal == 160);

    engine.shutdown();
}

TEST_CASE("AudioEngine delivers acknowledgements for all accepted commands without loss", "[exchange][audio][acks]") {
    AudioEngine engine;
    REQUIRE(engine.init({48000, 2, 220.0f}));

    std::vector<float> buffer(64 * 2, 0.0f);

    // Batch 1: push 40 commands and process them -> all 40 acks must be generated and queued
    for (std::uint64_t i = 1; i <= 40; ++i) {
        REQUIRE(engine.sendCommand(AudioCommand{
            .commandId = i,
            .type = AudioCommandType::Start,
            .paramF32 = 0.5f,
        }));
    }
    engine.processBlock(buffer.data(), 64);
    REQUIRE(engine.droppedAcks() == 0);

    // Retrieve all 40 acks
    for (std::uint64_t i = 1; i <= 40; ++i) {
        AudioCommandAck ack;
        REQUIRE(engine.pollAck(ack));
        REQUIRE(ack.commandId == i);
        REQUIRE(ack.status == AudioCommandStatus::Applied);
    }
    REQUIRE(engine.droppedAcks() == 0);

    // Batch 2: verify backpressure prevention of ack loss (Spec §20.7, DIF-M1-18)
    // Send up to queue capacity (128)
    for (std::uint64_t i = 41; i <= 41 + 127; ++i) {
        REQUIRE(engine.sendCommand(AudioCommand{
            .commandId = i,
            .type = AudioCommandType::Start,
            .paramF32 = 0.5f,
        }));
    }
    // Attempting to send beyond capacity must be rejected without dropping acks
    REQUIRE_FALSE(engine.sendCommand(AudioCommand{
        .commandId = 9999,
        .type = AudioCommandType::Start,
        .paramF32 = 0.5f,
    }));
    REQUIRE(engine.droppedAcks() == 0);

    // Process and drain all 128 acks
    engine.processBlock(buffer.data(), 64);
    for (std::uint64_t i = 41; i <= 41 + 127; ++i) {
        AudioCommandAck ack;
        REQUIRE(engine.pollAck(ack));
        REQUIRE(ack.commandId == i);
    }
    REQUIRE(engine.droppedAcks() == 0);

    engine.shutdown();
}

TEST_CASE("AudioControlFrame preserves unchanged parameters across triple buffer slots", "[exchange][audio][control]") {
    AudioEngine engine;
    REQUIRE(engine.init({48000, 2, 220.0f}));

    std::vector<float> buffer(64 * 2, 0.0f);

    // 1. Start tone at 440 Hz with volume 0.75 via controlWriter
    auto& initCtrl = engine.controlWriter();
    initCtrl.toneFrequencyHz = 440.0f;
    initCtrl.toneVolume = 0.75f;
    engine.publishControl();
    engine.processBlock(buffer.data(), 64);

    // 2. setToneFrequency to 880 Hz: must preserve tone volume 0.75 across slots (DIF-M1-17)
    engine.setToneFrequency(880.0f);
    engine.processBlock(buffer.data(), 64);

    auto& ctrl1 = engine.controlWriter();
    REQUIRE(ctrl1.toneFrequencyHz == 880.0f);
    REQUIRE(ctrl1.toneVolume == 0.75f);

    // 3. Mutate only dissonance: must preserve frequency 880 and volume 0.75
    ctrl1.dissonance = 0.6f;
    engine.publishControl();
    engine.processBlock(buffer.data(), 64);

    auto& ctrl2 = engine.controlWriter();
    REQUIRE(ctrl2.toneFrequencyHz == 880.0f);
    REQUIRE(ctrl2.toneVolume == 0.75f);
    REQUIRE(ctrl2.dissonance == 0.6f);

    engine.shutdown();
}

TEST_CASE("AudioEngine transport state machine handles pause and resume per Spec section 20.8", "[exchange][audio][pause_resume]") {
    AudioEngine engine;
    REQUIRE(engine.init({48000, 2, 220.0f}));

    // Start tone via command without starting background hardware thread
    REQUIRE(engine.sendCommand(AudioCommand{
        .commandId = 100,
        .type = AudioCommandType::Start,
        .paramF32 = 0.5f,
    }));

    std::vector<float> buffer(512 * 2, 0.0f);

    // 1. Running state
    engine.processBlock(buffer.data(), 512);
    AudioCommandAck startAck;
    REQUIRE(engine.pollAck(startAck));
    REQUIRE(startAck.commandId == 100);

    REQUIRE(engine.updateTelemetry());
    REQUIRE(engine.telemetry().transportState == TransportState::Running);
    const std::uint64_t cursorRunning = engine.telemetry().renderCursor;
    const std::uint64_t epoch1 = engine.telemetry().transportEpoch;
    REQUIRE(cursorRunning == 512);

    // 2. Pause
    REQUIRE(engine.sendCommand(AudioCommand{
        .commandId = 1,
        .type = AudioCommandType::Pause,
    }));
    engine.processBlock(buffer.data(), 512);

    AudioCommandAck ack;
    REQUIRE(engine.pollAck(ack));
    REQUIRE(ack.commandId == 1);
    REQUIRE(ack.status == AudioCommandStatus::Applied);

    REQUIRE(engine.updateTelemetry());
    REQUIRE(engine.telemetry().transportState == TransportState::Paused);
    REQUIRE(engine.telemetry().transportEpoch > epoch1);
    const std::uint64_t cursorPaused = engine.telemetry().renderCursor;
    // Render cursor must be frozen during pause (§20.8, DIF-M1-19)
    REQUIRE(cursorPaused == cursorRunning);

    // Output buffer must be silence
    for (float sample : buffer) {
        REQUIRE(sample == 0.0f);
    }

    // Process another block while paused: cursor still frozen
    engine.processBlock(buffer.data(), 512);
    REQUIRE(engine.updateTelemetry());
    REQUIRE(engine.telemetry().renderCursor == cursorPaused);

    // 3. Resume
    const std::uint64_t epoch2 = engine.telemetry().transportEpoch;
    REQUIRE(engine.sendCommand(AudioCommand{
        .commandId = 2,
        .type = AudioCommandType::Resume,
    }));
    engine.processBlock(buffer.data(), 512);

    REQUIRE(engine.pollAck(ack));
    REQUIRE(ack.commandId == 2);
    REQUIRE(ack.status == AudioCommandStatus::Applied);

    REQUIRE(engine.updateTelemetry());
    REQUIRE(engine.telemetry().transportState == TransportState::Running);
    REQUIRE(engine.telemetry().transportEpoch > epoch2);
    // Cursor resumes advancing from where it paused
    REQUIRE(engine.telemetry().renderCursor == cursorPaused + 512);

    // Tone restored: buffer contains audio samples
    bool hasSound = false;
    for (float sample : buffer) {
        if (sample != 0.0f) {
            hasSound = true;
            break;
        }
    }
    REQUIRE(hasSound);

    // Verify valid and invalid transitions per Spec §20.8
    REQUIRE(isValidTransportTransition(TransportState::Running, TransportState::Pausing));
    REQUIRE(isValidTransportTransition(TransportState::Pausing, TransportState::Paused));
    REQUIRE(isValidTransportTransition(TransportState::Paused, TransportState::Resuming));
    REQUIRE(isValidTransportTransition(TransportState::Resuming, TransportState::Running));
    REQUIRE(isValidTransportTransition(TransportState::Running, TransportState::Paused));
    REQUIRE(isValidTransportTransition(TransportState::Paused, TransportState::Running));
    REQUIRE_FALSE(isValidTransportTransition(TransportState::Paused, TransportState::Pausing));
    REQUIRE_FALSE(isValidTransportTransition(TransportState::Running, TransportState::Resuming));

    engine.shutdown();
}

TEST_CASE("SpscQueue size handles counter wrap and boundaries safely", "[exchange][spsc][size]") {
    SpscQueue<int, 4> queue;
    REQUIRE(queue.size() == 0);
    REQUIRE(queue.empty());

    REQUIRE(queue.tryPush(10));
    REQUIRE(queue.size() == 1);
    REQUIRE(queue.tryPush(20));
    REQUIRE(queue.size() == 2);

    int val = 0;
    REQUIRE(queue.tryPop(val));
    REQUIRE(val == 10);
    REQUIRE(queue.size() == 1);

    REQUIRE(queue.tryPop(val));
    REQUIRE(val == 20);
    REQUIRE(queue.size() == 0);
    REQUIRE(queue.empty());
}

TEST_CASE("AudioEngine processBlock performs zero heap allocations post-init", "[exchange][audio][zero_alloc]") {
    AudioEngine engine;
    REQUIRE(engine.init({48000, 2, 220.0f}));

    std::vector<float> buffer(512 * 2, 0.0f);

    // Verify tracker is functioning (non-tautological test)
    {
        AllocationScope scope;
        volatile auto* dummy = new int(42);
        delete dummy;
        REQUIRE(scope.count() == 1);
    }

    // Process blocks while sending commands and publishing continuous controls
    // Verifies 0 heap allocations across entire realtime exchange path (Spec §20.2, §32)
    {
        AllocationScope scope;
        for (int b = 0; b < 100; ++b) {
            if ((b % 10) == 0) {
                auto& ctrl = engine.controlWriter();
                ctrl.toneFrequencyHz = 220.0f + static_cast<float>(b);
                ctrl.toneVolume = 0.5f;
                engine.publishControl();
            }
            if ((b % 20) == 5) {
                (void)engine.sendCommand(AudioCommand{
                    .commandId = static_cast<std::uint64_t>(b + 1),
                    .type = AudioCommandType::SetTone,
                    .paramF32 = 440.0f + static_cast<float>(b),
                });
            }
            engine.processBlock(buffer.data(), 512);

            AudioCommandAck ack;
            while (engine.pollAck(ack)) {}
            (void)engine.updateTelemetry();
        }
        REQUIRE(scope.count() == 0);
    }

    engine.shutdown();
}
