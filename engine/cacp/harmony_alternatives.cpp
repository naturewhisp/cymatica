#include "harmony_alternatives.h"

#include "diversity.h"
#include "music_planner.h"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <set>

namespace cymatica::cacp {

using nlohmann::json;

namespace {

constexpr std::int64_t kStepsPerBar = kDiversityGridPerBar; // sixteenths, 4/4 in every section of the fixture
constexpr std::uint32_t kSectionB = 1;
constexpr std::size_t kCorpo = 1;
constexpr std::size_t kVettore = 3;

int pitchClass(int midi) { return ((midi % 12) + 12) % 12; }

bool isChordTone(const audio::PhrasePlan& p, const audio::ChordSpec& c, int midi) {
    for (const auto degree : {c.rootDegree, c.thirdDegree, c.fifthDegree}) {
        if (pitchClass(audio::chordToneMidi(p.scale, c, degree)) == pitchClass(midi)) return true;
    }
    return false;
}

bool sameChord(const audio::ChordSpec& a, const audio::ChordSpec& b) {
    return a.rootDegree == b.rootDegree && a.thirdDegree == b.thirdDegree && a.fifthDegree == b.fifthDegree &&
           a.thirdChromaticOffset == b.thirdChromaticOffset;
}

} // namespace

std::string HarmonyAlternativesResult::toJson() const {
    json alts = json::array();
    for (const auto& a : alternatives) {
        alts.push_back({{"alternative", a.alternative},
                        {"plans_ok", a.plansOk},
                        {"false_relations", a.falseRelations},
                        {"vettore_corpo_clashes", a.vettoreCorpoClashes},
                        {"change_onsets", a.changeOnsets},
                        {"change_chord_tones", a.changeChordTones},
                        {"real_change_onsets", a.realChangeOnsets},
                        {"real_change_chord_tones", a.realChangeChordTones},
                        {"adapted_notes", a.adaptedNotes},
                        {"repaired_bars", a.repairedBars},
                        {"register_unmet_phrases", a.registerUnmet},
                        {"distinct_corpo_groups", a.distinctCorpoGroups},
                        {"leading_tone_onsets", {{"pulso", a.leadingToneOnsets[0]}, {"corpo", a.leadingToneOnsets[1]},
                                                 {"trama", a.leadingToneOnsets[2]}, {"vettore", a.leadingToneOnsets[3]}}}});
    }
    return json{{"scenario", "harmony_alternatives_v1"},
                {"status", status},
                {"reasons", reasons},
                {"seeds", {seedFirst, seedLast}},
                {"alternatives", alts},
                {"drawn_count", drawnCount},
                {"manifest", manifestFilePath}}
        .dump(2);
}

HarmonyAlternativesResult runHarmonyAlternativesV1(const std::string& outputDir, bool writeFiles, std::uint64_t seedFirst,
                                                   std::uint64_t seedLast) {
    HarmonyAlternativesResult r;
    r.seedFirst = seedFirst;
    r.seedLast = seedLast;
    const auto clock = audio::MusicPlanner::makeFixtureM24V1Clock();
    if (!clock) {
        r.status = "INCONCLUSIVE";
        r.reasons.push_back("fixture clock unavailable");
        return r;
    }
    std::array<std::set<std::uint64_t>, 3> corpoGroups;

    for (std::uint64_t seed = seedFirst; seed <= seedLast; ++seed) {
        const audio::FixtureVariant drawn = audio::MusicPlanner::drawFixtureVariant(seed, audio::kPlannerPolicyVersion);
        ++r.drawnCount[drawn.bHarmony % 3];
        for (std::uint8_t alt = 0; alt < 3; ++alt) {
            auto& st = r.alternatives[alt];
            st.alternative = alt;
            audio::FixtureVariant v = drawn;
            v.bHarmony = alt;
            const FixturePlan plan = planFixtureVariant(seed, v);
            if (!plan.ok) {
                r.reasons.push_back("seed " + std::to_string(seed) + " alternative " + std::to_string(alt) + ": " + plan.error);
                continue;
            }
            ++st.plansOk;
            const auto phrases = audio::MusicPlanner::makeFixtureM24V1Phrases(*clock, v);
            const auto tokens = planTokens(plan);

            // Chord of a global bar (every fixture phrase has 4 bars)
            const auto phraseOf = [&](std::int64_t bar) -> const audio::PhrasePlan& {
                return phrases[static_cast<std::size_t>(std::clamp<std::int64_t>(bar / 4, 0, 23))];
            };
            const auto chordOf = [&](std::int64_t bar) -> const audio::ChordSpec& {
                return phraseOf(bar).chords[static_cast<std::size_t>(bar % 4)];
            };
            const auto inB = [&](std::int64_t bar) { return phraseOf(bar).sectionIndex == kSectionB; };

            for (const auto& t : tokens) {
                if (t.midi < 0 || t.role > kVettore) continue;
                const std::int64_t end = t.onset + static_cast<std::int64_t>(std::max<std::uint32_t>(1, t.durationClass));
                // H-FR: a note on the natural pitch class of an altered third sounding in one of its bars
                bool falseRelation = false;
                for (std::int64_t bar = t.onset / kStepsPerBar; bar * kStepsPerBar < end; ++bar) {
                    if (bar < 0 || bar >= 96 || !inB(bar)) continue;
                    const int natural = audio::naturalThirdPitchClass(phraseOf(bar).scale, chordOf(bar));
                    if (natural >= 0 && pitchClass(t.midi) == natural) falseRelation = true;
                }
                st.falseRelations += falseRelation ? 1 : 0;

                const std::int64_t bar = t.onset / kStepsPerBar;
                if (bar < 0 || bar >= 96 || !inB(bar)) continue;
                const auto& phrase = phraseOf(bar);
                const auto& chord = chordOf(bar);
                // H-LT: onsets on the altered third
                if (chord.thirdChromaticOffset != 0 &&
                    pitchClass(t.midi) == pitchClass(audio::chordToneMidi(phrase.scale, chord, chord.thirdDegree))) {
                    ++st.leadingToneOnsets[t.role];
                }
                if (t.role != kVettore) continue;
                // H-IC: interval class 1 or 6 with a Corpo note sounding at the onset
                for (const auto& c : tokens) {
                    if (c.role != kCorpo || c.midi < 0) continue;
                    if (c.onset > t.onset || t.onset >= c.onset + static_cast<std::int64_t>(c.durationClass)) continue;
                    const int d = pitchClass(t.midi - c.midi);
                    const int ic = d > 6 ? 12 - d : d;
                    if (ic == 1 || ic == 6) {
                        ++st.vettoreCorpoClashes;
                        break;
                    }
                }
                // H-CT: step 0 of a chord change, with the planner definition (bar 0 compares with bar 3 of the phrase)
                if (t.onset % kStepsPerBar != 0) continue;
                const std::int64_t b = bar % 4;
                const bool sectionStart = (b == 0 && phrase.phraseIndex == 0);
                const auto& plannerPrev = phrase.chords[static_cast<std::size_t>((b + 3) % 4)];
                if (sectionStart || !sameChord(plannerPrev, chord)) {
                    ++st.changeOnsets;
                    st.changeChordTones += isChordTone(phrase, chord, t.midi) ? 1 : 0;
                }
                if (sectionStart || !sameChord(chordOf(bar - 1), chord)) {
                    ++st.realChangeOnsets;
                    st.realChangeChordTones += isChordTone(phrase, chord, t.midi) ? 1 : 0;
                }
            }
            for (const auto& tr : plan.traces) {
                if (tr.sectionIndex != kSectionB) continue;
                st.adaptedNotes += tr.vettoreAdaptedNotes;
                st.repairedBars += tr.corpoDyadRepairedBars;
                st.registerUnmet += tr.vettoreRegisterUnmet ? 1 : 0;
            }
            for (std::size_t i = 0; i < plan.phrases.size(); ++i) {
                if (plan.phrases[i].section != kSectionB) continue;
                const auto fp = phraseContentFingerprint(plan.intentsPerPhrase[i], plan.noteOffs, plan.phrases[i].startFrame,
                                                         48000, audio::MusicRole::Corpo);
                if (fp) corpoGroups[alt].insert(*fp);
            }
        }
    }
    for (std::size_t a = 0; a < 3; ++a) r.alternatives[a].distinctCorpoGroups = corpoGroups[a].size();
    r.status = r.reasons.empty() ? "MEASURED" : "INCONCLUSIVE";

    if (writeFiles) {
        namespace fs = std::filesystem;
        std::error_code ec;
        fs::create_directories(outputDir, ec);
        r.manifestFilePath = (fs::path(outputDir) / "harmony_alternatives_v1_manifest.json").string();
        std::ofstream f(r.manifestFilePath);
        f << r.toJson();
        r.manifestWriteSuccess = f.good();
    }
    return r;
}

} // namespace cymatica::cacp
