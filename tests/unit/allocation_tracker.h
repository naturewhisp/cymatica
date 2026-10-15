#pragma once

#include <atomic>
#include <cstddef>

namespace cymatica::test {

extern std::atomic<bool> gTrackAllocations;
extern std::atomic<std::size_t> gAllocationCount;

struct AllocationScope {
    AllocationScope();
    ~AllocationScope();
    [[nodiscard]] std::size_t count() const noexcept;

#if defined(_MSC_VER) && defined(_DEBUG)
private:
    void* oldHook_{nullptr};
#endif
};

} // namespace cymatica::test
