#pragma once

#include "fade_event_log.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace cymatica::cacp {

// Scenario fade_events_v1 (measurement package 2b, docs/research/M2_5_Pkg2b_Fade_Event_Log_Preregistration.md):
// every point where a voice of the fixture stops sounding, from the allocator's fade-event log. Descriptive table
// per role and cause; checks E4 (counts equal the allocator counters), E5 (repeat identical), E6 (PCM equal to the
// golden with the log installed). MEASURED, never a gate.

inline constexpr std::size_t kFadeCauses = 6;
inline constexpr double kFadeReportDbfs = -40.0;   // grouping bin of the table, not a judgement
inline constexpr std::uint32_t kFastFadeFrames = 96; // <= 2 ms

struct FadeCell {
    std::size_t count{0};
    double medianOutDbfs{-1000.0};
    double maxOutDbfs{-1000.0};
    std::size_t fastAboveBin{0};       // fade <= 2 ms with output amplitude above -40 dBFS
    std::uint32_t minFadeFrames{0};
    std::uint32_t maxFadeFrames{0};
};

// Output amplitude in dBFS of a record (envelope amplitude times bus gain); -1000 for zero
[[nodiscard]] double fadeOutputDbfs(const audio::FadeEventRecord& r) noexcept;
// Table [role][cause] from a list of records
[[nodiscard]] std::array<std::array<FadeCell, kFadeCauses>, 4> fadeTable(const std::vector<audio::FadeEventRecord>& records);

struct FadeEventsResult {
    std::string status{"NOT_RUN"};
    std::vector<std::string> reasons{};
    std::array<std::array<FadeCell, kFadeCauses>, 4> table{};
    std::size_t records{0};
    std::uint64_t overflow{0};
    bool e4CountsMatch{false};
    std::array<std::uint32_t, 4> counterSteals{}, counterReleaseCuts{};
    bool e5RepeatIdentical{false};
    std::string goldenPcmStatus{};            // E6
    std::string manifestFilePath{};
    bool manifestWriteSuccess{false};
    [[nodiscard]] std::string tableJson() const;
    [[nodiscard]] std::string toJson() const;
};

[[nodiscard]] FadeEventsResult runFadeEventsV1(const std::string& outputDir = "artifacts/cacp", bool writeFiles = true);

} // namespace cymatica::cacp
