#include "plan_export.h"

#include "diversity.h"

#include <filesystem>
#include <fstream>
#include <iomanip>

namespace cymatica::cacp {

std::string runPlanExportV1(const std::string& outputDir) {
    namespace fs = std::filesystem;
    const FixturePlan plan = planFixtureM24V1(1337);
    if (!plan.ok) return {};
    std::error_code ec;
    const fs::path dir = fs::path(outputDir) / "plan_export_v1";
    fs::create_directories(dir, ec);
    const fs::path file = dir / "fixture_intents_seed1337.csv";
    std::ofstream o(file);
    o << "event_id,section,phrase,role,sample_frame,duration_frames,pitch_hz,velocity,timbre\n";
    o << std::fixed;
    for (std::size_t p = 0; p < plan.intentsPerPhrase.size() && p < plan.phrases.size(); ++p) {
        for (const auto& e : plan.intentsPerPhrase[p]) {
            o << e.eventId << ',' << plan.phrases[p].section << ',' << p << ',' << static_cast<int>(e.role) << ',' << e.sampleFrame << ','
              << e.durationFrames << ',' << std::setprecision(4) << e.pitchHz << ',' << e.velocity << ',' << e.timbreParam << '\n';
        }
    }
    for (const auto& n : plan.noteOffs) {
        o << "#noteoff," << n.eventId << ',' << static_cast<int>(n.role) << ',' << n.sampleFrame << '\n';
    }
    return o.good() ? file.lexically_normal().string() : std::string{};
}

} // namespace cymatica::cacp
