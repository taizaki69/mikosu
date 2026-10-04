// Copyright (c) 2026, mikosu contributors, All rights reserved.
#include "FrameStats.h"

#include "ConVar.h"
#include "LaunchArgs.h"
#include "Logging.h"
#include "Timing.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

#ifdef _WIN32
#include "WinDebloatDefs.h"
#include <windows.h>
#include <psapi.h>
#endif

namespace FrameStats {
namespace {

struct Segment {
    std::string label;
    u64 startNS{0};
    u64 endNS{0};
    u64 gameplayFrames{0};
    std::vector<f32> updateMS;    // App update (input handling, gameplay, UI)
    std::vector<f32> paintMS;     // draw + endScene (swap/submit)
    std::vector<f32> cpuMS;       // update + paint: the CPU work of one frame
    std::vector<f32> intervalMS;  // frame start to next frame start (includes the limiter's sleep)
    std::vector<f32> gpuMS;       // GPU time per frame (OpenGL only)
    std::vector<f32> latencyMS;   // input event timestamp -> the frame that handled it was handed to the driver
    f64 rssEndMB{0.};
};

bool s_enabled{false};
bool s_dumped{false};
std::string s_outPath;

u64 s_initNS{0};
u64 s_firstFrameDoneNS{0};
u64 s_frameBeginNS{0};
u64 s_updateDoneNS{0};
u64 s_lastFrameBeginNS{0};

std::vector<u64> s_pendingInput;   // arrived since the last frame began
std::vector<u64> s_inflightInput;  // being handled by the current frame

std::vector<Segment> s_segments;

Segment &current() { return s_segments.back(); }

void startSegment(std::string label) {
    const u64 now = Timing::getTicksNS();
    if(!s_segments.empty()) {
        current().endNS = now;
    }
    Segment &seg = s_segments.emplace_back();
    seg.label = std::move(label);
    seg.startNS = now;
    // keep the hot loop free of reallocations for typical runs (~2 minutes at 1000+ fps)
    for(auto *v : {&seg.updateMS, &seg.paintMS, &seg.cpuMS, &seg.intervalMS, &seg.gpuMS}) {
        v->reserve(1 << 17);
    }
    seg.latencyMS.reserve(1 << 12);
}

void onSegmentCommand(std::string_view args) {
    if(!s_enabled) return;
    std::string label{args};
    while(!label.empty() && (label.back() == ' ' || label.back() == '\n' || label.back() == '\r')) label.pop_back();
    if(label.empty()) label = "segment" + std::to_string(s_segments.size());
    debugLog("FrameStats: segment \"{}\"", label);
    startSegment(std::move(label));
}

// resident set size now and at peak, in MiB
void readMemory(f64 &rssMB, f64 &peakMB) {
    rssMB = peakMB = 0.;
#if defined(_WIN32)
    PROCESS_MEMORY_COUNTERS pmc{};
    if(GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc))) {
        rssMB = (f64)pmc.WorkingSetSize / (1024. * 1024.);
        peakMB = (f64)pmc.PeakWorkingSetSize / (1024. * 1024.);
    }
#elif defined(__linux__)
    if(FILE *f = std::fopen("/proc/self/status", "r")) {
        char line[256];
        while(std::fgets(line, sizeof(line), f)) {
            unsigned long kb = 0;
            if(std::sscanf(line, "VmRSS: %lu kB", &kb) == 1) rssMB = (f64)kb / 1024.;
            if(std::sscanf(line, "VmHWM: %lu kB", &kb) == 1) peakMB = (f64)kb / 1024.;
        }
        std::fclose(f);
    }
#endif
}

// "name":{"n":..,"mean":..,"p50":..,"p90":..,"p99":..,"max":..}
std::string summarize(const char *name, std::vector<f32> v) {
    if(v.empty()) return fmt::format(R"("{}":null)", name);
    const auto pct = [&v](f64 p) -> f64 {
        const size_t k = std::min(v.size() - 1, (size_t)std::floor(p * (f64)(v.size() - 1) + 0.5));
        std::nth_element(v.begin(), v.begin() + (ptrdiff_t)k, v.end());
        return v[k];
    };
    f64 sum = 0.;
    for(f32 x : v) sum += x;
    const f64 p50 = pct(0.50), p90 = pct(0.90), p99 = pct(0.99), mx = *std::ranges::max_element(v);
    return fmt::format(R"("{}":{{"n":{},"mean":{:.4f},"p50":{:.4f},"p90":{:.4f},"p99":{:.4f},"max":{:.4f}}})", name,
                       v.size(), sum / (f64)v.size(), p50, p90, p99, mx);
}

std::string jsonEscape(std::string_view s) {
    std::string out;
    for(char c : s) {
        if(c == '"' || c == '\\') out.push_back('\\');
        if((unsigned char)c < 0x20) continue;
        out.push_back(c);
    }
    return out;
}

}  // namespace

void init() {
    const auto out = Mc::LaunchArgs::has_arg(Mc::LaunchArgs::MISC_BENCH_OUT);
    if(!out || out->empty()) return;
    s_enabled = true;
    s_outPath = *out;
    s_initNS = Timing::getTicksNS();
    startSegment("startup");
}

bool enabled() { return s_enabled; }

void onInputEvent(u64 eventTimestampNS) {
    if(!s_enabled) return;
    s_pendingInput.push_back(eventTimestampNS);
}

void frameBegin() {
    if(!s_enabled) return;
    const u64 now = Timing::getTicksNS();
    if(s_lastFrameBeginNS != 0) {
        current().intervalMS.push_back((f32)((f64)(now - s_lastFrameBeginNS) / 1e6));
    }
    s_lastFrameBeginNS = s_frameBeginNS = now;
    s_inflightInput.swap(s_pendingInput);
    s_pendingInput.clear();
}

void updateDone() {
    if(!s_enabled) return;
    s_updateDoneNS = Timing::getTicksNS();
}

void paintDone(bool inGameplay) {
    if(!s_enabled) return;
    const u64 now = Timing::getTicksNS();
    Segment &seg = current();
    seg.updateMS.push_back((f32)((f64)(s_updateDoneNS - s_frameBeginNS) / 1e6));
    seg.paintMS.push_back((f32)((f64)(now - s_updateDoneNS) / 1e6));
    seg.cpuMS.push_back((f32)((f64)(now - s_frameBeginNS) / 1e6));
    if(inGameplay) seg.gameplayFrames++;
    for(u64 ts : s_inflightInput) {
        if(ts != 0 && ts <= now) seg.latencyMS.push_back((f32)((f64)(now - ts) / 1e6));
    }
    s_inflightInput.clear();
    if(s_firstFrameDoneNS == 0) s_firstFrameDoneNS = now;
}

void reportGpuFrameTime(f64 ms) {
    if(!s_enabled || s_segments.empty()) return;
    current().gpuMS.push_back((f32)ms);
}

void shutdown() {
    if(!s_enabled || s_dumped) return;
    s_dumped = true;
    const u64 now = Timing::getTicksNS();
    current().endNS = now;

    f64 rssMB, peakMB;
    readMemory(rssMB, peakMB);
    current().rssEndMB = rssMB;

    std::string json = fmt::format(
        R"({{"schema":1,"tool":"FrameStats","startup":{{"first_frame_ms":{:.3f}}},"memory":{{"rss_mb":{:.1f},"peak_rss_mb":{:.1f}}},"segments":[)",
        s_firstFrameDoneNS ? (f64)(s_firstFrameDoneNS - s_initNS) / 1e6 : -1., rssMB, peakMB);
    bool first = true;
    for(auto &seg : s_segments) {
        if(!first) json += ',';
        first = false;
        json += fmt::format(R"({{"label":"{}","start_s":{:.3f},"duration_s":{:.3f},"frames":{},"gameplay_frames":{},)",
                            jsonEscape(seg.label), (f64)(seg.startNS - s_initNS) / 1e9,
                            (f64)(seg.endNS - seg.startNS) / 1e9, seg.cpuMS.size(), seg.gameplayFrames);
        json += summarize("cpu_update_ms", seg.updateMS) + ',';
        json += summarize("cpu_paint_ms", seg.paintMS) + ',';
        json += summarize("cpu_frame_ms", seg.cpuMS) + ',';
        json += summarize("frame_interval_ms", seg.intervalMS) + ',';
        json += summarize("gpu_frame_ms", seg.gpuMS) + ',';
        json += summarize("input_latency_ms", seg.latencyMS);
        json += '}';
    }
    json += "]}\n";

    if(FILE *f = std::fopen(s_outPath.c_str(), "wb")) {
        std::fwrite(json.data(), 1, json.size(), f);
        std::fclose(f);
        debugLog("FrameStats: wrote {}", s_outPath);
    } else {
        debugLog("FrameStats: could not write {}", s_outPath);
    }
}

}  // namespace FrameStats

namespace cv {
static ConVar bench_segment_cmd("bench_segment", CLIENT | NOLOAD | NOSAVE,
                                "start a new labelled frame timing segment (only with -benchout)",
                                [](std::string_view args) -> void { FrameStats::onSegmentCommand(args); });
}  // namespace cv
