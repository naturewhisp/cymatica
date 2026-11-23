#include "warped_plan_render.h"

#include "audio_engine.h"
#include "audio_scheduler.h"
#include "evaluation_pack.h"
#include "music_intent.h"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

namespace cymatica::cacp {

using nlohmann::json;

namespace {

constexpr std::uint32_t kFs = 48000;
constexpr std::uint32_t kTick = 400;
constexpr std::uint64_t kMaxFrames = 12'000'000; // 250 s

struct Row {
    audio::MusicIntentEvent event{};
    std::uint64_t noteOff{0}; // 0 = duration note
};

// Empty on any malformed line (fail-closed)
std::vector<Row> readRows(const std::filesystem::path& path) {
    std::vector<Row> rows;
    std::ifstream in(path);
    std::string line;
    while (std::getline(in, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty()) continue;
        std::vector<std::string> f;
        std::stringstream ss(line);
        for (std::string cell; std::getline(ss, cell, ',');) f.push_back(cell);
        if (f.size() == 7) f.emplace_back();
        if (f.size() != 8) return {};
        try {
            Row r;
            const unsigned long role = std::stoul(f[1]);
            if (role > 3) return {};
            r.event.eventId = std::stoull(f[0]);
            r.event.role = static_cast<audio::MusicRole>(role);
            r.event.sampleFrame = std::stoull(f[2]);
            r.event.durationFrames = static_cast<std::uint32_t>(std::stoul(f[3]));
            r.event.pitchHz = std::stof(f[4]);
            r.event.velocity = std::stof(f[5]);
            r.event.timbreParam = std::stof(f[6]);
            r.event.transportEpoch = 1;
            r.noteOff = f[7].empty() ? 0 : std::stoull(f[7]);
            if (r.event.sampleFrame > kMaxFrames || r.noteOff > kMaxFrames || !(r.event.pitchHz > 0.0f)) return {};
            rows.push_back(r);
        } catch (const std::exception&) {
            return {};
        }
    }
    return rows;
}

} // namespace

std::string runWarpedPlanRenderV1(const std::string& outputDir) {
    namespace fs = std::filesystem;
    const fs::path dir = fs::path(outputDir) / "tempo_ramp_v1";
    json out{{"scenario", "warped_plan_render_v1"}, {"renders", json::array()}, {"reasons", json::array()}};
    std::vector<fs::path> inputs;
    std::error_code ec;
    for (const auto& e : fs::directory_iterator(dir, ec)) {
        if (e.path().extension() == ".csv") inputs.push_back(e.path());
    }
    std::sort(inputs.begin(), inputs.end());
    if (inputs.empty()) return {};
    for (const auto& path : inputs) {
        const std::string name = path.stem().string();
        const auto rows = readRows(path);
        if (rows.empty()) {
            out["reasons"].push_back(name + ": no valid events");
            continue;
        }
        audio::AudioEngine engine;
        if (!engine.initOffline({kFs, 2, 220.0f})) {
            out["reasons"].push_back(name + ": offline engine initialisation failed");
            continue;
        }
        audio::AudioScheduler scheduler({.lookaheadFrames = 7200, .sampleRate = kFs});
        engine.sendCommand(audio::AudioCommand{.commandId = 1, .type = audio::AudioCommandType::Start, .targetSampleFrame = 0});
        std::uint64_t end = 0;
        for (const auto& r : rows) {
            scheduler.scheduleIntent(r.event, 1);
            end = (std::max)(end, r.event.sampleFrame + r.event.durationFrames);
            if (r.noteOff > 0) {
                scheduler.scheduleNoteOff(r.event.eventId, r.event.role, r.noteOff, 1);
                end = (std::max)(end, r.noteOff);
            }
        }
        const std::uint64_t ticks = (end + kFs + kTick - 1) / kTick;
        std::vector<float> pcm;
        pcm.reserve(static_cast<std::size_t>(ticks) * kTick * 2);
        std::vector<float> block(kTick * 2);
        for (std::uint64_t t = 0; t < ticks; ++t) {
            (void)scheduler.dispatchDueEvents(t * kTick, engine);
            engine.processBlock(block.data(), kTick);
            pcm.insert(pcm.end(), block.begin(), block.end());
            audio::AudioCommandAck ack{};
            while (engine.pollAck(ack)) {}
        }
        const auto bytes = encodeWavStereo16(pcm, kFs);
        std::ofstream wav(dir / (name + ".wav"), std::ios::binary);
        wav.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
        const bool finite = std::all_of(pcm.begin(), pcm.end(), [](float x) { return std::isfinite(x); });
        const float gr = engine.masterLimiter().maxGainReductionDb();
        if (!wav.good() || !finite || scheduler.stagedEventCount() != 0 || scheduler.backpressureDrops() != 0) {
            out["reasons"].push_back(name + ": invalid render");
        }
        out["renders"].push_back({{"name", name},
                                  {"events", rows.size()},
                                  {"dispatched", scheduler.totalDispatchedEvents()},
                                  {"staged_at_end", scheduler.stagedEventCount()},
                                  {"backpressure_drops", scheduler.backpressureDrops()},
                                  {"voice_drops", engine.voiceAllocator().voiceDropCount()},
                                  {"orphans", engine.voiceAllocator().orphanNoteRecoveredCount()},
                                  {"max_gain_reduction_db", gr}});
        engine.shutdown();
    }
    out["status"] = out["reasons"].empty() ? "MEASURED" : "INCONCLUSIVE";
    std::ofstream f(dir / "warped_plan_render_v1_manifest.json");
    f << out.dump(2);
    f.close();
    return f.good() ? out.dump(2) : std::string{};
}

} // namespace cymatica::cacp
