# mikosu architecture map

How the code inherited from neomod is organised: where things live, how a frame flows, and which areas each renovation workstream touches. Surveyed at upstream `0fedcafb` (neomod 43.14-dev, 2026-10-04). Update this file when a subsystem moves.

Line counts are from that survey: about 245k lines of C/C++ in `src/` plus `libraries/`.

## Layers

```
┌────────────────────────────────────────────────────────────────────────────┐
│ App: src/App/Neomod (80k lines)                                              │
│   the osu! game: screens, gameplay, skins, database, diffcalc, online       │
├────────────────────────────────────────────────────────────────────────────┤
│ GUI: src/GUI (8k)                                                            │
│   CBaseUI* widget toolkit inherited from McEngine                           │
├────────────────────────────────────────────────────────────────────────────┤
│ Engine: src/Engine (41k)                                                     │
│   Engine, ConVars/console, Renderer, Sound, Input, Resources, File, Async,  │
│   Network, i18n, Discord, profiler, logging                                 │
├────────────────────────────────────────────────────────────────────────────┤
│ Platform: src/Platform (6.5k)                                                │
│   SDL3 main callbacks, window, event pump, paths, launch args, Win32 bits   │
├────────────────────────────────────────────────────────────────────────────┤
│ Util: src/Util (6.5k) and libraries/ (bundled header libs, glad, md5)        │
└────────────────────────────────────────────────────────────────────────────┘
Third-party dependencies are built from source by the autotools build (see Build).
```

The engine is McKay's McEngine, heavily modernised by neomod. The app layer is McOsu's `Osu*` code, renamed and extended (`src/App/Neomod`). Copyright headers: `PG` is McKay, `kiwec` is neomod, `WH` is whrvt/spectator.

## Repository layout

| Path | What |
|---|---|
| `src/Platform/` | `main.cpp` (process setup, SDL hints, headless setup), `main_impl.cpp` (`SDLMain`: event handling, `iterate()` loop, window creation, scripted input commands `sendkey`/`mouse_to`/…), `Paths.cpp` (data dirs, `-datadir`), `LaunchArgs.cpp` (all CLI switches), `Environment*` (OS services), `Windows/` |
| `src/Engine/` | `Engine` (owns input devices, GUI root, console), `ConVars/` (ConVar system, console, cfg exec), `Renderer/` (`Graphics` API plus `OpenGL/`, `DirectX11/`, `SDLGPU/`, `NullGraphics/`, `ModernShared/`), `Sound/` (`SoundEngine` plus `BASS/` and `SoLoud/` backends, `PlaybackInterpolator`), `Input/` (mouse, keyboard, touch, key bindings), `Resources/` (`ResourceManager`, images, FreeType fonts, texture atlas, `FFmpegInterop/` for video), `File/` (files, archives via libarchive, async IO, `DirectoryWatcher`), `Async/` (`AsyncPool` worker lanes), `NetworkHandler` (curl, websockets), `FPSLimiter`, `Timing`, `Profiler` (VPROF), `Logging` (spdlog), `i18n` (gettext) |
| `src/GUI/` | `CBaseUIElement`/`Container`/`Button`/`Label`/`Textbox`/`ScrollView`/…, `CBaseUIDispatch` (mouse routing), `CBaseUIEventCtx` |
| `src/App/Neomod/` | the game (see below) |
| `src/App/Tests/` | in-binary test apps, run with `-testapp <name>` (needs `--enable-tests`) |
| `src/App/AppRegistry.cpp`, `AppRunner.*` | picks which app runs (the game, or a test app) |
| `libraries/` | glad, fast_float, unordered_dense, boost/sort, delegates, md5/sha256, stb_image, neotrace |
| `assets/` | fonts, `materials/` (UI icons and **`materials/default/`, the built-in default skin, 260 files**), GLSL shaders (`shaders/*.glsl`, compiled to SPIR-V and transpiled per backend at build time), icons, `misc/credits.txt` and `licenses.txt` |
| `tools/diffcalc/` | standalone star/pp calculator (`BUILD_TOOLS_ONLY`, CMake) with golden fixtures, batch and crosscheck modes |
| `tools/neomod.js/`, `tools/neomod-migration-trampoline/` | wasm runner; neosu→neomod migration helper |
| `tests/sanity/` | boots a finished build headless, autoplays a generated map, checks the ranking-screen screenshot (CI on Linux and Windows) |
| `tests/ui/` | scripted UI tests (`scripts/*.txt` piped to stdin) plus pixel probes |
| `tests/network/` | a fake server for network tests |
| `cmake-win/` | MSVC + CMake build for Windows debugging |
| `mk/`, `build-aux/`, `m4/`, `configure.ac`, `Makefile.am`, `autogen.sh` | autotools build. `build-aux/cache/` holds downloaded dependency tarballs (git-ignored) |
| `translations/` | gettext `.po`/`.pot`. **The build rewrites them when UI strings change** (see the hygiene notes in `PLAN.md`) |

## Startup and the frame loop

1. `main()` (`src/Platform/main.cpp`) parses launch args, initialises logging, sets SDL hints, and in `-headless` mode selects SDL's `offscreen` video driver (OpenGL) and a dummy audio driver. Then it hands control to SDL's main callbacks.
2. `SDLMain::initialize()` creates the window and renderer, then `Engine::loadApp()` builds the app (`Osu`) through `AppRegistry`.
3. Each SDL event goes to `SDLMain::handleEvent()`, which feeds the input devices (`Mouse`, `Keyboard`, `Touch`). Keyboard and mouse listeners then receive the events.
4. Each iteration, `SDLMain::iterate()` runs `Engine::onUpdate()` (the app update: gameplay, UI animations, async completions), then `Engine::onPaint()` (draw), then `FPSLimiter::limit_frames()`. Update, draw and present are **single-threaded on the main thread**, and the limiter sleeps precisely only in gameplay.
   - The FPS cap is `fps_max` in gameplay (default: 4× the display refresh rate), `fps_max_menu` in menus (default: the refresh rate) and `fps_max_background` when unfocused. Headless runs uncapped.
   - Music time comes from the audio backend through `PlaybackInterpolator`, which smooths the backend's coarse position against the engine clock. Gameplay reads the interpolated position every frame.
5. Worker threads: `AsyncPool` (foreground and background lanes, used for resource loading, the database loader, thumbnails, volume normalisation and batch star calculation), `NetworkHandler` (curl), `DirectoryWatcher`, `ConsoleReader` (stdin commands for headless and scripted runs), and the audio backends' own mixer threads.

## The game (`src/App/Neomod`)

- **Root:** `Osu.cpp` (app object, convar callbacks, skin and audio setup, data migration). `UI.h`/`UI.cpp` hold the **screen registry**: one row per screen, with explicit layer ranks. There's a base band (main menu, song browser, ranking, lobby, …) and an overlay band (pause, chat, mod selector, options, prompts, tooltips, volume, notifications). `set_active_ui_screen <name>` switches screens from the console and from test scripts.
- **Screens:** `MainMenu*`, `SongBrowser/` (carousel `BeatmapCarousel`, `SongButton`/`SongDifficultyButton`/`CollectionButton`, `InfoLabel` beatmap info, `ScoreButton` leaderboard rows, `BottomBar`, `AsyncSongButtonMatcher` search, `VolNormalization`), `ModSelector`, `OptionsOverlay` (4.5k lines, sectioned, with search), `RankingScreen` (results), `PauseOverlay`, `HUD` (2.8k lines), `UserStatsScreen`, `OsuDirectScreen` (online beatmaps), `Lobby`/`RoomScreen`/`SpectatorScreen`/`Chat` (online), `AboutScreen` (credits and changelog), `NotificationOverlay`, `PromptOverlay`, `TooltipOverlay`, `VolumeOverlay`, `BeatmapInstallOverlay`.
- **Gameplay:** `BeatmapInterface` (4.5k lines: playing state, music sync, input → clicks, judgement dispatch, skipping, failing, spectating, replays), `HitObjects.cpp` (circles, sliders, spinners and their judgement), `SliderCurves`/`SliderRenderer` (SDF slider rendering), `GameRules` (AR/OD/hit-window formulas; stable-style constants), `ModFlags.h` (64-bit mod flags including McOsu's experimental mods; `LegacyFlags` for stable interop), `ModFPoSu` (first-person mode), `score.*` (`FinishedScore`, which holds stable-style counts only: 300/100/50/geki/katu/miss and max combo), `LegacyReplay` (`.osr` read and write), `Replay`, `SimulatedBeatmapInterface` (replays a score without drawing; the seed of the headless runner).
- **Difficulty and pp:** `DiffCalc/`. `DifficultyCalculator` ports lazer's osu! calculator (`PP_ALGORITHM_VERSION` 20260811, which already has the 2026 Q2 rework). `StarPrecalc`/`BatchDiffCalc` compute stars across the library in the background, `LivePPCalc` gives in-game live pp, and `AsyncPPCalculator` serves on-demand requests. `OsuConVars/DiffCalcDefaults.h` shares defaults with `tools/diffcalc`.
- **Skins:** `Skin.cpp` (skin.ini, element lookup, `@2x`, fallback to `assets/materials/default`, `[neomod]` convar section in skin.ini), `SkinImage` (animated and pre-sized elements), `SkinArchive` (`.osk`), `HitSounds`.
- **Library and data:** `Database.cpp` (3.1k lines) reads stable `osu!.db`, `scores.db` and `collection.db`, McOsu/neosu `scores.db`/`collections.db`, and its own `neomod_maps.db`/`neomod_scores.db`. It reconciles the `maps/` folder, imports loose `.osz`, and computes player stats with the 0.95ⁿ weighting plus bonus pp. `DatabaseBeatmap` parses `.osu` files. `Collections`, `BeatmapInstaller`, `MapExporter`, `SettingsImporter` (stable `osu!.<user>.cfg`, McOsu cfg, Windows registry lookup of the stable folder), `ThumbnailManager`, `BackgroundImageHandler`, `PreviewTrackManager`.
- **Online (private servers):** `Bancho*` (login, packets, AES, leaderboards, score submission, users), `Downloader`/`MapFetcher` (beatmap mirrors, osu.direct-style search), `UpdateHandler` (self-updater against `https://neomod.net`), `RichPresence` (Discord), `NeomodUrl` (URL scheme), `Lobby`/`RoomScreen`/`SpectatorScreen`.
- **Settings:** about 860 ConVars (`OsuConVars/OsuConVarDefs.h` has 701, `Engine/ConVars/ConVarDefs.h` 157). They're saved to `cfg/osu.cfg`, can be overridden from skin.ini, and are all reachable from the console.

## Data on disk (neomod defaults)

Portable layout. The writable data dir defaults to the executable's directory (`APP_DATA_DIR`), or wherever `-datadir` points. Inside it:

- `cfg/`
- `maps/`
- `skins/`
- `replays/`
- `screenshots/`
- `exports/`
- the databases
- `cache/` (on Linux, `$XDG_CACHE_HOME/neomod`)
- `logs/`

The stable folder is configured with the `osu_folder` convar. The default detection is the Windows registry and `%LOCALAPPDATA%\osu!` on Windows. On Linux and Wine it checks `~/.local/share/{osu!,osu-wine,osu-wine/osu!,osu,osu/osu!}` for `osu!.exe`. **It wouldn't find this user's `~/.local/share/osu-stable`, any NTFS mount, or lazer.**

## Build

- **Linux and MinGW:** autotools. `configure.ac` (2.1k lines) pins about 25 dependencies (SDL3 at a git commit, curl, OpenSSL, FFmpeg, FreeType, libarchive, mpg123, SoLoud fork `neoloud`, BASS from archive.org, mimalloc, nsync, spdlog, fmt, simdutf, ctre, …). `Makefile.am` downloads them into `build-aux/cache/` and builds them into `<builddir>/build/deps`, then builds `neomod` with LTO and a PCH. `make install` stages a portable folder under `<builddir>/dist/bin-<arch>/`.
  - **Gotchas:**
    - `configure.ac` hard-codes `CC=gcc`/`CXX=g++` (or clang). It needs GCC ≥ 14 for `<print>`; this machine defaults to 13, hence the gcc-14 shim.
    - The build rewrites tracked `translations/*.po` files.
    - The checked-in generated files (`configure`, `Makefile.in`, …) are from autoconf 2.73; running `autogen.sh` with distro autoconf 2.71 churns them.
- **Windows (MSVC):** `cmake-win/` (CMake), used by upstream for debugging only.
- **Generated sources:** `autogen.sh` regenerates `src/Makefile.sources` with `find`. Adding or removing a `.cpp` requires running it.
- **CI (upstream):**
  - `linux-multiarch`: Docker image `whrvt/neomod-linux`, gcc-14, plus an llvmpipe sanity test.
  - `win-multiarch`: Arch container with llvm-mingw for x64 and arm64 and mingw-gcc for x86, plus a sanity test on real Windows runners using WARP.
  - Also `linux-aarch64`, `macos-arm64` and `web-wasm32`.

## Tests and tooling that already exist

- `-headless` (offscreen GL, dummy audio, uncapped), `-datadir`, `-multi`, stdin console scripting (`sendkey`, `mouse_to`, `set_active_ui_screen`, `ui_assert`, `take_screenshot`, `@wait_secs`, `exit`).
- `tests/sanity/run.py`: an end-to-end smoke test.
- `tests/ui/`: scripted UI assertions and pixel probes.
- `src/App/Tests/`: in-binary tests (`ConVarTest`, `CryptoTest`, `AsyncPoolTest`, `NetworkTest`, `Neomod/*` such as `SkinLoadTest`, `SliderRenderTest`, `PacketTest`).
- `tools/diffcalc`: golden star/pp fixtures (exact compare), a corpus batch mode and crosscheck.

## Where the renovation lands

| Workstream | Main touch points |
|---|---|
| A: branding and tooling | `PACKAGE_NAME` (configure.ac), `NEOMOD_DOMAIN`/`OSU_VERSION` (`BanchoNetworking.h`), app metadata in `main.cpp`, window title (`main_impl.cpp`), `Paths.cpp`, `UpdateHandler`, `assets/`, CI workflows, tests |
| B: star rating and pp | `DiffCalc/*`, `tools/diffcalc`, `Database` (cache and storage of SR), `score.*` (algorithm id), `RankingScreen`/`HUD`/`InfoLabel` (display) |
| C: scoring and judgements | `HitObjects.cpp`, `BeatmapInterface`, `GameRules`, `LiveScore`, `score.*`, `LegacyReplay`, `SimulatedBeatmapInterface` (headless runner) |
| D: library and data | `Database.cpp`, `SettingsImporter`, `Osu::getDefaultFallbackOsuFolder`, `Collections`, `DirectoryWatcher`, `Skin` (case-insensitive lookup), new lazer reader |
| E: online | new osu!api v2 client (beside `Bancho*`), `ScoreButton`/leaderboard, `Downloader`/`MapFetcher`, `RichPresence`, `Bancho.cpp` (honest client identification) |
| F: UI | every screen file, `Skin`, `GUI/`, new design-token and theme module, fonts in `assets/` |
| G: gameplay features | storyboards (upstream branch `sb` is in progress), `FFmpegInterop`, `ModSelector`/`ModFlags`, replay viewer in `BeatmapInterface`/`HUD` |
| H: release | CI, packaging scripts, `UpdateHandler`, `assets/misc/licenses.txt` |
