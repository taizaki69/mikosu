#pragma once
// Copyright (c) 2026, mikosu contributors, All rights reserved.
//
// Frame timing recorder for performance baselines and regression checks (docs/renovation/BASELINE.md).
// Inert unless the game is launched with "-benchout <file>": then every frame's CPU phase times, the GPU
// frame time (OpenGL timer queries) and input-to-present latency are recorded, split into labelled
// segments ("bench_segment <label>" from the console or a stdin script), and summarized as JSON to <file>
// at shutdown.

#include "types.h"

namespace FrameStats {

// reads the launch arguments; call once, early in main()
void init();
[[nodiscard]] bool enabled();

// an input event (key, mouse button/motion/wheel) became visible to the app; ts is in Timing::getTicksNS() time
void onInputEvent(u64 eventTimestampNS);

// frame phases, called from the main loop in order
void frameBegin();
void updateDone();
void paintDone(bool inGameplay);  // after Graphics::endScene (the frame was handed to the driver)

// GPU frame time in ms, reported by the renderer a few frames late (when its timer query resolves)
void reportGpuFrameTime(f64 ms);

// writes the summary (if enabled); safe to call more than once
void shutdown();

}  // namespace FrameStats
