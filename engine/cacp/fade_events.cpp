#include "fade_events.h"

#include "audio_probe.h"
#include "stem_analysis.h"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>

namespace cymatica::cacp {

using nlohmann::json;

namespace {
constexpr std::size_t kFadeLogCapacity = 20000;
}

double fadeOutputDbfs(const audio::FadeEventRecord& r) noexcept {
    const double a = static_cast<double>(r.envAmplitude) * static_cast<double>(r.busGain);
    return a > 0.0 ? 20.0 * std::log10(a) : -1000.0;
}

std::array<std::array<FadeCell, kFadeCauses>, 4> fadeTable(const std::vector<audio::FadeEventRecord>& records) {
    std::array<std::array<FadeCell, kFadeCauses>, 4> t{};
    std::array<std::array<std::vector<double>, kFadeCauses>, 4> levels{};
    for (const auto& r : records) {
        if (r.role >= 4 || static_cast<std::size_t>(r.cause) >= kFadeCauses) continue;
        FadeCell& c = t[r.role][static_cast<std::size_t>(r.cause)];
        const double db = fadeOutputDbfs(r);
        if (c.count == 0) {
            c.minFadeFrames = r.fadeFrames;
            c.maxFadeFrames = r.fadeFrames;
        }
        ++c.count;
        c.minFadeFrames = (std::min)(c.minFadeFrames, r.fadeFrames);
        c.maxFadeFrames = (std::max)(c.maxFadeFrames, r.fadeFrames);
        c.maxOutDbfs = (std::max)(c.maxOutDbfs, db);
        if (r.fadeFrames <= kFastFadeFrames && db > kFadeReportDbfs) ++c.fastAboveBin;
        levels[r.role][static_cast<std::size_t>(r.cause)].push_back(db);
    }
    for (std::size_t role = 0; role < 4; ++role) {
        for (std::size_t k = 0; k < kFadeCauses; ++k) {
            auto& v = levels[role][k];
            if (v.empty()) continue;
            std::sort(v.begin(), v.end());
            const std::size_t n = v.size();
            t[role][k].medianOutDbfs = (n % 2 == 1) ? v[n / 2] : 0.5 * (v[n / 2 - 1] + v[n / 2]);
        }
    }
    return t;
}

FadeEventsResult runFadeEventsV1(const std::string& outputDir, bool writeFiles) {
    auto once = [&](FadeEventsResult& r) {
        audio::FadeEventLog log(kFadeLogCapacity);
        audio::VoiceConfounders conf{};
        audio::ProbeAnalysisCapture pc{};
        pc.fadeLog = &log;
        pc.confounders = &conf;
        const auto probe = audio::runFixtureProbeV1Offline(outputDir, false, audio::FixtureInjectedFault::None, &pc);
        r.goldenPcmStatus = probe.goldenPcmStatus;
        const std::vector<audio::FadeEventRecord> recs(log.records().begin(), log.records().end());
        r.records = recs.size();
        r.overflow = log.overflow();
        r.table = fadeTable(recs);
        r.counterSteals = conf.roleVoiceDrops;
        r.counterReleaseCuts = conf.roleReleaseCuts;
        bool match = true;
        for (std::size_t role = 0; role < 4; ++role) {
            match = match && r.table[role][static_cast<std::size_t>(audio::FadeCause::Steal)].count == conf.roleVoiceDrops[role] &&
                    r.table[role][static_cast<std::size_t>(audio::FadeCause::ReleaseCut)].count == conf.roleReleaseCuts[role];
        }
        r.e4CountsMatch = match;
        if (!probe.passed) r.reasons.push_back("fixture probe failed: " + probe.failureReason);
    };
    FadeEventsResult r;
    once(r);
    FadeEventsResult again;
    once(again);
    r.e5RepeatIdentical = again.tableJson() == r.tableJson() && again.records == r.records;
    if (r.overflow != 0) r.reasons.push_back("fade log overflow");
    if (!r.e4CountsMatch) r.reasons.emplace_back("E4: log counts differ from the allocator counters");
    if (!r.e5RepeatIdentical) r.reasons.emplace_back("E5: repeat run differs");
    if (r.goldenPcmStatus != "PASS") r.reasons.push_back("E6: golden PCM status " + r.goldenPcmStatus);
    r.status = r.reasons.empty() ? "MEASURED" : "INCONCLUSIVE";
    if (writeFiles) {
        r.manifestFilePath = (std::filesystem::path(outputDir) / "fade_events_v1_manifest.json").lexically_normal().string();
        std::ofstream mf(r.manifestFilePath);
        r.manifestWriteSuccess = mf.is_open();
        if (r.manifestWriteSuccess) {
            mf << r.toJson();
            r.manifestWriteSuccess = mf.good();
        }
    }
    return r;
}

std::string FadeEventsResult::tableJson() const {
    json t = json::object();
    for (std::size_t role = 0; role < 4; ++role) {
        json row = json::object();
        for (std::size_t k = 0; k < kFadeCauses; ++k) {
            const FadeCell& c = table[role][k];
            if (c.count == 0) continue;
            row[audio::kFadeCauseNames[k]] = {{"count", c.count},
                                              {"median_out_dbfs", c.medianOutDbfs},
                                              {"max_out_dbfs", c.maxOutDbfs},
                                              {"fade_frames_min", c.minFadeFrames},
                                              {"fade_frames_max", c.maxFadeFrames},
                                              {"fast_fades_above_minus40_dbfs", c.fastAboveBin}};
        }
        t[kRoleNames[role]] = row;
    }
    return t.dump(2);
}

std::string FadeEventsResult::toJson() const {
    json j;
    j["scenario"] = "fade_events_v1";
    j["status"] = status;
    j["reasons"] = reasons;
    j["preregistration"] = "docs/research/M2_5_Pkg2b_Fade_Event_Log_Preregistration.md";
    j["records"] = records;
    j["overflow"] = overflow;
    j["table"] = json::parse(tableJson());
    j["E4_counts_match"] = e4CountsMatch;
    j["E4_counter_steals"] = counterSteals;
    j["E4_counter_release_cuts"] = counterReleaseCuts;
    j["E5_repeat_identical"] = e5RepeatIdentical;
    j["E6_golden_pcm_status"] = goldenPcmStatus;
    j["notes"] = {"output amplitude = envelope amplitude x current bus gain; Corpo ducking and the Trama delay are not included",
                  "fade_frames 0 = oscillation cut with a linear tail of the last sample (steal, release_cut, stop)"};
    return j.dump(2) + "\n";
}

} // namespace cymatica::cacp
