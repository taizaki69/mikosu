# Performance baseline

Ground rule 4: measure before changing anything. These numbers are the bar every later change is held to.

- **Source:** neomod at upstream `0fedcafb` plus the measurement-only `FrameStats` recorder (inert without `-benchout`).
- **Raw data:** `bench/baseline-neomod-0fedcafb-{gl,sdlgpu}.json`.

## How it was measured

- **Machine:** the reference PC: i5-13400F, RTX 4070 SUPER (driver 610.57.04), 32 GB RAM, Linux Mint 22.3, kernel 7.0.0-38, X11.
- **Build:** the upstream release config (`tools/build.sh linux`): gcc-14, LTO, `-O3`, hardened, `--with-audio=bass,soloud --with-renderer=opengl,sdlgpu`.
- **Run mode:** headless, with offscreen rendering **on the real GPU** (NVIDIA EGL for OpenGL; Vulkan for SDL_gpu).
  - 2560×1440, which is the user's monitor resolution.
  - Frame rate uncapped (the limiter is off in headless mode); dummy audio.
  - Runs under `tools/dev/guarded --bench`: memory caps, normal priority.
- **Tool:** `python3 tools/bench/bench.py --game build/dist/bin-x86_64/mikosu --renderer gl|sdlgpu --runs 3 --library ~/.local/share/osu-stable`.
  - Every number is the median over 3 runs.
  - Summaries: `tools/bench/report.py <json>`.
- **Scenes:**
  - *Gameplay, dense map:* a generated reference map (`bench.py` writes it: 559 objects; 220 BPM 1/4 streams, then slider spam, then jumps with stacks; AR 9.3, CS 4.2), autoplayed for 45 s, with a background image.
  - *Song select:* the user's full stable library (Linux copy: 7,514 difficulties, 1,114 sets shown). It's measured three ways:
    - idle for 8 s;
    - 80 `Down` key presses, one every 100 ms (each selects a map and loads its background and preview);
    - 60 wheel steps.
  - *Startup:* launch to first frame, and to the first console command being processed (ready for input), with a warm data dir.
- **Metrics:**
  - *CPU frame:* app update plus draw, including the swap or submit call.
  - *GPU frame:* OpenGL timestamp queries; not available on SDL_gpu.
  - *Input→present:* SDL event timestamp of a key or button press until the swap call of the frame that handled it returns. Synthetic presses (`sendkey`) go through the same SDL event queue as real ones.
  - *Peak RSS:* `VmHWM`.

## Results

**OpenGL** (`-opengl`):

| Scene | FPS | CPU frame p50 / p99 / max (ms) | p99 run spread | GPU p50 / p99 (ms) | Input→present p50 / p99 (ms) | Peak RSS (MB) |
|---|---:|---:|---:|---:|---:|---:|
| Gameplay, dense map | 5559 | 0.160 / 0.448 / 2.41 | 0.402–0.478 | 0.068 / 0.104 | 0.349 / 0.875 | 489 |
| Song select, idle | 4014 | 0.222 / 0.709 / 1.61 | 0.399–0.914 | 0.072 / 0.081 | – | 565 |
| Song select, key scrolling | 3439 | 0.252 / 0.712 / 7.64 | 0.645–0.953 | 0.089 / 0.100 | 0.670 / 1.344 | 565 |
| Song select, wheel scrolling | 3720 | 0.230 / 0.735 / 4.06 | 0.568–0.831 | 0.071 / 0.096 | – | 565 |

**SDL_gpu / Vulkan** (neomod's default renderer):

| Scene | FPS | CPU frame p50 / p99 / max (ms) | p99 run spread | GPU (ms) | Input→present p50 / p99 (ms) | Peak RSS (MB) |
|---|---:|---:|---:|---:|---:|---:|
| Gameplay, dense map | 8884 | 0.094 / 0.466 / 2.31 | 0.466–0.597 | n/a | 0.234 / 0.822 | 617 |
| Song select, idle | 6326 | 0.135 / 0.477 / 1.79 | 0.458–0.478 | n/a | – | 722 |
| Song select, key scrolling | 5923 | 0.144 / 0.556 / 2.05 | 0.502–0.625 | n/a | 0.416 / 0.790 | 722 |
| Song select, wheel scrolling | 6610 | 0.136 / 0.457 / 1.90 | 0.444–0.530 | n/a | – | 722 |

**Startup and memory:**

| | OpenGL | SDL_gpu |
|---|---:|---:|
| First frame | 144 ms | 432 ms |
| Ready for input | 134 ms | 424 ms |
| Peak RSS at the main menu | 472 MB | 591 MB |
| First library import (7,514 diffs from `osu!.db`, cold data dir; launch → "Loading 1114 beatmapsets") | 0.22 s | 0.44 s |

"Ready" comes before "first frame" because the first console command runs during the first update, before that frame is drawn.

**What this says:**
- neomod is extremely light at this resolution: under half a millisecond of CPU per frame at p99, and a tenth of a millisecond of GPU.
- The occasional song-select hitch (max 7.6 ms on GL while key-scrolling) is the background and preview loading for each newly selected map.
- SDL_gpu does more frames per second (lower p50) but has the same p99, a slower startup and about 120 MB more memory.

## The rules these numbers enforce

**Gameplay regression rule (ground rule 4):**
- A gameplay CPU-frame p99 more than 5% above this baseline is a bug.
- p99 varies about ±10% between runs, so the rule applies to the **median of at least 3 runs**. A change fails when its median p99 exceeds both 1.05× the baseline median and the slowest baseline run.
- `tools/bench/report.py --baseline docs/renovation/bench/baseline-neomod-0fedcafb-gl.json new.json` applies exactly this, and exits non-zero on a gameplay regression.
- Re-measure with `--runs 10` before acting on a borderline result.

**Menu frame-time budget** (reference machine, 2560×1440, headless, both renderers):

| Metric | Budget | Baseline (GL) | Why |
|---|---|---|---|
| CPU frame p99 in any menu scene | ≤ 1.5 ms | 0.71–0.74 ms | about 2× headroom for the remastered UI (glass, glow, richer visualizer) |
| GPU frame p99 in any menu scene (GL) | ≤ 1.5 ms | 0.08–0.10 ms | blur must be cached; per-frame blur would blow this |
| Worst frame while scrolling song select | ≤ 8 ms | 7.6 ms | never worse than today's hitch; one 120 Hz frame |

The budget keeps every menu far inside one 240 Hz frame (4.17 ms) on the user's monitor.

## Not measured yet (planned)

- **On-screen presentation:** compositor, vsync and fullscreen flip. Planned for the first play-test, with the user's OK, because it opens a window on their desktop.
- **Cursor motion latency.** neomod polls the cursor position every frame instead of using motion events, so it isn't in the input→present numbers.
- **Audio cost.** Headless uses a dummy audio device. Audio latency (BASS vs SoLoud) is measured separately for the default-backend decision (workstream H).
- **Windows numbers:** these come with the first Windows play-test.
