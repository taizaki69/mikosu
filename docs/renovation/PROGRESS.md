# Progress log

Newest session first. Every claim cites its evidence (a test, a measurement or a screenshot).

## 2026-10-04: Session 1, continued (stopped at the usage limit)

### What changed
- **GitHub:** the repo is public at https://github.com/taizaki69/mikosu, as a standalone repo with `main` as the default branch.
  - Before the first push, history was rewritten so commits are authored with the user's GitHub noreply address, and machine paths and personal details were scrubbed from the docs.
  - `upstream` push is disabled.
- **`README.md` and `CREDITS.md`** for mikosu. neomod's README moved to `docs/renovation/NEOMOD-README.md`.
- **Design round 1** (faithful / refined / bold) was **rejected** by the user as "way too old style".
  - New direction (see `DESIGN.md` and PLAN decisions 17–18): stable layout, modern styling borrowed from lazer. The user picked slanted shapes, a clean modern font and flat dark panels, with a "clean and calm" feel.
  - Not picked: colour-coded star rating.
- **Design round 2 is in progress.** Sources are `mockups/src/{modern.css,songselect2.html,mainmenu2.html}`, with variants `all` / `controls` / `accents` (how much is slanted). The font is Outfit, with M PLUS 1 for CJK.
  - Renders: `mockups/*2-*.jpg`. **Not shown to the user yet.**
  - Known glitches to fix first:
    - the song-select carousel overlaps the Group/Sort row (start the carousel lower, around y=140);
    - unselected difficulty rows show the star rating on a line above the name; inspect `.panel.diff .d` in a browser.
  - Then check `mainmenu2-*.jpg`, rebuild the decision page (the artifact link in `DESIGN.md`) with round 2 and ask the user to pick.
- **Lesson:** `pkill -f <pattern>` also matches the shell running it. Use `pkill -f "[h]ttp.server"`-style patterns.

### Next
1. Finish round-2 mockups → user picks → write `DESIGN.md` tokens.
2. Phase 1 (`PLAN.md`): branding module, honest network identity, XDG data dirs, CI on GitHub Actions (the repo exists now), unit tests, headless runner, screenshot tool, Wine smoke test, asset audit.

## 2026-10-04: Session 1, bootstrap

### What changed

**Repo**
- mikosu repo created by cloning neomod. `upstream` remote; `main` starts at `0fedcafb`.
- Reference checkouts in `ref/`: McOsu, McEngine, ppy/osu at `2026.921.0-lazer`, osu-tools, rosu-pp.

**Tools**
- The user installed the build packages; `libattr1-dev` was added later.
- llvm-mingw 20260908 is in `~/.local/share/mikosu-dev/toolchains` (SHA-256 checked against GitHub's digest).
- osu-tools `PerformanceCalculator` is built against the local ppy/osu (.NET 10).

**Build system** (commits on `main`):
- `tools/build.sh`: one-command Linux or Windows build. It regenerates autotools files when needed, uses a gcc-14 shim, and sets `LC_ALL=C.UTF-8`.
- `tools/dev/guarded`: a cgroup-capped runner. It was added after the PC froze twice; see below.
- Generated autotools files are no longer tracked.
- `gen_translations.py` no longer rewrites tracked `.po` files on clean builds.
- SDL3 patch `SDL3-offscreen-display-size.patch`, so headless rendering can go beyond 1024×768.

**Measurement**
- `src/Engine/FrameStats.*` (`-benchout`, inert otherwise).
- `tools/bench/bench.py` and `report.py`.
- `tools/screens/capture_before.py`.

**Docs**
- `BRIEF.md`, `ARCHITECTURE.md`, `FEATURES.md`, `BASELINE.md`, `PLAN.md` (phases and decision log), `UPSTREAM.md`, `STATUS.md`, `status.json`, `DESIGN.md` (draft), `mockups/` (3 directions × 2 screens), `CLAUDE.md`.

**Decision page for the user:** https://claude.ai/artifact/WPKRiAu87RZV2pKhaAQncj

### Verified, and how

- **Linux build:** `tools/build.sh linux` passes; the binary was `build/dist/bin-x86_64/neomod` (renamed to `mikosu` in Phase 1).
- **Sanity test:** `tests/sanity/run.py` passes with `-opengl` (19.4 s) and with the default SDL_gpu (20.0 s).
- **Headless rendering at 2560×1440** on the real GPU (NVIDIA EGL), after the SDL patch and `-w/-h`. The screenshots show the full gameplay HUD and the user's library in song select.
- **The user's data was found and only read:**
  - stable on the Windows drive (mounted read-only via `udisksctl … -o ro`) and a Linux copy at `~/.local/share/osu-stable`;
  - `osu!.db` v20260711 with 7,514 diffs, `scores.db` with 84 scores (54 with online ids), `collection.db` with 1 collection, 86 replays in `Data/r`, 13 skins;
  - lazer at `AppData/Roaming/osu` (`client.realm`, 31k files), with no `storage.ini`;
  - McOsu in Steam.
  - No writes anywhere.
- **Baseline:** `BASELINE.md`, with raw JSON in `bench/`. For example, OpenGL gameplay CPU p99 is 0.448 ms, GPU p99 0.104 ms, input→present p99 0.875 ms, and startup to ready is 134 ms.
- **Oracle spot check, fixture 2785319 NM:**
  - star rating 6.004272 (neomod) vs 6.004027 (osu-tools): Δ 2.4e-4, outside the 1e-4 goal;
  - reading 0.82918 vs 0.82292.
- **Windows cross-build:** the first attempts compiled. One failed at link because a parallel build picked up half-written FrameStats edits, and automake crashes were traced to the locale bug. A rebuild through `tools/build.sh windows` was running at session end; see the next entry.

### Incidents

**The PC froze twice; the user had to hard-reset.**
- *Cause:* a PATH shim with `ar → gcc-ar-14` (and `nm` and `ranlib`). gcc-ar runs `ar` from PATH, which was itself, so it recursed forever: a fork bomb, then a global OOM. `journalctl -b -1` showed thousands of `ar`/`nm` processes; boot −3 ended with the OOM killer hitting a desktop app.
- *Fix:* the shim now holds compiler drivers only.
- *Prevention:* all heavy work runs through `tools/dev/guarded` (MemoryMax 16G, TasksMax, low priority). The rule is in `CLAUDE.md` and in memory.

### Next

1. **Wait for the user's answers:** design direction (A/B/C, recommended B) and the GitHub repo.
2. **Phase 1:**
   - the branding module and honest network identity;
   - XDG data dirs;
   - CI (once the repo exists);
   - the doctest unit-test target;
   - the headless runner;
   - the screenshot tool;
   - the Wine smoke test;
   - the asset audit;
   - `CREDITS.md`.
3. **Then in parallel:**
   - B: the parity corpus and divergence fixes, starting with the Reading skill;
   - D: detection and the full `scores.db` import;
   - F: `DESIGN.md` finalised in the chosen direction.

### Open questions (for the user)

- Design direction: A Faithful, B Refined or C Bold (or a mix).
- Put the project on GitHub now? A public repo gives free CI; a private one gets about 2,000 free CI minutes a month. This also needs their GitHub username, for the commit email.
