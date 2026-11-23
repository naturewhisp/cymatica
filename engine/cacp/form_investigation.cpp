#include "form_investigation.h"

#include "diversity.h"

#include <filesystem>
#include <fstream>

namespace cymatica::cacp {

std::string runFormInvestigationV1(const std::string& outputDir) {
    namespace fs = std::filesystem;
    const fs::path dir = fs::path(outputDir) / "form_investigation_v1";
    std::error_code ec;
    fs::create_directories(dir, ec);
    const fs::path path = dir / "plan_tokens.csv";
    std::ofstream out(path, std::ios::binary);
    out << "plan,role,onset,midi,duration_class\n";
    const auto dump = [&](const std::string& name, const FixturePlan& plan) {
        if (!plan.ok) return false;
        for (const auto& t : planTokens(plan)) {
            out << name << ',' << t.role << ',' << t.onset << ',' << t.midi << ',' << t.durationClass << '\n';
        }
        return true;
    };
    bool ok = dump("ref", planFixtureM24V1(1337));
    for (std::uint64_t s = kDiversitySeedFirst; s <= kDiversitySeedLast; ++s) {
        ok = dump(std::to_string(s), planFixtureM24V1Procedural(s)) && ok;
    }
    out.close();
    return (ok && out.good()) ? path.string() : std::string{};
}

} // namespace cymatica::cacp
