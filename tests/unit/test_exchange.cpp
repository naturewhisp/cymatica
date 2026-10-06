#include <catch2/catch_test_macros.hpp>

#include "audio_engine.h"
#include "realtime_exchange.h"

#include <atomic>
#include <chrono>
#include <cstdint>
#include <thread>
#include <vector>

using namespace cymatica::audio;

namespace {

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

TEST_CASE("AudioEngine tracks dropped command acks when ack queue overflows", "[exchange][audio][drops]") {
    AudioEngine engine;
    REQUIRE(engine.init({48000, 2, 220.0f}));

    std::vector<float> buffer(64 * 2, 0.0f);

    // Batch 1: push 40 commands and process them -> 40 acks added to ackQueue (capacity 64)
    for (std::uint64_t i = 1; i <= 40; ++i) {
        REQUIRE(engine.sendCommand(AudioCommand{
            .commandId = i,
            .type = AudioCommandType::Start,
            .paramF32 = 0.5f,
        }));
    }
    engine.processBlock(buffer.data(), 64);
    REQUIRE(engine.droppedAcks() == 0);

    // Batch 2: push another 40 commands and process them without reading acks.
    // 24 will fit in ackQueue, 16 will overflow and be dropped.
    for (std::uint64_t i = 41; i <= 80; ++i) {
        REQUIRE(engine.sendCommand(AudioCommand{
            .commandId = i,
            .type = AudioCommandType::Start,
            .paramF32 = 0.5f,
        }));
    }
    engine.processBlock(buffer.data(), 64);

    REQUIRE(engine.droppedAcks() == 16);

    engine.shutdown();
}
