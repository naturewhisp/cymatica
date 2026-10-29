#include "evaluation_pack.h"

#include "acao02_calibration.h"
#include "audio_probe.h"
#include "mel.h"
#include "music_planner.h"
#include "onset.h"
#include "png_writer.h"
#include "sha256.h"
#include "stereo.h"
#include "stft.h"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <set>
#include <sstream>

namespace cymatica::cacp {

using nlohmann::json;

namespace {

constexpr std::array<const char*, 4> kMethods{"listening", "spectrogram", "descriptor", "code"};
constexpr std::array<const char*, 7> kPredicates{"role_signal_present", "role_onset_detected", "role_audible",
                                                 "role_planned",        "stereo_wider_than",   "artifact_present",
                                                 "observation"};
constexpr std::array<const char*, 3> kArtifactKinds{"click", "clipping", "dropout"};
constexpr double kPi = 3.14159265358979323846;

// Isolated selection stream (SplitMix64, Steele, Lea, Flood 2014), never the musical seed
class SelectionRng {
public:
    explicit SelectionRng(std::uint64_t seed) noexcept : state_(seed ^ 0x6576616C7061636BULL) {} // "evalpack"
    std::uint64_t next() noexcept {
        std::uint64_t z = (state_ += 0x9E3779B97F4A7C15ULL);
        z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
        z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
        return z ^ (z >> 31);
    }
    // Unbiased enough for selection among a handful of items (modulo of a 64-bit draw)
    std::size_t below(std::size_t n) noexcept { return n == 0 ? 0 : static_cast<std::size_t>(next() % n); }

private:
    std::uint64_t state_;
};

template <typename T>
void shuffle(std::vector<T>& v, SelectionRng& rng) {
    for (std::size_t i = v.size(); i > 1; --i) std::swap(v[i - 1], v[rng.below(i)]);
}

bool contains(std::span<const char* const> set, std::string_view s) {
    return std::any_of(set.begin(), set.end(), [&](const char* x) { return s == x; });
}

int roleIndex(std::string_view name) {
    for (std::size_t i = 0; i < kRoleCount; ++i) {
        if (name == kRoleNames[i]) return static_cast<int>(i);
    }
    return -1;
}

double dbFromPower(double ratio) { return ratio > 0.0 ? 10.0 * std::log10(ratio) : -std::numeric_limits<double>::infinity(); }

json numOrNull(double v) { return std::isfinite(v) ? json(v) : json(nullptr); }
double numFrom(const json& j) { return j.is_number() ? j.get<double>() : std::numeric_limits<double>::quiet_NaN(); }

std::string hexId(std::uint64_t v) {
    std::ostringstream s;
    s << 'w' << std::hex << std::setw(8) << std::setfill('0') << (v & 0xFFFFFFFFULL);
    return s.str();
}

std::vector<std::uint8_t> bytesOf(const std::string& s) { return {s.begin(), s.end()}; }

std::string sha256Of(const std::vector<std::uint8_t>& b) { return sha256Hex(std::span<const std::uint8_t>(b.data(), b.size())); }

// "hash  name" lines sorted by name, over every file of the map (SHA256SUMS itself excluded)
std::string sha256Sums(const std::map<std::string, std::vector<std::uint8_t>>& files) {
    std::ostringstream s;
    for (const auto& [name, bytes] : files) s << sha256Of(bytes) << "  " << name << '\n';
    return s.str();
}

std::map<std::string, std::string> hashMap(const std::map<std::string, std::vector<std::uint8_t>>& files) {
    std::map<std::string, std::string> out;
    for (const auto& [name, bytes] : files) out[name] = sha256Of(bytes);
    return out;
}

// Mel spectrogram image (descriptors_v1): one column per STFT frame, one row per band (low bands at the bottom),
// grey levels of the normalised dB. A non-Ok status gives a black image of the same size and is recorded.
std::pair<std::string, std::vector<std::uint8_t>> melPng(const std::vector<float>& mono, std::uint32_t sampleRate) {
    const MagnitudeSpectrogram mag = stftMagnitude(mono);
    const MelFilterbank fb = melFilterbank(kDescriptorsV1Mel, kDescriptorsV1Stft, sampleRate);
    const MelSpectrogram mel = melSpectrogramNormalized(mag, fb, kDescriptorsV1Mel.dbRange);
    const std::size_t width = (std::max)(stftFrameCount(mono.size(), kDescriptorsV1Stft), std::size_t{1});
    const std::size_t height = kDescriptorsV1Mel.bands;
    std::vector<std::uint8_t> rgb(width * height * 3, 0);
    if (mel.status == AnalysisStatus::Ok && mel.frames == width && mel.bands == height) {
        for (std::size_t x = 0; x < width; ++x) {
            for (std::size_t b = 0; b < height; ++b) {
                const float v = std::clamp(mel.normalized[x * height + b], 0.0f, 1.0f);
                const auto g = static_cast<std::uint8_t>(std::lround(v * 255.0f));
                const std::size_t y = height - 1 - b;
                const std::size_t o = (y * width + x) * 3;
                rgb[o] = rgb[o + 1] = rgb[o + 2] = g;
            }
        }
    }
    return {analysisStatusName(mel.status),
            encodePngRgb8(static_cast<std::uint32_t>(width), static_cast<std::uint32_t>(height), rgb)};
}

std::vector<float> channelMix(const std::vector<float>& stereo, bool side) {
    std::vector<float> out(stereo.size() / 2);
    for (std::size_t i = 0; i < out.size(); ++i) {
        out[i] = side ? 0.5f * (stereo[2 * i] - stereo[2 * i + 1]) : 0.5f * (stereo[2 * i] + stereo[2 * i + 1]);
    }
    return out;
}

bool overlaps(const RoleSpan& s, std::uint64_t start, std::uint64_t end) { return s.startFrame < end && s.endFrame > start; }

json roleTruthJson(const RoleWindowTruth& r) {
    return json{{"planned", r.planned},
                {"applied", r.applied},
                {"signal", signalLevelName(r.signal)},
                {"peak_dbfs", numOrNull(r.peakDbfs)},
                {"rms_dbfs", numOrNull(r.rmsDbfs)},
                {"stem_to_mix_db", numOrNull(r.stemToMixDb)},
                {"onset_status", r.onsetStatus},
                {"applied_note_ons", r.appliedNoteOns},
                {"matched_onsets", r.onsetStatus == "MEASURED" ? json(r.matchedOnsets) : json(nullptr)},
                {"onset_detected", r.onsetStatus == "MEASURED" ? json(r.onsetDetected) : json(nullptr)}};
}

json windowTruthJson(const WindowTruth& w) {
    json roles = json::object();
    for (std::size_t r = 0; r < kRoleCount; ++r) roles[kRoleNames[r]] = roleTruthJson(w.roles[r]);
    return json{{"id", w.id},
                {"kind", w.kind},
                {"duplicate_of", w.duplicateOf},
                {"removed_role", w.removedRole >= 0 ? json(kRoleNames[static_cast<std::size_t>(w.removedRole)]) : json(nullptr)},
                {"phrase", w.phrase},
                {"section", w.section},
                {"start_bar", w.startBar},
                {"bar_count", w.barCount},
                {"start_frame", w.startFrame},
                {"end_frame", w.endFrame},
                {"gain_db", w.gainDb},
                {"roles", roles},
                {"side_mid_status", w.sideMidStatus},
                {"side_mid_db", w.sideMidDb ? json(*w.sideMidDb) : json(nullptr)},
                {"side_is_zero", w.sideIsZero},
                {"mid_is_zero", w.midIsZero},
                {"clipped_samples", w.clippedSamples},
                {"mel_mid_status", w.melMidStatus},
                {"mel_side_status", w.melSideStatus}};
}

WindowTruth windowTruthFrom(const json& j) {
    WindowTruth w;
    w.id = j.at("id").get<std::string>();
    w.kind = j.at("kind").get<std::string>();
    w.duplicateOf = j.at("duplicate_of").get<std::string>();
    w.removedRole = j.at("removed_role").is_string() ? roleIndex(j.at("removed_role").get<std::string>()) : -1;
    w.phrase = j.at("phrase").get<int>();
    w.section = j.at("section").get<std::uint32_t>();
    w.startBar = j.at("start_bar").get<std::uint32_t>();
    w.barCount = j.at("bar_count").get<std::uint32_t>();
    w.startFrame = j.at("start_frame").get<std::uint64_t>();
    w.endFrame = j.at("end_frame").get<std::uint64_t>();
    w.gainDb = j.at("gain_db").get<double>();
    for (std::size_t r = 0; r < kRoleCount; ++r) {
        const json& rj = j.at("roles").at(kRoleNames[r]);
        RoleWindowTruth& rt = w.roles[r];
        rt.planned = rj.at("planned").get<bool>();
        rt.applied = rj.at("applied").get<bool>();
        rt.signal = signalLevelFromName(rj.at("signal").get<std::string>()).value_or(SignalLevel::Unavailable);
        rt.peakDbfs = numFrom(rj.at("peak_dbfs"));
        rt.rmsDbfs = numFrom(rj.at("rms_dbfs"));
        rt.stemToMixDb = numFrom(rj.at("stem_to_mix_db"));
        rt.onsetStatus = rj.at("onset_status").get<std::string>();
        rt.appliedNoteOns = rj.at("applied_note_ons").get<std::size_t>();
        rt.matchedOnsets = rj.at("matched_onsets").is_null() ? 0 : rj.at("matched_onsets").get<std::size_t>();
        rt.onsetDetected = !rj.at("onset_detected").is_null() && rj.at("onset_detected").get<bool>();
    }
    w.sideMidStatus = j.at("side_mid_status").get<std::string>();
    if (j.at("side_mid_db").is_number()) w.sideMidDb = j.at("side_mid_db").get<double>();
    w.sideIsZero = j.at("side_is_zero").get<bool>();
    w.midIsZero = j.at("mid_is_zero").get<bool>();
    w.clippedSamples = j.at("clipped_samples").get<std::uint64_t>();
    w.melMidStatus = j.at("mel_mid_status").get<std::string>();
    w.melSideStatus = j.at("mel_side_status").get<std::string>();
    return w;
}

json windowsJson(const std::vector<WindowTruth>& v) {
    json a = json::array();
    for (const auto& w : v) a.push_back(windowTruthJson(w));
    return a;
}

std::vector<WindowTruth> windowsFrom(const json& a) {
    std::vector<WindowTruth> v;
    for (const auto& j : a) v.push_back(windowTruthFrom(j));
    return v;
}

std::string dump(const json& j) { return j.dump(2) + "\n"; }

} // namespace

PresenceProfile presenceProfileV1() noexcept { return PresenceProfile{}; }

const char* signalLevelName(SignalLevel s) noexcept {
    switch (s) {
        case SignalLevel::Unavailable: return "UNAVAILABLE";
        case SignalLevel::ExactSilence: return "EXACT_SILENCE";
        case SignalLevel::BelowNumericalFloor: return "BELOW_NUMERICAL_FLOOR";
        case SignalLevel::BelowThreshold: return "BELOW_THRESHOLD";
        case SignalLevel::Present: return "PRESENT";
    }
    return "UNAVAILABLE";
}

std::optional<SignalLevel> signalLevelFromName(std::string_view name) noexcept {
    for (const auto s : {SignalLevel::Unavailable, SignalLevel::ExactSilence, SignalLevel::BelowNumericalFloor,
                         SignalLevel::BelowThreshold, SignalLevel::Present}) {
        if (name == signalLevelName(s)) return s;
    }
    return std::nullopt;
}

std::vector<RoleSpan> plannedSpans(std::span<const audio::MusicIntentEvent> intents,
                                   std::span<const audio::PlannedNoteOff> noteOffs) {
    std::vector<RoleSpan> out;
    out.reserve(intents.size());
    for (const auto& e : intents) {
        std::uint64_t end = e.sampleFrame + e.durationFrames;
        for (const auto& off : noteOffs) {
            if (off.eventId == e.eventId && off.role == e.role) end = off.sampleFrame;
        }
        out.push_back({e.role, e.sampleFrame, (std::max)(end, e.sampleFrame + 1)});
    }
    return out;
}

std::vector<RoleSpan> appliedSpans(std::span<const AppliedEventRecord> applied, std::uint64_t endFrame) {
    std::vector<RoleSpan> out;
    for (const auto& on : applied) {
        if (on.id.type != audio::AudioEventType::NoteOn || on.disposition != EventDisposition::Applied) continue;
        std::uint64_t end = endFrame;
        for (const auto& off : applied) {
            if (off.id.type == audio::AudioEventType::NoteOff && off.disposition == EventDisposition::Applied &&
                off.id.eventId == on.id.eventId && off.id.role == on.id.role && off.id.sampleFrame >= on.id.sampleFrame) {
                end = (std::min)(end, off.id.sampleFrame);
            }
        }
        out.push_back({on.id.role, on.id.sampleFrame, (std::max)(end, on.id.sampleFrame + 1)});
    }
    return out;
}

OnsetIndex buildOnsetIndex(const PackSource& src, double toleranceS) {
    OnsetIndex idx;
    for (std::size_t r = 0; r < kRoleCount; ++r) {
        const auto role = static_cast<audio::MusicRole>(r);
        idx.referencesS[r] = roleReferenceOnsetsS(src.applied, role, src.sampleRate);
        idx.referenceMatched[r].assign(idx.referencesS[r].size(), false);
        if (src.stems[r].empty()) {
            idx.status[r] = "NOT_RUN";
            continue;
        }
        OnsetParams p{};
        p.delta = src.onsetProfile[r].delta;
        const OnsetDetection det = detectOnsetsSeconds(monoFromStereo(src.stems[r]), p, src.sampleRate);
        if (det.status != AnalysisStatus::Ok) {
            idx.status[r] = analysisStatusName(det.status);
            continue;
        }
        std::vector<double> est = det.timesS;
        for (auto& t : est) t -= src.onsetProfile[r].offsetS;
        const OnsetScore sc = matchOnsetsMaxCardinality(idx.referencesS[r], est, toleranceS);
        if (sc.status != AnalysisStatus::Ok) {
            idx.status[r] = analysisStatusName(sc.status);
            continue;
        }
        for (const auto& pr : sc.pairs) idx.referenceMatched[r][pr.reference] = true; // references already sorted
        idx.status[r] = "MEASURED";
    }
    return idx;
}

WindowTruth measureWindow(const PackSource& src, const OnsetIndex& onsets, const WindowRender& w,
                          const PresenceProfile& profile, std::vector<float>* audioOut) {
    WindowTruth t;
    t.startFrame = w.startFrame;
    t.endFrame = w.endFrame;
    t.gainDb = w.gainDb;
    const std::size_t n = (w.endFrame > w.startFrame) ? static_cast<std::size_t>(w.endFrame - w.startFrame) : 0;
    bool inRange = n > 0;
    for (std::size_t r = 0; r < kRoleCount; ++r) inRange = inRange && src.stems[r].size() >= 2 * static_cast<std::size_t>(w.endFrame);
    if (!inRange) {
        if (audioOut != nullptr) audioOut->clear();
        return t; // every level Unavailable
    }

    const double gain = std::pow(10.0, w.gainDb / 20.0);
    const auto fade = static_cast<std::size_t>(std::llround(w.fadeS * src.sampleRate));
    auto envelope = [&](std::size_t k) {
        if (fade == 0) return 1.0;
        if (k < fade) return 0.5 * (1.0 - std::cos(kPi * (static_cast<double>(k) + 0.5) / static_cast<double>(fade)));
        if (k >= n - (std::min)(fade, n)) {
            const std::size_t j = n - 1 - k;
            return 0.5 * (1.0 - std::cos(kPi * (static_cast<double>(j) + 0.5) / static_cast<double>(fade)));
        }
        return 1.0;
    };

    std::vector<float> mix(2 * n, 0.0f);
    std::array<double, kRoleCount> sumSq{};
    std::array<double, kRoleCount> peak{};
    std::array<bool, kRoleCount> allZero{true, true, true, true};
    std::array<bool, kRoleCount> finite{true, true, true, true};
    for (std::size_t k = 0; k < n; ++k) {
        const double e = gain * envelope(k);
        for (std::size_t ch = 0; ch < 2; ++ch) {
            float m = 0.0f;
            for (std::size_t r = 0; r < kRoleCount; ++r) {
                if (!w.include[r]) continue;
                const float s = static_cast<float>(src.stems[r][2 * (static_cast<std::size_t>(w.startFrame) + k) + ch] * e);
                if (!std::isfinite(s)) {
                    finite[r] = false;
                    continue;
                }
                if (s != 0.0f) allZero[r] = false;
                sumSq[r] += static_cast<double>(s) * s;
                peak[r] = (std::max)(peak[r], static_cast<double>(std::abs(s)));
                m += s;
            }
            mix[2 * k + ch] = m;
        }
    }
    double mixSq = 0.0;
    for (const float v : mix) {
        mixSq += static_cast<double>(v) * v;
        if (std::abs(v) >= 1.0f) ++t.clippedSamples;
    }

    const std::vector<RoleSpan> applied = appliedSpans(src.applied, w.endFrame + 1);
    const double fs = static_cast<double>(src.sampleRate);
    const double startS = static_cast<double>(w.startFrame) / fs;
    const double endS = static_cast<double>(w.endFrame) / fs;
    for (std::size_t r = 0; r < kRoleCount; ++r) {
        RoleWindowTruth& rt = t.roles[r];
        const auto role = static_cast<audio::MusicRole>(r);
        rt.planned = std::any_of(src.planned.begin(), src.planned.end(),
                                 [&](const RoleSpan& s) { return s.role == role && overlaps(s, w.startFrame, w.endFrame); });
        rt.applied = std::any_of(applied.begin(), applied.end(),
                                 [&](const RoleSpan& s) { return s.role == role && overlaps(s, w.startFrame, w.endFrame); });
        if (!finite[r]) {
            rt.signal = SignalLevel::Unavailable;
        } else if (allZero[r]) {
            rt.signal = SignalLevel::ExactSilence;
        } else {
            rt.peakDbfs = 20.0 * std::log10(peak[r]);
            rt.rmsDbfs = dbFromPower(sumSq[r] / static_cast<double>(2 * n));
            rt.stemToMixDb = mixSq > 0.0 ? dbFromPower(sumSq[r] / mixSq) : std::numeric_limits<double>::quiet_NaN();
            if (rt.peakDbfs < profile.numericalFloorDbfs) {
                rt.signal = SignalLevel::BelowNumericalFloor;
            } else if (rt.rmsDbfs >= profile.absoluteRmsDbfs && std::isfinite(rt.stemToMixDb) &&
                       rt.stemToMixDb >= profile.stemToMixDb) {
                rt.signal = SignalLevel::Present;
            } else {
                rt.signal = SignalLevel::BelowThreshold;
            }
        }
        rt.onsetStatus = onsets.status[r].empty() ? "NOT_RUN" : onsets.status[r];
        const auto& refs = onsets.referencesS[r];
        for (std::size_t i = 0; i < refs.size(); ++i) {
            if (refs[i] < startS || refs[i] >= endS) continue;
            ++rt.appliedNoteOns;
            if (w.include[r] && i < onsets.referenceMatched[r].size() && onsets.referenceMatched[r][i]) ++rt.matchedOnsets;
        }
        rt.onsetDetected = rt.onsetStatus == "MEASURED" && rt.matchedOnsets > 0;
    }

    const SideMidResult sm = sideMid(mix);
    t.sideMidStatus = analysisStatusName(sm.status);
    t.sideMidDb = sm.db;
    t.sideIsZero = sm.sideIsZero;
    t.midIsZero = sm.midIsZero;
    if (audioOut != nullptr) *audioOut = std::move(mix);
    return t;
}

std::vector<std::uint8_t> encodeWavStereo16(std::span<const float> interleaved, std::uint32_t sampleRate) {
    const auto dataBytes = static_cast<std::uint32_t>(interleaved.size() * 2);
    std::vector<std::uint8_t> out;
    out.reserve(44 + dataBytes);
    auto u32 = [&](std::uint32_t v) {
        for (int i = 0; i < 4; ++i) out.push_back(static_cast<std::uint8_t>(v >> (8 * i)));
    };
    auto u16 = [&](std::uint16_t v) {
        out.push_back(static_cast<std::uint8_t>(v));
        out.push_back(static_cast<std::uint8_t>(v >> 8));
    };
    auto tag = [&](const char* s) { out.insert(out.end(), s, s + 4); };
    tag("RIFF");
    u32(36 + dataBytes);
    tag("WAVE");
    tag("fmt ");
    u32(16);
    u16(1);
    u16(2);
    u32(sampleRate);
    u32(sampleRate * 4);
    u16(4);
    u16(16);
    tag("data");
    u32(dataBytes);
    for (const float raw : interleaved) {
        const float s = std::isfinite(raw) ? std::clamp(raw, -1.0f, 1.0f) : 0.0f;
        u16(static_cast<std::uint16_t>(static_cast<std::int16_t>(std::round(s * 32767.0f))));
    }
    return out;
}

std::string packSeedCommitment(std::uint64_t seed) {
    return sha256Hex(std::string_view(std::string(kEvaluationPackVersion) + "|selection_seed|" + std::to_string(seed)));
}

std::string truthToJson(const TruthBundle& t) {
    json j;
    j["pack_version"] = t.packVersion;
    j["source"] = t.sourceName;
    j["pack_seed"] = t.packSeed;
    j["seed_commitment_sha256"] = t.seedSha256;
    j["default_seed"] = t.defaultSeed;
    j["presence_profile"] = {{"version", t.profile.version},
                             {"absolute_rms_dbfs", t.profile.absoluteRmsDbfs},
                             {"stem_to_mix_db", t.profile.stemToMixDb},
                             {"numerical_floor_dbfs", t.profile.numericalFloorDbfs}};
    j["config"] = {{"blind_window_s", t.config.blindWindowS},
                   {"fade_s", t.config.fadeS},
                   {"duplicate_gain_db", t.config.duplicateGainDb},
                   {"duplicate_shift_s", t.config.duplicateShiftS},
                   {"windows_per_mask_group", t.config.windowsPerMaskGroup},
                   {"stereo_margin_db", t.config.stereoMarginDb},
                   {"onset_tolerance_s", t.config.onsetToleranceS},
                   {"click_resolution_s", t.config.clickResolutionS}};
    j["onset_profile_sha256"] = t.onsetProfileSha256;
    j["blind"] = windowsJson(t.blind);
    j["diagnostic"] = windowsJson(t.diagnostic);
    j["phrases"] = windowsJson(t.phrases);
    j["altered_mask_novel"] = t.alteredMaskNovel;
    j["public_blind_files"] = t.publicBlindFiles;
    j["public_diagnostic_files"] = t.publicDiagnosticFiles;
    return dump(j);
}

std::optional<TruthBundle> truthFromJson(std::string_view text, std::string* error) {
    try {
        const json j = json::parse(text);
        TruthBundle t;
        t.packVersion = j.at("pack_version").get<std::string>();
        if (t.packVersion != kEvaluationPackVersion) throw std::runtime_error("unsupported pack version " + t.packVersion);
        t.sourceName = j.at("source").get<std::string>();
        t.packSeed = j.at("pack_seed").get<std::uint64_t>();
        t.seedSha256 = j.at("seed_commitment_sha256").get<std::string>();
        t.defaultSeed = j.at("default_seed").get<bool>();
        const json& p = j.at("presence_profile");
        t.profile.absoluteRmsDbfs = p.at("absolute_rms_dbfs").get<double>();
        t.profile.stemToMixDb = p.at("stem_to_mix_db").get<double>();
        t.profile.numericalFloorDbfs = p.at("numerical_floor_dbfs").get<double>();
        const json& c = j.at("config");
        t.config.blindWindowS = c.at("blind_window_s").get<double>();
        t.config.fadeS = c.at("fade_s").get<double>();
        t.config.duplicateGainDb = c.at("duplicate_gain_db").get<double>();
        t.config.duplicateShiftS = c.at("duplicate_shift_s").get<double>();
        t.config.windowsPerMaskGroup = c.at("windows_per_mask_group").get<std::size_t>();
        t.config.stereoMarginDb = c.at("stereo_margin_db").get<double>();
        t.config.onsetToleranceS = c.at("onset_tolerance_s").get<double>();
        t.config.clickResolutionS = c.at("click_resolution_s").get<double>();
        t.onsetProfileSha256 = j.at("onset_profile_sha256").get<std::string>();
        t.blind = windowsFrom(j.at("blind"));
        t.diagnostic = windowsFrom(j.at("diagnostic"));
        t.phrases = windowsFrom(j.at("phrases"));
        t.alteredMaskNovel = j.at("altered_mask_novel").get<bool>();
        t.publicBlindFiles = j.at("public_blind_files").get<std::map<std::string, std::string>>();
        t.publicDiagnosticFiles = j.at("public_diagnostic_files").get<std::map<std::string, std::string>>();
        return t;
    } catch (const std::exception& e) {
        if (error != nullptr) *error = e.what();
        return std::nullopt;
    }
}

PackBuild buildEvaluationPack(const PackSource& src, std::uint64_t packSeed, const PackConfig& cfg,
                              const PresenceProfile& profile) {
    PackBuild b;
    TruthBundle& truth = b.truth;
    truth.sourceName = src.name;
    truth.packSeed = packSeed;
    truth.seedSha256 = packSeedCommitment(packSeed);
    truth.defaultSeed = (packSeed == kDefaultPackSeed);
    truth.profile = profile;
    truth.config = cfg;
    truth.onsetProfileSha256 = src.onsetProfileSha256;

    const auto blindFrames = static_cast<std::uint64_t>(std::llround(cfg.blindWindowS * src.sampleRate));
    if (src.sampleRate == 0 || src.phrases.empty() || blindFrames == 0) b.errors.push_back("empty source or window");
    for (std::size_t r = 0; r < kRoleCount; ++r) {
        if (src.stems[r].empty() || src.stems[r].size() != src.stems[0].size()) b.errors.push_back("stems missing or of different length");
    }
    for (const auto& p : src.phrases) {
        if (p.endFrame - p.startFrame < blindFrames + static_cast<std::uint64_t>(std::llround(cfg.duplicateShiftS * src.sampleRate))) {
            b.errors.push_back("blind window (plus duplicate shift) longer than phrase " + std::to_string(p.index));
        }
        if (2 * p.endFrame > src.stems[0].size()) b.errors.push_back("phrase " + std::to_string(p.index) + " outside the stems");
    }
    if (!b.errors.empty()) return b;

    const OnsetIndex onsets = buildOnsetIndex(src, cfg.onsetToleranceS);
    SelectionRng rng(packSeed);

    // Blind excerpt of every phrase: planned mask (selection) and presence calibration evidence
    std::vector<std::uint32_t> mask(src.phrases.size(), 0);
    std::vector<WindowTruth> excerpt(src.phrases.size());
    for (std::size_t i = 0; i < src.phrases.size(); ++i) {
        const auto& p = src.phrases[i];
        WindowTruth t = measureWindow(src, onsets, {p.startFrame, p.startFrame + blindFrames, {true, true, true, true}, 0.0, cfg.fadeS}, profile);
        t.id = "phrase_" + std::to_string(p.index);
        t.kind = "calibration";
        t.phrase = static_cast<int>(p.index);
        t.section = p.section;
        t.startBar = p.startBar;
        t.barCount = p.barCount;
        for (std::size_t r = 0; r < kRoleCount; ++r) mask[i] |= (t.roles[r].planned ? 1U : 0U) << r;
        excerpt[i] = t;
    }
    truth.phrases = excerpt;
    b.phrasesMeasured = excerpt.size();

    // Test windows: up to k phrases per (section, planned mask), drawn by the selection stream
    std::map<std::pair<std::uint32_t, std::uint32_t>, std::vector<std::size_t>> groups;
    for (std::size_t i = 0; i < src.phrases.size(); ++i) groups[{src.phrases[i].section, mask[i]}].push_back(i);
    std::vector<std::size_t> tests;
    for (auto& [key, members] : groups) {
        shuffle(members, rng);
        for (std::size_t k = 0; k < members.size() && k < cfg.windowsPerMaskGroup; ++k) tests.push_back(members[k]);
    }
    std::sort(tests.begin(), tests.end());
    std::set<std::size_t> used(tests.begin(), tests.end());

    auto presentRoles = [&](std::size_t i) {
        std::vector<std::size_t> v;
        for (std::size_t r = 0; r < kRoleCount; ++r) {
            if (excerpt[i].roles[r].signal == SignalLevel::Present) v.push_back(r);
        }
        return v;
    };

    // Altered-truth item: a phrase with >= 2 roles present, one present role removed. Preferred (AG-5d-03): the
    // remaining role set equals the planned mask of some phrase, so the item is not recognisable by counting roles.
    // Phrases outside the tests first; a novel role set is recorded in the sealed truth.
    const std::set<std::uint32_t> plannedMasks(mask.begin(), mask.end());
    struct AlteredCandidate {
        std::size_t phrase;
        std::size_t role;
    };
    auto alteredCandidates = [&](bool unusedOnly, bool existingMaskOnly) {
        std::vector<AlteredCandidate> v;
        for (std::size_t i = 0; i < src.phrases.size(); ++i) {
            if (unusedOnly && used.contains(i)) continue;
            const auto roles = presentRoles(i);
            if (roles.size() < 2) continue;
            std::uint32_t present = 0;
            for (const auto r : roles) present |= 1U << r;
            for (const auto r : roles) {
                if (!existingMaskOnly || plannedMasks.contains(present & ~(1U << r))) v.push_back({i, r});
            }
        }
        return v;
    };
    std::vector<AlteredCandidate> alteredPool = alteredCandidates(true, true);
    if (alteredPool.empty()) alteredPool = alteredCandidates(false, true);
    truth.alteredMaskNovel = alteredPool.empty();
    if (alteredPool.empty()) alteredPool = alteredCandidates(true, false);
    if (alteredPool.empty()) alteredPool = alteredCandidates(false, false);
    std::optional<std::size_t> alteredPhrase;
    int removedRole = -1;
    if (!alteredPool.empty()) {
        const AlteredCandidate pick = alteredPool[rng.below(alteredPool.size())];
        alteredPhrase = pick.phrase;
        removedRole = static_cast<int>(pick.role);
        used.insert(pick.phrase);
    }
    const std::size_t duplicateSource = tests[rng.below(tests.size())];
    const auto duplicateShift = static_cast<std::uint64_t>(std::llround(cfg.duplicateShiftS * src.sampleRate));

    // Labelled references: solo stem of each role per section, from phrases that are not used as test items
    struct Ref {
        std::size_t role;
        std::uint32_t section;
        std::size_t phrase;
    };
    std::vector<Ref> refs;
    std::set<std::uint32_t> sections;
    for (const auto& p : src.phrases) sections.insert(p.section);
    for (std::size_t r = 0; r < kRoleCount; ++r) {
        for (const auto s : sections) {
            std::vector<std::size_t> cands;
            for (std::size_t i = 0; i < src.phrases.size(); ++i) {
                if (src.phrases[i].section == s && !used.contains(i) && excerpt[i].roles[r].signal == SignalLevel::Present) {
                    cands.push_back(i);
                }
            }
            if (!cands.empty()) refs.push_back({r, s, cands[rng.below(cands.size())]});
        }
    }

    // Opaque identifiers from the selection stream
    std::set<std::string> ids;
    auto newId = [&]() {
        for (;;) {
            std::string id = hexId(rng.next());
            if (ids.insert(id).second) return id;
        }
    };

    struct BlindItem {
        WindowRender render;
        WindowTruth meta;
    };
    std::vector<BlindItem> items;
    auto phraseMeta = [&](std::size_t i, const std::string& kind) {
        WindowTruth m;
        m.kind = kind;
        m.phrase = static_cast<int>(src.phrases[i].index);
        m.section = src.phrases[i].section;
        m.startBar = src.phrases[i].startBar;
        m.barCount = src.phrases[i].barCount;
        return m;
    };
    std::map<std::size_t, std::string> testIds;
    for (const auto i : tests) {
        BlindItem it{{src.phrases[i].startFrame, src.phrases[i].startFrame + blindFrames, {true, true, true, true}, 0.0, cfg.fadeS},
                     phraseMeta(i, "test")};
        it.meta.id = newId();
        testIds[i] = it.meta.id;
        items.push_back(it);
    }
    {
        const std::size_t i = duplicateSource;
        BlindItem it{{src.phrases[i].startFrame + duplicateShift, src.phrases[i].startFrame + duplicateShift + blindFrames,
                      {true, true, true, true}, cfg.duplicateGainDb, cfg.fadeS},
                     phraseMeta(i, "duplicate")};
        it.meta.id = newId();
        it.meta.duplicateOf = testIds[i];
        items.push_back(it);
    }
    if (alteredPhrase) {
        const std::size_t i = *alteredPhrase;
        BlindItem it{{src.phrases[i].startFrame, src.phrases[i].startFrame + blindFrames, {true, true, true, true}, 0.0, cfg.fadeS},
                     phraseMeta(i, "altered")};
        it.render.include[static_cast<std::size_t>(removedRole)] = false;
        it.meta.removedRole = removedRole;
        it.meta.id = newId();
        items.push_back(it);
    }
    {
        BlindItem it{{src.phrases[0].startFrame, src.phrases[0].startFrame + blindFrames, {false, false, false, false}, 0.0, cfg.fadeS},
                     WindowTruth{}};
        it.meta.kind = "catch";
        it.meta.phrase = -1;
        it.meta.id = newId();
        items.push_back(it);
    }

    // ---- public_blind ----
    json windowsPublic = json::array();
    for (const auto& it : items) {
        std::vector<float> audio;
        WindowTruth t = measureWindow(src, onsets, it.render, profile, &audio);
        t.id = it.meta.id;
        t.kind = it.meta.kind;
        t.duplicateOf = it.meta.duplicateOf;
        t.removedRole = it.meta.removedRole;
        t.phrase = it.meta.phrase;
        t.section = it.meta.section;
        t.startBar = it.meta.startBar;
        t.barCount = it.meta.barCount;
        if (t.kind == "catch") {
            for (auto& rt : t.roles) {
                rt.planned = false;  // synthetic silence: not an excerpt of the run
                rt.applied = false;
                rt.appliedNoteOns = 0;
            }
        }
        auto mid = melPng(channelMix(audio, false), src.sampleRate);
        auto side = melPng(channelMix(audio, true), src.sampleRate);
        t.melMidStatus = mid.first;
        t.melSideStatus = side.first;
        b.publicBlind[t.id + ".wav"] = encodeWavStereo16(audio, src.sampleRate);
        b.publicBlind[t.id + "_mid.png"] = std::move(mid.second);
        b.publicBlind[t.id + "_side.png"] = std::move(side.second);
        truth.blind.push_back(t);
    }
    std::sort(truth.blind.begin(), truth.blind.end(), [](const WindowTruth& a, const WindowTruth& c) { return a.id < c.id; });
    for (const auto& t : truth.blind) {
        windowsPublic.push_back({{"id", t.id}, {"audio", t.id + ".wav"}, {"mel_mid", t.id + "_mid.png"}, {"mel_side", t.id + "_side.png"}});
    }
    json refsPublic = json::array();
    for (const auto& ref : refs) {
        const auto& p = src.phrases[ref.phrase];
        WindowRender wr{p.startFrame, p.startFrame + blindFrames, {false, false, false, false}, 0.0, cfg.fadeS};
        wr.include[ref.role] = true;
        std::vector<float> audio;
        (void)measureWindow(src, onsets, wr, profile, &audio);
        const std::string name = std::string("ref_") + kRoleNames[ref.role] + "_s" + std::to_string(ref.section) + "_mid.png";
        b.publicBlind[name] = melPng(channelMix(audio, false), src.sampleRate).second;
        refsPublic.push_back({{"role", kRoleNames[ref.role]}, {"section_label", "s" + std::to_string(ref.section)}, {"mel_mid", name}});
    }
    b.references = refs.size();

    auto commonPack = [&](const char* mode) {
        json j;
        j["pack_version"] = kEvaluationPackVersion;
        j["mode"] = mode;
        j["result_schema"] = kEvaluationResultSchema;
        j["claims_version"] = kClaimsVersion;
        j["descriptors_version"] = "descriptors_v1";
        j["sample_rate"] = src.sampleRate;
        j["channels"] = 2;
        j["sample_format"] = "pcm_s16le";
        j["fade_s"] = cfg.fadeS;
        j["mel"] = {{"bands", kDescriptorsV1Mel.bands}, {"f_min_hz", kDescriptorsV1Mel.fMinHz}, {"f_max_hz", kDescriptorsV1Mel.fMaxHz},
                    {"db_range", kDescriptorsV1Mel.dbRange}, {"stft_size", kDescriptorsV1Stft.fftSize}, {"hop", kDescriptorsV1Stft.hop},
                    {"image", "column = STFT frame, row = mel band (low at the bottom), grey = normalised dB"}};
        j["roles"] = json(std::vector<std::string>(kRoleNames.begin(), kRoleNames.end()));
        j["methods"] = json(std::vector<std::string>(kMethods.begin(), kMethods.end()));
        j["predicates"] = json(std::vector<std::string>(kPredicates.begin(), kPredicates.end()));
        j["artifact_kinds"] = json(std::vector<std::string>(kArtifactKinds.begin(), kArtifactKinds.end()));
        j["undetermined"] = "claimed_value null = not determinable (counted apart, never a success)";
        return j;
    };
    {
        json j = commonPack("blind");
        j["window_duration_s"] = cfg.blindWindowS;
        j["window_frames"] = blindFrames;
        j["seed_commitment_sha256"] = truth.seedSha256;
        j["windows"] = windowsPublic;
        j["references"] = refsPublic;
        b.publicBlind["pack.json"] = bytesOf(dump(j));
    }
    truth.publicBlindFiles = hashMap(b.publicBlind);
    b.publicBlind["SHA256SUMS"] = bytesOf(sha256Sums(b.publicBlind));
    truth.publicBlindFiles["SHA256SUMS"] = sha256Of(b.publicBlind["SHA256SUMS"]);

    // ---- public_diagnostic: full phrases of the test windows, absolute timing, truth visible ----
    json windowsDiag = json::array();
    for (const auto i : tests) {
        const auto& p = src.phrases[i];
        std::vector<float> audio;
        WindowTruth t = measureWindow(src, onsets, {p.startFrame, p.endFrame, {true, true, true, true}, 0.0, cfg.fadeS}, profile, &audio);
        std::ostringstream id;
        id << "p" << std::setw(2) << std::setfill('0') << p.index;
        t.id = id.str();
        t.kind = "test";
        t.phrase = static_cast<int>(p.index);
        t.section = p.section;
        t.startBar = p.startBar;
        t.barCount = p.barCount;
        auto mid = melPng(channelMix(audio, false), src.sampleRate);
        auto side = melPng(channelMix(audio, true), src.sampleRate);
        t.melMidStatus = mid.first;
        t.melSideStatus = side.first;
        b.publicDiagnostic[t.id + ".wav"] = encodeWavStereo16(audio, src.sampleRate);
        b.publicDiagnostic[t.id + "_mid.png"] = std::move(mid.second);
        b.publicDiagnostic[t.id + "_side.png"] = std::move(side.second);
        json stems = json::object();
        for (std::size_t r = 0; r < kRoleCount; ++r) {
            WindowRender wr{p.startFrame, p.endFrame, {false, false, false, false}, 0.0, cfg.fadeS};
            wr.include[r] = true;
            std::vector<float> solo;
            (void)measureWindow(src, onsets, wr, profile, &solo);
            const std::string name = t.id + "_" + kRoleNames[r] + "_mid.png";
            b.publicDiagnostic[name] = melPng(channelMix(solo, false), src.sampleRate).second;
            stems[kRoleNames[r]] = name;
        }
        windowsDiag.push_back({{"id", t.id}, {"audio", t.id + ".wav"}, {"mel_mid", t.id + "_mid.png"}, {"mel_side", t.id + "_side.png"},
                               {"stem_mel_mid", stems}, {"phrase", p.index}, {"section", p.section}, {"start_bar", p.startBar},
                               {"bar_count", p.barCount}, {"start_frame", p.startFrame}, {"end_frame", p.endFrame}});
        truth.diagnostic.push_back(t);
    }
    {
        json j = commonPack("diagnostic");
        j["pack_seed"] = packSeed;
        j["source"] = src.name;
        j["onset_profile_sha256"] = src.onsetProfileSha256;
        j["presence_profile"] = {{"version", profile.version}, {"absolute_rms_dbfs", profile.absoluteRmsDbfs},
                                 {"stem_to_mix_db", profile.stemToMixDb}, {"numerical_floor_dbfs", profile.numericalFloorDbfs}};
        j["windows"] = windowsDiag;
        j["never_blind"] = true;
        b.publicDiagnostic["pack.json"] = bytesOf(dump(j));
        b.publicDiagnostic["truth.json"] = bytesOf(dump(windowsJson(truth.diagnostic)));
    }
    truth.publicDiagnosticFiles = hashMap(b.publicDiagnostic);
    b.publicDiagnostic["SHA256SUMS"] = bytesOf(sha256Sums(b.publicDiagnostic));
    truth.publicDiagnosticFiles["SHA256SUMS"] = sha256Of(b.publicDiagnostic["SHA256SUMS"]);

    // ---- sealed ----
    b.sealed["evaluation_truth.json"] = bytesOf(truthToJson(truth));
    b.sealed["SHA256SUMS"] = bytesOf(sha256Sums(b.sealed));
    b.ok = true;
    return b;
}

// ---- checker ----

ProportionCi wilsonInterval(std::size_t successes, std::size_t trials) noexcept {
    ProportionCi ci;
    ci.successes = successes;
    ci.trials = trials;
    if (trials == 0) return ci;
    constexpr double z = 1.96;
    const double n = static_cast<double>(trials);
    const double p = static_cast<double>(successes) / n;
    const double denom = 1.0 + z * z / n;
    const double centre = (p + z * z / (2.0 * n)) / denom;
    const double half = z * std::sqrt(p * (1.0 - p) / n + z * z / (4.0 * n * n)) / denom;
    ci.value = p;
    ci.low = (std::max)(0.0, centre - half);
    ci.high = (std::min)(1.0, centre + half);
    return ci;
}

CheckReport checkEvaluation(const TruthBundle& truth, std::string_view resultJson, const std::vector<std::string>& sealedFileHashes) {
    CheckReport rep;
    json res;
    try {
        res = json::parse(resultJson);
    } catch (const std::exception& e) {
        rep.error = std::string("result is not valid JSON: ") + e.what();
        return rep;
    }
    if (!res.is_object()) {
        rep.error = "result is not a JSON object";
        return rep;
    }
    rep.checked = true;
    auto inadmissible = [&](std::string why) { rep.inadmissibleReasons.push_back(std::move(why)); };
    auto str = [&](const json& o, const char* k) { return (o.is_object() && o.contains(k) && o[k].is_string()) ? o[k].get<std::string>() : std::string{}; };

    if (str(res, "schema") != kEvaluationResultSchema) inadmissible("schema is not evaluation_result_v0");
    if (str(res, "claims_version") != kClaimsVersion) inadmissible("claims_version is not claims_v0");
    rep.packMode = str(res, "pack_mode");
    const bool blindMode = rep.packMode == "blind";
    if (!blindMode && rep.packMode != "diagnostic") inadmissible("pack_mode is neither blind nor diagnostic");
    const auto& ownFiles = blindMode ? truth.publicBlindFiles : truth.publicDiagnosticFiles;
    std::set<std::string> ownHashes, otherHashes;
    for (const auto& [n, h] : ownFiles) ownHashes.insert(h);
    for (const auto& [n, h] : (blindMode ? truth.publicDiagnosticFiles : truth.publicBlindFiles)) otherHashes.insert(h);
    const std::set<std::string> sealedHashes(sealedFileHashes.begin(), sealedFileHashes.end());
    std::set<std::string> declared; // names of the own-mode files whose hash the result declares

    if (!res.contains("input_hashes") || !res["input_hashes"].is_array() || res["input_hashes"].empty()) {
        inadmissible("no input hashes declared");
    } else {
        for (const auto& h : res["input_hashes"]) {
            const std::string v = h.is_string() ? h.get<std::string>() : std::string{};
            if (sealedHashes.contains(v)) {
                inadmissible("input from sealed/: " + v);
            } else if (ownHashes.contains(v)) {
                for (const auto& [name, hash] : ownFiles) {
                    if (hash == v) declared.insert(name);
                }
            } else if (otherHashes.contains(v)) {
                inadmissible(blindMode ? "diagnostic file presented as blind: " + v : "file of the blind pack in a diagnostic result: " + v);
            } else {
                inadmissible("unknown input: " + v);
            }
        }
    }
    const json access = res.contains("access") ? res["access"] : json::object();
    auto accessFlag = [&](const char* k) -> std::optional<bool> {
        if (access.is_object() && access.contains(k) && access[k].is_boolean()) return access[k].get<bool>();
        return std::nullopt;
    };
    if (accessFlag("sealed").value_or(false)) inadmissible("evaluator declares access to sealed/");
    rep.resultAdmissible = rep.inadmissibleReasons.empty();

    // Blind qualification (D-M2-15): blind pack, admissible, no repository / sealed / diagnostic access declared
    auto notBlind = [&](std::string why) { rep.blindNotQualifiedReasons.push_back(std::move(why)); };
    if (!blindMode) notBlind("diagnostic pack: never blind");
    if (!rep.resultAdmissible) notBlind("result inadmissible");
    for (const char* k : {"repository", "sealed", "diagnostic_pack"}) {
        const auto f = accessFlag(k);
        if (!f.has_value()) notBlind(std::string("access.") + k + " not declared");
        else if (*f) notBlind(std::string("access to ") + k);
    }
    if (truth.defaultSeed) notBlind("pack built with the public default seed");
    rep.blindQualified = rep.blindNotQualifiedReasons.empty();

    const std::vector<WindowTruth>& windows = blindMode ? truth.blind : truth.diagnostic;
    std::map<std::string, const WindowTruth*> byId;
    for (const auto& w : windows) byId[w.id] = &w;
    const json channel = res.contains("channel") && res["channel"].is_object() ? res["channel"] : json::object();
    const int audioChannels = channel.contains("audio_channels") && channel["audio_channels"].is_number_integer()
                                  ? channel["audio_channels"].get<int>() : 0;
    const double tokensPerS = channel.contains("tokens_per_s") && channel["tokens_per_s"].is_number()
                                  ? channel["tokens_per_s"].get<double>() : 0.0;
    const bool refsUsed = res.contains("spectrogram_references_used") && res["spectrogram_references_used"].is_boolean() &&
                          res["spectrogram_references_used"].get<bool>();

    const json claims = res.contains("claims") && res["claims"].is_array() ? res["claims"] : json::array();
    // Values of boolean claims, for the controls: (window, predicate, args) -> value
    std::map<std::string, std::map<std::string, bool>> values;
    std::set<std::string> seenClaims;
    // Labelled spectrogram references of the mode: ref_* (blind), per-stem mel <id>_<role>_mid.png (diagnostic)
    const bool referenceDeclared = std::any_of(declared.begin(), declared.end(), [&](const std::string& n) {
        if (blindMode) return n.rfind("ref_", 0) == 0;
        return std::any_of(kRoleNames.begin(), kRoleNames.end(),
                           [&](const char* r) { return n.ends_with(std::string("_") + r + "_mid.png"); });
    });
    // The window input the method needs must be among the declared inputs (AG-5d-02)
    auto inputDeclared = [&](const std::string& id, const std::string& method) {
        if (method == "listening" || method == "descriptor") return declared.contains(id + ".wav");
        if (method == "spectrogram") return declared.contains(id + "_mid.png") || declared.contains(id + "_side.png");
        return true; // code: no audio input
    };
    std::size_t tp = 0, fn = 0, tn = 0, fp = 0;
    for (std::size_t ci = 0; ci < claims.size(); ++ci) {
        const json& c = claims[ci];
        ClaimOutcome o;
        o.index = ci;
        o.windowId = str(c, "window_id");
        o.method = str(c, "method");
        o.predicate = str(c, "predicate");
        auto finish = [&](const char* outcome, std::string reason) {
            o.outcome = outcome;
            o.reason = std::move(reason);
            ++rep.outcomeCounts[o.outcome];
            rep.claims.push_back(o);
        };
        if (!rep.resultAdmissible) {
            finish("inadmissible", "result inadmissible");
            continue;
        }
        const auto wit = byId.find(o.windowId);
        if (wit == byId.end()) { finish("inadmissible", "unknown window_id"); continue; }
        if (!contains(kMethods, o.method)) { finish("inadmissible", "unknown method"); continue; }
        if (!contains(kPredicates, o.predicate)) { finish("inadmissible", "unknown predicate"); continue; }
        const WindowTruth& w = *wit->second;
        const json args = c.contains("args") && c["args"].is_object() ? c["args"] : json::object();
        int role = -1;
        const WindowTruth* other = nullptr;
        std::string kind;
        std::string argKey;
        if (o.predicate.rfind("role_", 0) == 0) {
            role = roleIndex(str(args, "role"));
            if (role < 0) { finish("inadmissible", "missing or unknown role"); continue; }
            argKey = kRoleNames[static_cast<std::size_t>(role)];
        } else if (o.predicate == "stereo_wider_than") {
            const auto oit = byId.find(str(args, "window_id_other"));
            if (oit == byId.end() || oit->second == &w) { finish("inadmissible", "missing or invalid window_id_other"); continue; }
            other = oit->second;
            argKey = other->id;
        } else if (o.predicate == "artifact_present") {
            kind = str(args, "kind");
            if (!contains(kArtifactKinds, kind)) { finish("inadmissible", "missing or unknown artifact kind"); continue; }
            argKey = kind;
        }
        if (o.method == "spectrogram" && (!refsUsed || !referenceDeclared)) {
            finish("inadmissible", "spectrogram read without declared labelled references (D-M2-11)");
            continue;
        }
        if (!inputDeclared(w.id, o.method) || (other != nullptr && !inputDeclared(other->id, o.method))) {
            finish("inadmissible", "window input of the method not among the declared input hashes");
            continue;
        }
        if (o.predicate != "observation" && !seenClaims.insert(w.id + "|" + o.predicate + "|" + argKey).second) {
            finish("inadmissible", "repeated claim for the same window, predicate and arguments");
            continue;
        }
        if (o.predicate == "observation") { finish("unverifiable", "observation: no base of truth"); continue; }
        if (!c.contains("claimed_value") || c["claimed_value"].is_null()) { finish("abstained", "not determinable (counted apart)"); continue; }
        if (!c["claimed_value"].is_boolean()) { finish("inadmissible", "claimed_value is not boolean"); continue; }
        const bool v = c["claimed_value"].get<bool>();

        // Method cannot produce the claim
        if (o.predicate == "role_planned" && o.method != "code") { finish("not_attributable", "role_planned is not a perception: method code only"); continue; }
        if (o.predicate == "role_audible" && (o.method == "code" || o.method == "descriptor")) {
            finish("not_attributable", "audibility is perceptual: not from code or descriptors");
            continue;
        }
        if (o.predicate == "stereo_wider_than" && o.method == "listening" && audioChannels == 1) {
            finish("not_attributable", "stereo from a mono channel");
            continue;
        }
        if (o.predicate == "artifact_present" && kind == "click" && o.method == "listening" && tokensPerS > 0.0 &&
            1.0 / tokensPerS > truth.config.clickResolutionS) {
            finish("not_attributable", "click below the temporal resolution of the channel");
            continue;
        }
        values[w.id][o.predicate + "|" + argKey] = v;

        if (o.predicate == "role_signal_present") {
            const SignalLevel s = w.roles[static_cast<std::size_t>(role)].signal;
            if (s == SignalLevel::Unavailable) { finish("unverifiable", "signal measure unavailable"); continue; }
            const bool present = s == SignalLevel::Present;
            if (w.kind != "catch") {
                if (present && v) ++tp;
                if (present && !v) ++fn;
                if (!present && !v) ++tn;
                if (!present && v) ++fp;
            }
            finish(present == v ? "supported" : "contradicted", std::string("measured ") + signalLevelName(s));
        } else if (o.predicate == "role_onset_detected") {
            const auto& rt = w.roles[static_cast<std::size_t>(role)];
            if (rt.onsetStatus != "MEASURED") { finish("unverifiable", "onset detector status " + rt.onsetStatus); continue; }
            finish(rt.onsetDetected == v ? "supported" : "contradicted",
                   std::to_string(rt.matchedOnsets) + " detected attacks of " + std::to_string(rt.appliedNoteOns) + " applied" +
                       (w.removedRole == role ? " (role removed from the delivered audio)" : ""));
        } else if (o.predicate == "role_audible") {
            const SignalLevel s = w.roles[static_cast<std::size_t>(role)].signal;
            const std::string limit = "; independent perceptual verification absent";
            if (s == SignalLevel::Unavailable) finish("unverifiable", "signal measure unavailable" + limit);
            else if (v && s == SignalLevel::ExactSilence) finish("contradicted", "demonstrated absence (exact silence)" + limit);
            else if (v && s == SignalLevel::Present) finish("compatible", "signal present" + limit);
            else if (v) finish("unverifiable", std::string("weak signal: ") + signalLevelName(s) + limit);
            else if (s == SignalLevel::Present) finish("unverifiable", "signal present, possibly masked" + limit);
            else finish("compatible", std::string("measured ") + signalLevelName(s) + limit);
        } else if (o.predicate == "role_planned") {
            const bool planned = w.roles[static_cast<std::size_t>(role)].planned;
            finish(planned == v ? "supported" : "contradicted", planned ? "planned" : "not planned");
        } else if (o.predicate == "stereo_wider_than") {
            auto value = [](const WindowTruth& x) -> std::optional<double> {
                if (x.sideMidStatus != "OK") return std::nullopt;
                if (x.sideMidDb) return x.sideMidDb;
                if (x.sideIsZero) return -std::numeric_limits<double>::infinity();
                if (x.midIsZero) return std::numeric_limits<double>::infinity();
                return std::nullopt;
            };
            const auto a = value(w), c2 = value(*other);
            if (!a || !c2) { finish("unverifiable", "Side/Mid undefined"); continue; }
            if (std::isinf(*a) && std::isinf(*c2) && (*a > 0) == (*c2 > 0)) { finish("unverifiable", "Side/Mid equal"); continue; }
            const double d = *a - *c2;
            if (std::abs(d) < truth.config.stereoMarginDb) { finish("unverifiable", "Side/Mid difference below the margin"); continue; }
            finish((d > 0) == v ? "supported" : "contradicted", "Side/Mid difference measured");
        } else if (o.predicate == "artifact_present") {
            if (kind != "clipping") { finish("unverifiable", "no descriptor for " + kind); continue; }
            const bool clipped = w.clippedSamples > 0;
            finish(clipped == v ? "supported" : "contradicted", std::to_string(w.clippedSamples) + " clipped samples");
        }
    }

    // Controls
    for (const auto& w : windows) {
        if (w.kind == "catch") {
            for (const auto& [k, v] : values[w.id]) {
                if (v && (k.rfind("role_signal_present|", 0) == 0 || k.rfind("role_onset_detected|", 0) == 0 ||
                          k.rfind("role_audible|", 0) == 0)) {
                    rep.catchTrialFailed = true;
                }
            }
        }
        if (w.kind == "altered" && w.removedRole >= 0) {
            const std::string r = kRoleNames[static_cast<std::size_t>(w.removedRole)];
            for (const char* p : {"role_signal_present|", "role_onset_detected|", "role_audible|"}) {
                const auto it = values[w.id].find(p + r);
                if (it != values[w.id].end() && it->second) ++rep.alteredPlanFollowing;
            }
        }
        if (w.kind == "duplicate" && byId.contains(w.duplicateOf)) {
            for (const auto& [k, v] : values[w.id]) {
                const auto it = values[w.duplicateOf].find(k);
                if (it == values[w.duplicateOf].end()) continue;
                ++rep.duplicatePairsCompared;
                if (it->second == v) ++rep.duplicatePairsAgreeing;
            }
        }
    }
    rep.sensitivity = wilsonInterval(tp, tp + fn);
    rep.specificity = wilsonInterval(tn, tn + fp);
    if (rep.catchTrialFailed) {
        rep.presenceStatus = "EXCLUDED_BY_CATCH_RULE";
    } else if (tp + fn == 0 || tn + fp == 0) {
        rep.presenceStatus = "INSUFFICIENT_DATA";
    } else {
        rep.presenceStatus = "MEASURED";
        // Trivial classifiers have sensitivity + specificity = 1: exceeded only if both lower bounds sum above 1
        rep.aboveTrivialBaseline = rep.sensitivity.low + rep.specificity.low > 1.0;
    }
    return rep;
}

std::string CheckReport::toJson() const {
    json j;
    j["report"] = "evaluation_check_v0";
    j["gate"] = false;
    j["checked"] = checked;
    if (!error.empty()) j["error"] = error;
    j["pack_mode"] = packMode;
    j["result_admissible"] = resultAdmissible;
    j["inadmissible_reasons"] = inadmissibleReasons;
    j["blind_qualified"] = blindQualified;
    j["blind_not_qualified_reasons"] = blindNotQualifiedReasons;
    j["outcome_counts"] = outcomeCounts;
    json cl = json::array();
    for (const auto& c : claims) {
        cl.push_back({{"index", c.index}, {"window_id", c.windowId}, {"method", c.method}, {"predicate", c.predicate},
                      {"outcome", c.outcome}, {"reason", c.reason}});
    }
    j["claims"] = cl;
    auto ci = [](const ProportionCi& p) {
        return json{{"successes", p.successes}, {"trials", p.trials}, {"value", numOrNull(p.value)},
                    {"wilson95_low", numOrNull(p.low)}, {"wilson95_high", numOrNull(p.high)}};
    };
    j["controls"] = {{"catch_trial_failed", catchTrialFailed},
                     {"altered_plan_following_claims", alteredPlanFollowing},
                     {"duplicate_pairs_compared", duplicatePairsCompared},
                     {"duplicate_pairs_agreeing", duplicatePairsAgreeing}};
    // Rates only when MEASURED (AG-5d-05): counts are kept, values and intervals are null otherwise
    auto ciOrCounts = [&](const ProportionCi& p) {
        json c = ci(p);
        if (presenceStatus != "MEASURED") c["value"] = c["wilson95_low"] = c["wilson95_high"] = nullptr;
        return c;
    };
    j["role_signal_present_scoring"] = {{"status", presenceStatus}, {"sensitivity", ciOrCounts(sensitivity)},
                                        {"specificity", ciOrCounts(specificity)}, {"above_trivial_baseline", aboveTrivialBaseline}};
    j["note"] = "supplementary evidence: never a gate (D-M2-11); role_audible is at most compatible, never supported";
    return dump(j);
}

// ---- scenario ----

namespace {

bool writeFiles(const std::string& dir, const std::map<std::string, std::vector<std::uint8_t>>& files, std::string& error) {
    std::error_code ec;
    std::filesystem::remove_all(dir, ec);
    std::filesystem::create_directories(dir, ec);
    if (ec) {
        error = "cannot create " + dir;
        return false;
    }
    for (const auto& [name, bytes] : files) {
        std::ofstream out(std::filesystem::path(dir) / name, std::ios::binary);
        out.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
        if (!out.good()) {
            error = "cannot write " + name;
            return false;
        }
    }
    return true;
}

struct FixtureRender {
    std::vector<AppliedEventRecord> applied;
    std::vector<audio::MusicIntentEvent> intents;
    std::vector<audio::PlannedNoteOff> noteOffs;
    CaptureCheck check;
};

FixtureRender renderFixture(StemCapture& cap) {
    FixtureRender fr;
    cap.reset();
    std::uint64_t appliedOverflow = 0;
    audio::ProbeAnalysisCapture pc{};
    pc.stems = cap.sink();
    pc.applied = &fr.applied;
    pc.appliedOverflow = &appliedOverflow;
    pc.intents = &fr.intents;
    pc.noteOffs = &fr.noteOffs;
    const auto probe = audio::runFixtureProbeV1Offline("artifacts/cacp", false, audio::FixtureInjectedFault::None, &pc);
    fr.check.rendered = true;
    fr.check.probePassed = probe.passed;
    fr.check.probeFailure = probe.failureReason;
    fr.check.expectedFrames = static_cast<std::size_t>(probe.requestedSampleFrames);
    fr.check.stemFrames = cap.frames();
    fr.check.stemOverflowFrames = cap.overflowFrames();
    fr.check.appliedOverflow = appliedOverflow;
    return fr;
}

PackSource fixtureSource(const StemCapture& cap, const FixtureRender& fr) {
    PackSource src;
    src.name = "fixture_probe_v1 seed 1337";
    for (std::size_t r = 0; r < kRoleCount; ++r) src.stems[r] = cap.stem(r);
    src.applied = fr.applied;
    src.planned = plannedSpans(fr.intents, fr.noteOffs);
    if (const auto clock = audio::MusicPlanner::makeFixtureM24V1Clock()) {
        for (const auto& p : audio::MusicPlanner::makeFixtureM24V1Phrases(*clock)) {
            src.phrases.push_back({static_cast<std::uint32_t>(src.phrases.size()), p.sectionIndex, p.startBar, p.barCount,
                                   p.startFrame, p.endFrame});
        }
    }
    if (const auto frozen = frozenStemProfileV1()) {
        src.onsetProfile = frozen->roles;
        src.onsetProfileSha256 = stemProfileSha256(frozen->roles);
    }
    return src;
}

constexpr std::size_t kFixtureCaptureFrames = 8'900'000;

} // namespace

EvaluationPackRun runEvaluationPackV1(const std::string& publicRoot, const std::string& sealedDir, std::uint64_t packSeed) {
    EvaluationPackRun run;
    run.packSeed = packSeed;
    run.defaultSeed = (packSeed == kDefaultPackSeed);
    run.publicBlindDir = (std::filesystem::path(publicRoot) / "public_blind").lexically_normal().string();
    run.publicDiagnosticDir = (std::filesystem::path(publicRoot) / "public_diagnostic").lexically_normal().string();
    run.sealedDir = std::filesystem::path(sealedDir).lexically_normal().string();

    StemCapture cap(kFixtureCaptureFrames);
    const FixtureRender first = renderFixture(cap);
    run.captureInvalid = captureInvalidReason(first.check);
    if (!run.captureInvalid.empty()) run.errors.push_back("invalid capture: " + run.captureInvalid);
    PackBuild build;
    if (run.errors.empty()) {
        build = buildEvaluationPack(fixtureSource(cap, first), packSeed);
        for (const auto& e : build.errors) run.errors.push_back(e);
    }
    if (run.errors.empty()) {
        run.blindWindows = build.truth.blind.size();
        run.diagnosticWindows = build.truth.diagnostic.size();
        run.references = build.references;
        run.publicBlindFiles = build.publicBlind.size();
        run.publicDiagnosticFiles = build.publicDiagnostic.size();
        std::string err;
        if (!writeFiles(run.publicBlindDir, build.publicBlind, err) || !writeFiles(run.publicDiagnosticDir, build.publicDiagnostic, err) ||
            !writeFiles(run.sealedDir, build.sealed, err)) {
            run.errors.push_back(err);
        }
    }
    if (run.errors.empty()) {
        // Second independent render and build: every file must be byte-identical (D-M2-15 verification)
        const FixtureRender second = renderFixture(cap);
        if (!captureInvalidReason(second.check).empty()) {
            run.errors.push_back("invalid capture on the repeat render");
        } else {
            const PackBuild again = buildEvaluationPack(fixtureSource(cap, second), packSeed);
            run.repeatIdentical = again.ok && again.publicBlind == build.publicBlind &&
                                  again.publicDiagnostic == build.publicDiagnostic && again.sealed == build.sealed;
            if (!run.repeatIdentical) run.errors.push_back("repeat build differs");
        }
    }
    run.ok = run.errors.empty();
    run.status = run.ok ? "GENERATED" : "FAILED";

    const std::filesystem::path root(publicRoot);
    run.manifestFilePath = (root.parent_path() / (root.filename().string() + "_manifest.json")).lexically_normal().string();
    std::error_code ec;
    std::filesystem::create_directories(root.parent_path(), ec);
    std::ofstream mf(run.manifestFilePath);
    run.manifestWriteSuccess = mf.is_open();
    if (run.manifestWriteSuccess) {
        mf << run.toJson();
        run.manifestWriteSuccess = mf.good();
    }
    return run;
}

std::string EvaluationPackRun::toJson() const {
    json j;
    j["manifest_version"] = "1.0";
    j["scenario"] = kEvaluationPackVersion;
    j["build_id"] = CYMATICA_BUILD_ID;
    j["verdict"] = status;
    j["gate"] = false;
    j["errors"] = errors;
    j["seed_commitment_sha256"] = packSeedCommitment(packSeed);
    j["default_seed"] = defaultSeed;
    j["blind_windows"] = blindWindows;
    j["diagnostic_windows"] = diagnosticWindows;
    j["references"] = references;
    j["public_blind_files"] = publicBlindFiles;
    j["public_diagnostic_files"] = publicDiagnosticFiles;
    j["repeat_identical"] = repeatIdentical;
    j["public_blind_dir"] = publicBlindDir;
    j["public_diagnostic_dir"] = publicDiagnosticDir;
    j["sealed_dir"] = sealedDir;
    j["note"] = "hand only public_blind/ to a blind evaluator; sealed/ is for the checker (D-M2-15)";
    return dump(j);
}

CheckReport runEvaluationCheck(const std::string& resultPath, const std::string& sealedDir, const std::string& reportPath) {
    CheckReport rep;
    auto readAll = [](const std::filesystem::path& p) -> std::optional<std::string> {
        std::ifstream in(p, std::ios::binary);
        if (!in.is_open()) return std::nullopt;
        std::ostringstream s;
        s << in.rdbuf();
        return s.str();
    };
    const std::filesystem::path sealed(sealedDir);
    const auto sums = readAll(sealed / "SHA256SUMS");
    const auto truthText = readAll(sealed / "evaluation_truth.json");
    const auto resultText = readAll(resultPath);
    if (!sums || !truthText || !resultText) {
        rep.error = "cannot read the sealed truth, its SHA256SUMS or the result";
    } else {
        std::vector<std::string> sealedHashes{sha256Hex(std::string_view(*sums))};
        std::string expectedTruth;
        std::istringstream lines(*sums);
        std::string line;
        while (std::getline(lines, line)) {
            if (!line.empty() && line.back() == '\r') line.pop_back();
            if (line.size() < 66) continue;
            sealedHashes.push_back(line.substr(0, 64));
            if (line.substr(66) == "evaluation_truth.json") expectedTruth = line.substr(0, 64);
        }
        std::string err;
        if (expectedTruth.empty() || sha256Hex(std::string_view(*truthText)) != expectedTruth) {
            rep.error = "sealed truth does not match sealed/SHA256SUMS";
        } else if (const auto truth = truthFromJson(*truthText, &err)) {
            rep = checkEvaluation(*truth, *resultText, sealedHashes);
        } else {
            rep.error = "sealed truth unreadable: " + err;
        }
    }
    if (!reportPath.empty()) {
        std::ofstream out(reportPath);
        out << rep.toJson();
        if (!out.good()) {
            rep.checked = false;
            rep.error = "cannot write the report " + reportPath;
        }
    }
    return rep;
}

} // namespace cymatica::cacp
