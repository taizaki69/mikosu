# mikosu: notes for Claude

mikosu is an osu! client: a fork of neomod (itself McOsu's descendant) being renovated into an "osu!stable, remastered" third client. The requirements are in `docs/renovation/BRIEF.md`.

**Session protocol:**
- Start every session by reading this file, `docs/renovation/PLAN.md`, `docs/renovation/PROGRESS.md` and `git log --oneline -20`.
- End every session by updating `PROGRESS.md` (and `STATUS.md` and `status.json` when they changed), then commit.

## The user

- A player, not a programmer. Work autonomously.
- Ask only for taste, product, account or data decisions, money, things only they can do (sudo, logging in, rebooting into Windows) and play-tests.
- Batch questions. Use plain words. Give a recommendation and say what you'll do if they don't answer.
- Commands they must run go in the **final message** of a turn, one per bash block, each with one sentence on what it does. They don't see text written before a tool call.

## Ground rules

1. **Never connect to official osu! servers (Bancho, score submission).**
   - Never spoof client versions or hashes, never use lazer's OAuth credentials or the `lazer` scope, and always identify as mikosu.
   - osu!api v2 access is read-only, at most 60 requests per minute.
   - No secrets in the repo.
2. **The user's osu! data is read-only.** That's stable (Windows drive and the Linux copy at `~/.local/share/osu-stable`), lazer, and McOsu (`~/.local/share/Steam/steamapps/common/McOsu`).
   - Any write-back must be opt-in, atomic and backed up.
   - Never write into lazer's dir.
   - Never write to a hibernated NTFS volume.
3. **GPL-3.0 code. Ship nothing of ppy's** (no osu-resources, no Torus, no osu! logo or cookie). Audit every asset's provenance.
4. **Gameplay feel comes first.**
   - A gameplay p99 frame-time regression of more than 5% against `docs/renovation/BASELINE.md` is a bug.
   - So is any change to judgement timing outside the deliberate parity work.
5. **Keep it shippable.** Main always builds on Linux and Windows. Work on branches, in small commits. Evolve the code rather than rewriting it.
6. **Never drop a player-visible feature without asking.** VR and Steam are approved for removal.

## Machine safety (read this before running anything heavy)

**Run every build, game run, corpus run or long test through `tools/dev/guarded`.** It puts the command in a systemd user scope with a memory cap, a task cap and low priority (`--bench` drops the low priority for measurements).
- On 2026-10-04 a PATH shim that made `ar`/`nm`/`ranlib` point at `gcc-ar-14` etc. recursed forever and froze the PC twice; the user had to hard-reset both times. **Never put binutils-named symlinks on PATH.**
- Keep builds at `-j10` or less and never run two big builds at once. If the PC froze, check `journalctl -b -1 -k | grep -i oom`.

## Layout

| Path | What |
|---|---|
| `src/` | the game: `Platform/` (SDL3 main loop), `Engine/` (McEngine), `GUI/`, `App/Neomod/` (the osu! app), `Util/`. Map: `docs/renovation/ARCHITECTURE.md` |
| `tools/build.sh` | one-command build (Linux or Windows cross) |
| `tools/dev/guarded` | resource-capped runner for heavy commands |
| `tools/bench/bench.py` | performance benchmark (frame times, latency, startup, memory) |
| `tools/diffcalc/` | standalone star/pp calculator with golden fixtures |
| `tests/sanity/`, `tests/ui/` | headless end-to-end smoke test, scripted UI tests |
| `docs/renovation/` | `BRIEF`, `PLAN` (phases and decision log), `PROGRESS` (session log), `STATUS` (plain-language status for the user), `status.json`, `ARCHITECTURE`, `FEATURES` (brief vs neomod matrix), `BASELINE`, `UPSTREAM`, `DESIGN`, `mockups/`, `screens/` |
| `ref/` (git-ignored) | read-only reference checkouts: `McOsu`, `McEngine`, `osu` (ppy/osu at the pinned tag), `osu-tools` (the oracle, switched to the local `osu` with `UseLocalOsu.sh`), `rosu-pp` |
| `build/`, `build-win64/`, `build-*-dev/` (git-ignored) | build trees |

Dev tooling outside the repo lives in `~/.local/share/mikosu-dev/`:
- `toolchains/llvm-mingw-20260908`
- `gcc14-shim/bin`: compiler drivers only

## Build

**Prerequisites (Ubuntu 24.04 / Mint 22).** gcc-14 is needed because the default gcc 13 lacks `<print>`.

```
sudo apt install g++-14 git cmake ninja-build autoconf automake libtool libtool-bin autoconf-archive autopoint \
  pkgconf meson nasm patchelf ccache gettext glslc glslang-tools spirv-cross curl wget unzip python3 \
  libx11-dev libxext-dev libxi-dev libxrandr-dev libxcursor-dev libxfixes-dev libxss-dev libxtst-dev libxkbcommon-dev \
  libgl-dev libegl-dev libgles-dev libglu1-mesa-dev libdrm-dev libgbm-dev libvulkan-dev libwayland-dev \
  wayland-protocols libdecor-0-dev libasound2-dev libpulse-dev libpipewire-0.3-dev libdbus-1-dev libudev-dev \
  libibus-1.0-dev libattr1-dev libltdl-dev libssl-dev zlib1g-dev
```

**Build commands:**

```
tools/build.sh linux            # release build -> build/dist/bin-x86_64/mikosu
tools/build.sh windows          # cross-build with llvm-mingw (downloaded and verified on first use) -> build-win64/dist/bin-x86_64/mikosu.exe
tools/build.sh linux --dev      # with in-binary tests (-testapp), in build-dev/
```

- The first build downloads about 25 dependency tarballs into `build-aux/cache/` and builds them, which takes about 10 minutes. Later builds are incremental.
- `tools/build.sh` handles three quirks:
  - it runs `autogen.sh` when `configure.ac`, `Makefile.am` or the source list changed (the generated files aren't tracked);
  - it uses a gcc-14 shim when needed;
  - it sets `LC_ALL=C.UTF-8`, because perl/automake segfaults at exit with this machine's `LC_NUMERIC=agr_PE`.
- **After adding or removing a `.cpp`, just run `tools/build.sh` again;** it regenerates the source list.

## Run and test

```
build/dist/bin-x86_64/mikosu                                  # normal run (writes its data next to the binary)
build/dist/bin-x86_64/mikosu -datadir /tmp/x -multi          # throwaway data dir, alongside a running instance
python3 tests/sanity/run.py build/dist/bin-x86_64/mikosu -- -opengl   # headless smoke test (also without -opengl)
python3 tools/bench/bench.py --game build/dist/bin-x86_64/mikosu --renderer gl --library ~/.local/share/osu-stable --out x.json
ref/osu-tools/PerformanceCalculator/bin/Release/net10.0/PerformanceCalculator difficulty <map.osu> -j   # oracle, offline
```

**Headless runs and screenshots:**
- Add `-headless`, which gives offscreen GL on the real GPU and dummy audio, and drive the game through stdin. Commands: `set_active_ui_screen songbrowser`, `sendkey Return`, `mouse_to x y`, `@wait_secs N`, `take_screenshot name.png` (written to the data dir), `ui_assert ...`, `exit`.
- For a size above 1024×768, set `SDL_VIDEO_OFFSCREEN_DISPLAY_SIZE=WxH` (our SDL patch) **and** pass `-w W -h H`. The offscreen GL surface can't be resized after the window is created.
- Wrap runs in `tools/dev/guarded --mem 6G --tasks 512 --`.

## Conventions

- Match the surrounding code: C++23, `.clang-format`, `debugLog(...)`, ConVars for settings (`CONVAR(...)` in `*ConVarDefs.h`), console commands as ConVars with callbacks.
- Internal names stay neomod's (`namespace neomod`, `src/App/Neomod`, `NEOMOD_*`) so upstream fixes keep cherry-picking cleanly. User-facing identity goes through the branding definition (see the PLAN decision log).
- New dependency patches go in `build-aux/misc/` and are applied in `Makefile.am` next to the existing ones. List mikosu-specific patches in `docs/renovation/UPSTREAM.md`.
- Commits: small, on branches for feature work, with the `Co-Authored-By` trailer from the system reminder. The repo-local git identity is `taizaki69 <127163458+taizaki69@users.noreply.github.com>` (the user's GitHub noreply address; never their personal email).
- Never commit the user's osu! content: maps, skins, replays, or screenshots with beatmap art. Mockups use the generated art in `docs/renovation/mockups/src/art/`.
