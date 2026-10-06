#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <utility>

namespace cymatica::audio {

// Lock-free, wait-free Triple Buffer for latest-value exchange (Spec §20.6, ADR-0002).
// Invariant: backIdx (producer), frontIdx (consumer), and shared (intermediate)
// are always three distinct slot indices in {0, 1, 2}.
// The shared word packs slot index (bits 0..6) and NEW_FLAG (bit 7).
template <typename T>
class TripleBuffer {
public:
    TripleBuffer() noexcept = default;

    explicit TripleBuffer(const T& initial) noexcept {
        slots_[0] = initial;
        slots_[1] = initial;
        slots_[2] = initial;
    }

    // --- Producer API (Writer thread only) ---
    [[nodiscard]] T& writeSlot() noexcept {
        return slots_[backIdx_];
    }

    void publish() noexcept {
        // Exchange back slot into shared with NEW_FLAG (0x80) set
        const std::uint8_t prev = sharedState_.exchange(backIdx_ | kNewDataFlag, std::memory_order_acq_rel);
        // The slot returned by shared (masking out flag) becomes the writer's next back slot
        backIdx_ = prev & kSlotMask;
    }

    // --- Consumer API (Reader thread only) ---
    // Returns true if a new frame was acquired, false if no new frame was published.
    // Bounded CAS loop (Spec §20.6, DIF-M1-12) to guarantee deterministic realtime safety.
    bool update() noexcept {
        std::uint8_t current = sharedState_.load(std::memory_order_acquire);
        for (int attempt = 0; attempt < 4; ++attempt) {
            if (!(current & kNewDataFlag)) {
                return false; // No new data published since last update
            }
            // Atomically swap reader's front slot into shared without NEW_FLAG
            if (sharedState_.compare_exchange_weak(current, frontIdx_, std::memory_order_acq_rel, std::memory_order_acquire)) {
                frontIdx_ = current & kSlotMask;
                return true;
            }
        }
        return false;
    }

    [[nodiscard]] const T& readSlot() const noexcept {
        return slots_[frontIdx_];
    }

private:
    static constexpr std::uint8_t kNewDataFlag = 0x80;
    static constexpr std::uint8_t kSlotMask    = 0x7F;

    T slots_[3]{};
    std::uint8_t backIdx_{0};                          // Owned exclusively by writer thread
    std::uint8_t frontIdx_{1};                         // Owned exclusively by reader thread
    std::atomic<std::uint8_t> sharedState_{2};         // Shared intermediate slot (2 initially, flag 0)
};

#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable: 4324) // structure was padded due to alignment specifier (DIF-M1-06)
#endif

// Single-Producer Single-Consumer Bounded Queue (Spec §20.5, §20.7, ADR-0002)
// Wait-free, cache-line aligned to prevent false sharing.
template <typename T, std::size_t Capacity>
class SpscQueue {
    static_assert((Capacity & (Capacity - 1)) == 0, "Capacity must be a power of two");
    static_assert(Capacity >= 2, "Capacity must be at least 2");

public:
    SpscQueue() noexcept : head_(0), tail_(0) {}

    // Producer method (Writer thread only)
    bool tryPush(const T& item) noexcept {
        const std::size_t currentHead = head_.load(std::memory_order_relaxed);
        const std::size_t currentTail = tail_.load(std::memory_order_acquire);

        if ((currentHead - currentTail) >= Capacity) {
            return false; // Queue full
        }

        buffer_[currentHead & (Capacity - 1)] = item;
        head_.store(currentHead + 1, std::memory_order_release);
        return true;
    }

    // Consumer method (Reader thread only)
    bool tryPop(T& item) noexcept {
        const std::size_t currentTail = tail_.load(std::memory_order_relaxed);
        const std::size_t currentHead = head_.load(std::memory_order_acquire);

        if (currentTail == currentHead) {
            return false; // Queue empty
        }

        item = buffer_[currentTail & (Capacity - 1)];
        tail_.store(currentTail + 1, std::memory_order_release);
        return true;
    }

    [[nodiscard]] bool empty() const noexcept {
        return tail_.load(std::memory_order_relaxed) == head_.load(std::memory_order_relaxed);
    }

    [[nodiscard]] std::size_t size() const noexcept {
        const std::size_t h = head_.load(std::memory_order_relaxed);
        const std::size_t t = tail_.load(std::memory_order_relaxed);
        return h >= t ? h - t : 0;
    }

    [[nodiscard]] static constexpr std::size_t capacity() noexcept {
        return Capacity;
    }

private:
    T buffer_[Capacity]{};
    alignas(64) std::atomic<std::size_t> head_{0};
    alignas(64) std::atomic<std::size_t> tail_{0};
};

#if defined(_MSC_VER)
#pragma warning(pop)
#endif

} // namespace cymatica::audio
