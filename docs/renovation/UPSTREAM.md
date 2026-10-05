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
| Upstream CI workflows replaced by `.github/workflows/ci.yml` (Linux and Windows x64 only, built with `tools/build.sh`) | `.github/workflows/` | mikosu targets Linux and Windows; upstream's workflows use neomod's Docker image and the `master` branch. **Skip** workflow changes when cherry-picking |
| Identity from `AC_INIT` and `src/Branding.h` (name, version 0.1, URLs, app id, user agent, logo, Discord id empty) | `configure.ac`, `Makefile.am`, `src/Branding.h`, `Osu.cpp`, `MainMenu.cpp`, `main.cpp`, `BanchoNetworking.h`, assets | rebrand (PLAN #20). **Check every cherry-pick for new user-visible "neomod" strings and new identity sent to servers** |
| HTTP requests send mikosu's user agent, never `osu!` | `Bancho*.cpp`, `Chat.cpp`, `Downloader.cpp`, `LegacyReplay.cpp`, `OsuDirectScreen.cpp` | ground rule 1 (PLAN #21). **A cherry-pick that adds `.user_agent = "osu!"` must use `BanchoState::user_agent`** |
| neomod's config migrations only for a neomod-lineage `version.txt` (>= 30) | `MainMenu.cpp` | mikosu's 0.x versions are below every threshold (PLAN #20). New upstream migrations go inside the same check |
| Windows registry: mikosu's own ProgID and URL protocol only; neosu's entries no longer deleted | `NeomodEnvInterop.cpp` | don't take over or remove another client's registrations |
| Old `neomod_*.db` / `neosu_*.db` copied to `mikosu_*.db` on load | `Database.cpp` | data folders from neomod builds (portable installs) |
| UI test runner sends `force_oauth 1` before every script | `tests/ui/run.py` | the scripts were recorded against neomod's default server (compact OAuth form); mikosu's default server is empty |
| Include dirs found relative to `src/` | `Makefile.am` (`NEOMOD_INCLUDE_FLAGS`) | upstream bug: a checkout below a hidden directory lost every include dir. **Worth offering upstream** |
| Precompiled header compiled with `CCACHE_DISABLE=1` | `Makefile.am` | ccache returned a stale `.gch` after a `config.h` define changed (pch_defines sloppiness). **Worth offering upstream** |
| Linux: SDL's video init bypasses the X input method (`XMODIFIERS=@im=none`, restored after) unless `use_ime` (now off by default on Linux) or `-ime`; an Options checkbox for it | `src/Platform/main.cpp`, `ConVarDefs.h`, `OptionsOverlay.cpp`, translations | IBus's XIM bridge swallowed menu key presses (0 of 30 arrived on a test display; all 30 with the bypass). **Worth offering upstream** |
| SDL patch: X11 keycode-0 key events (an input method's composed character) are handled even with XInput2 keyboards | `build-aux/misc/SDL3-x11-xinput2-ime-commit.patch`, `Makefile.am` (SDL now rebuilds when its patches change) | dead keys (´ + e → é) typed nothing. **Worth offering to SDL** |
| DT/HT mod buttons have NC/DC as second states (new `mod_nightcore_dummy`/`mod_daycore_dummy`); "Prefer Nightcore" option removed | `ModSelector.cpp`, `Osu.cpp`, `Replay.cpp`, `OsuConVarDefs.h`, `OptionsOverlay.cpp`, translations | user request (PLAN 25), like stable |

## Review log

| Date | Range | Taken | Skipped |
|---|---|---|---|
| 2026-10-04 | fork at `0fedcafb` | — | — |
