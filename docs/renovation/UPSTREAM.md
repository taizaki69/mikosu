# Upstream policy and log

**Upstream** is `neomodnet/neomod` (remote `upstream`, branch `master`).

- **Fork point:** `0fedcafb3515c86d8be8ee736c76bce19d0198d3`, "improve audio quality with rates >1.0 (esp. vocals)", 2026-10-04.
  - It was master's HEAD when the fork was made. CI was green on Linux, Windows, macOS and wasm.
  - It's three weeks past release v43.13 (`6db462ea`), so its fixes are included.
- mikosu's `main` starts there.

## Policy

- **Review upstream at every milestone:** run `git fetch upstream` and read `git log main..upstream/master`.
- **Cherry-pick fixes worth having:** engine, gameplay, importers, the server protocol, crash fixes and security fixes. Use `git cherry-pick -x` so the origin stays visible.
- **Skip what conflicts with the redesign or the rules:**
  - UI restyling in screens we're redesigning;
  - anything that sends neomod's identity to servers;
  - neomod.net endpoints;
  - the self-updater.
- **Don't stay merge-compatible** in redesigned areas. Do keep internal names (`namespace neomod`, `src/App/Neomod`) so unrelated fixes still apply cleanly.
- **Generated autotools files aren't tracked in mikosu.** When a cherry-pick touches `configure`, `Makefile.in`, `aclocal.m4`, `src/config.h.in` or `src/Makefile.sources`, resolve with `git rm` on those paths; `tools/build.sh` regenerates them.
- **Record every review below:** the range reviewed, what was taken (with mikosu hashes) and what was skipped and why.

## Upstream work to watch

- Branch `sb` (storyboards, in progress). Workstream G wants it; watch it before building our own.
- Branch `async-gl-image`.
- The difficulty calculator. neomod tracks lazer closely. Compare their `PP_ALGORITHM_VERSION` bumps with our versioned calculator (workstream B).

## mikosu-specific changes to upstream files (to re-check on each review)

| Change | Files | Why |
|---|---|---|
| Generated autotools files untracked | `.gitignore`; removed `configure`, `Makefile.in`, `aclocal.m4`, `src/config.h.in`, `src/Makefile.sources` | distro autotools versions churned them; `tools/build.sh` regenerates |
| `gen_translations.py` leaves the tracked `.pot`/`.po` alone unless the string set changed | `build-aux/gen_translations.py` | gettext < 0.22 rewrote them on every clean build |
| SDL3 offscreen display size from `SDL_VIDEO_OFFSCREEN_DISPLAY_SIZE` | `build-aux/misc/SDL3-offscreen-display-size.patch`, `Makefile.am` | headless screenshots and benchmarks at real resolutions |
| FrameStats benchmark recorder (`-benchout`) | `src/Engine/FrameStats.*`, `src/Platform/main*.cpp`, `LaunchArgs.*`, `SDLGLInterface.cpp` | performance baseline and regression checks |

## Review log

| Date | Range | Taken | Skipped |
|---|---|---|---|
| 2026-10-04 | fork at `0fedcafb` | — | — |
