// Copyright (c) 2026, mikosu contributors, All rights reserved.
//
// Headless replay runner: the console command
//
//     replay_run <beatmap.osu>|<replay.osr>[|<profile>[|<output.json>]]
//
// loads a beatmap and a replay, plays the replay through the gameplay code (SimulatedBeatmapInterface, nothing
// drawn), and reports the judgements, max combo and score it produced next to the values stored in the .osr, plus
// star rating and pp for both, as one JSON line on stdout (prefixed "REPLAYRUN ") and optionally into a file.
// Arguments are separated by '|' because beatmap paths usually contain spaces.
//
// Meant for headless runs (tools/runner/replay_runner.py drives it): this is the tool the scoring/judgement parity
// work (workstream C) and pp parity checks run through. Profiles: "stable" (the current judgement code); "lazer" and
// "lazer-classic" come with workstream C and are rejected until then.

#include "ConVar.h"
#include "DatabaseBeatmap.h"
#include "DiffCalc/DifficultyCalculator.h"
#include "LegacyReplay.h"
#include "Logging.h"
#include "SimulatedBeatmapInterface.h"
#include "Tools/DiffCalcToolShared.h"
#include "score.h"

#include <cstdio>
#include <limits>
#include <string>
#include <string_view>
#include <vector>

namespace {


std::string jsonString(std::string_view s) {
    std::string out{"\""};
    for(const unsigned char c : s) {
        switch(c) {
            case '"': out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default:
                if(c < 0x20) {
                    out += fmt::format("\\u{:04x}", c);
                } else {
                    out += static_cast<char>(c);
                }
        }
    }
    return out + "\"";
}

std::vector<std::string_view> splitArgs(std::string_view args) {
    std::vector<std::string_view> parts;
    while(true) {
        const size_t sep = args.find('|');
        std::string_view part = args.substr(0, sep);
        while(!part.empty() && (part.front() == ' ')) part.remove_prefix(1);
        while(!part.empty() && (part.back() == ' ' || part.back() == '\n' || part.back() == '\r')) part.remove_suffix(1);
        parts.push_back(part);
        if(sep == std::string_view::npos) break;
        args.remove_prefix(sep + 1);
    }
    return parts;
}

void emit(const std::string &json, std::string_view outPath) {
    // one line on stdout (the headless log goes there too, so it carries a marker to grep for)
    std::printf("REPLAYRUN %s\n", json.c_str());
    std::fflush(stdout);
    if(!outPath.empty()) {
        if(FILE *f = std::fopen(std::string{outPath}.c_str(), "wb")) {
            std::fwrite(json.data(), 1, json.size(), f);
            std::fputc('\n', f);
            std::fclose(f);
        }
    }
}

std::string countsJson(int n300, int n100, int n50, int geki, int katu, int miss) {
    return fmt::format(R"({{"300":{},"100":{},"50":{},"geki":{},"katu":{},"miss":{}}})", n300, n100, n50, geki, katu,
                       miss);
}

void replayRun(std::string_view args) {
    const auto parts = splitArgs(args);
    const std::string_view mapPath = parts.size() > 0 ? parts[0] : std::string_view{};
    const std::string_view osrPath = parts.size() > 1 ? parts[1] : std::string_view{};
    const std::string_view profile = parts.size() > 2 && !parts[2].empty() ? parts[2] : std::string_view{"stable"};
    const std::string_view outPath = parts.size() > 3 ? parts[3] : std::string_view{};

    const std::string ident =
        fmt::format(R"("schema":1,"tool":"replay_run","profile":{},"algo":{},"beatmap_path":{},"replay_path":{})",
                    jsonString(profile), neomod::DiffCalc::PP_ALGORITHM_VERSION, jsonString(mapPath),
                    jsonString(osrPath));
    const auto fail = [&](std::string_view why) { emit(fmt::format(R"({{{},"error":{}}})", ident, jsonString(why)), outPath); };

    if(mapPath.empty() || osrPath.empty()) return fail("usage: replay_run <beatmap.osu>|<replay.osr>[|<profile>[|<out.json>]]");
    if(profile != "stable") return fail("profile not implemented yet (only \"stable\"; lazer profiles arrive with workstream C)");

    FinishedScore score;
    if(!LegacyReplay::load_osr(osrPath, score)) return fail("could not read the replay (missing, unreadable or no frames)");

    const std::string mapPathStr{mapPath};
    const size_t slash = mapPathStr.find_last_of("/\\");
    const std::string folder = slash == std::string::npos ? std::string{"./"} : mapPathStr.substr(0, slash + 1);
    auto map = std::make_unique<DatabaseBeatmap>(mapPathStr, folder, DatabaseBeatmap::BeatmapType::NEOMOD_DIFFICULTY);
    if(const auto meta = map->loadMetadata(); meta.error.errc) {
        return fail(fmt::format("could not load the beatmap: {}", meta.error.error_string()));
    }

    // play the replay through the gameplay code. stable records until the ranking screen, but if a recording stops
    // early, no-input frames every 16 ms until past the end of the map get the remaining objects judged (and
    // simulate_to never plays the last two frames)
    SimulatedBeatmapInterface sim(map.get(), score.mods);
    sim.spectated_replay = score.replay;
    if(!sim.spectated_replay.empty()) {
        auto tail = sim.spectated_replay.back();
        const i32 last = tail.cur_music_pos;
        const i32 end = std::max<i32>(last, static_cast<i32>(map->getLengthMS())) + 1000;
        for(i32 t = last + 16; t <= end + 32; t += 16) {
            tail.milliseconds_since_last_frame = 16;
            tail.cur_music_pos = t;
            tail.key_flags = 0;
            sim.spectated_replay.push_back(tail);
        }
    }
    sim.simulate_to(std::numeric_limits<i32>::max());
    const LiveScore &ls = sim.live_score;

    // star rating and pp for the mods and rate of the replay
    const auto calc = neomod::DiffCalcTool::computeOneMap(mapPath, score.mods.flags, score.mods.speed);
    if(calc.failed()) return fail(fmt::format("star rating calculation failed: {}", calc.error));
    const auto ppFor = [&](int combo, int miss, int n300, int n100, int n50, u64 total) {
        neomod::DiffCalc::PPv2CalcParams p = calc.ssParams;
        p.combo = combo;
        p.misses = miss;
        p.c300 = n300;
        p.c100 = n100;
        p.c50 = n50;
        p.legacyTotalScore = static_cast<u32>(std::min<u64>(total, std::numeric_limits<u32>::max()));
        p.isMcOsuImported = false;
        return neomod::DiffCalc::calculatePPv2(p);
    };

    const int sim300 = ls.getNum300s(), sim100 = ls.getNum100s(), sim50 = ls.getNum50s(), simMiss = ls.getNumMisses();
    const int simCombo = ls.getComboMax();
    const u64 simScore = ls.getScore();
    const bool countsMatch = sim300 == score.num300s && sim100 == score.num100s && sim50 == score.num50s &&
                             simMiss == score.numMisses;

    const std::string json = fmt::format(
        R"({{{},"beatmap_md5":{},"md5_matches":{},"player":{},"mods":{},"speed":{},)"
        R"("replay":{{"counts":{},"max_combo":{},"score":{},"perfect":{}}},)"
        R"("simulated":{{"counts":{},"max_combo":{},"score":{},"slider_breaks":{},"accuracy":{:.6f}}},)"
        R"("match":{{"counts":{},"max_combo":{},"score":{}}},)"
        R"("objects":{},"stars":{},"max_combo":{},"pp":{{"replay":{:.6f},"simulated":{:.6f},"ss":{:.6f}}}}})",
        ident, jsonString(map->getMD5().to_chars().string()), map->getMD5() == score.beatmap_hash ? "true" : "false",
        jsonString(score.playerName),
        jsonString(neomod::DiffCalcTool::modsStringFromMods(score.mods.flags, score.mods.speed)), score.mods.speed,
        countsJson(score.num300s, score.num100s, score.num50s, score.numGekis, score.numKatus, score.numMisses),
        score.comboMax, score.score, score.perfect ? "true" : "false",
        countsJson(sim300, sim100, sim50, ls.getNum300gs(), ls.getNum100ks(), simMiss), simCombo, simScore,
        ls.getNumSliderBreaks(), ls.getAccuracy(), countsMatch ? "true" : "false",
        simCombo == score.comboMax ? "true" : "false", simScore == score.score ? "true" : "false",
        map->getNumObjects(), calc.totalStars,
        calc.maxCombo,
        ppFor(score.comboMax, score.numMisses, score.num300s, score.num100s, score.num50s, score.score),
        ppFor(simCombo, simMiss, sim300, sim100, sim50, simScore), calc.ppSS);
    emit(json, outPath);
}

}  // namespace

namespace cv {
static ConVar replay_run_cmd("replay_run", CLIENT | NOLOAD | NOSAVE,
                             "simulate a replay on a beatmap and print judgements, combo, score, stars and pp as JSON: "
                             "replay_run <beatmap.osu>|<replay.osr>[|<profile>[|<out.json>]]",
                             [](std::string_view args) -> void { replayRun(args); });
}  // namespace cv
