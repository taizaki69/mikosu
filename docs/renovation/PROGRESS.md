# Progress log

Newest session first. Every claim cites its evidence (a test, a measurement or a screenshot).

## 2026-10-04 evening to 2026-10-05: Session 1, continued (Phase 1 nearly done)

### What changed
- **Design:** the user picked round 2, option 2 "Slanted controls" (PLAN 19, `DESIGN.md`).
- **CI (PR #1, merged):** `.github/workflows/ci.yml` builds Linux and Windows x64 with `tools/build.sh` and smoke-tests both (Linux OpenGL; Windows SDL_gpu and D3D11 on windows-2025), with artifacts `mikosu-linux-x64` and `mikosu-win-x64`. The first run failed on a missing `libltdl-dev` (mpg123's autoreconf); fixed.
- **Rebrand (PR #2, merged):** identity from `AC_INIT` and `src/Branding.h` (PLAN 20). A review agent's findings were all fixed before merging:
  - neomod's config migrations would have re-run on every launch and remapped custom keybinds. They now run only for a neomod-lineage `version.txt` (30 and up).
  - the version shows as "0.1", not "0.10" (translations updated).
  - every server request sends mikosu's user agent, never `osu!` (PLAN 21).
  - Windows registrations use only mikosu's own ProgID and `mikosu://`.
  - `cmake-win` identity; the shortcut stamp; old `neomod_*.db` copied in.
  - **Build fixes worth sending upstream:** the include-dir `find` excluded everything when the checkout sits below a hidden directory, and the PCH went stale through ccache.
- **UI tests:** `run.py` sends `force_oauth 1` (scripts were recorded against neomod's default server's compact login form), and the logo probe samples the placeholder wordmark.
  - **Baseline comparison on this machine:** a build of `main` passes 56/71, the rebrand 59/71.
  - Fails on both (environmental or flaky here, not investigated yet): `console_suggestions`, `console_window_select`, `convar_invalid_text`, `convar_lock_vs_skin`, `convar_options_roundtrip`, `convar_skin_optout`, `convar_skin_reload`, `layered_scrollviews`, `mainmenu_nowplaying`, `roomscreen_modselector_overlay`.
  - `dropdown_drag_chain` is flaky on both (1 of 3).
- **Data folders (PR #3, open):** per-user data folder unless portable (PLAN 22).
- **Replay runner** (`replay_run` console command, plus `tools/runner/replay_runner.py`, which matches replays to maps by MD5) found two simulator bugs, fixed:
  - circles and follow circles were sized for the window (≈1.6× too big headless, ≈3× at 1440p), so watched and spectated replays were judged too leniently;
  - the last object was never judged if the frames passed its end time first.
  - **Results on the 84 local stable replays:** exact judgement counts 0 → 63 (75%), exact max combo 0 → 68 (81%). Every replay now judges all of its map's objects.
- **ScoreV1 parity** (PLAN 23):
  - a held slider's tail counts before the slider's judgement;
  - the combo bonus is rounded once, like stable.
  - **Exact total score: 0 → 36 of 84.** Every no-spinner replay whose judgements match now scores exactly, except 7 (sim slightly high, probably combo-break placement). All 14 with spinners are off by spinner spin points.
- **Wine smoke test:** `tools/dev/wine-smoke` (own prefix `~/.local/share/mikosu-dev/wine-smoke`, never `~/.wine`).
  - Wine 9.0: default renderer and `-dx11` pass.
  - `-opengl` can't run headless on Windows (no EGL); exits cleanly.
- **Stable detection:**
  - `~/.local/share/osu-stable` is now among the fallback folders.
  - The first-launch import of stable's settings now finds that folder itself. Before, it ran before the fallback and never worked on Linux.
  - It falls back to the newest `osu!.*.cfg` when the Windows user name differs.
  - **Verified on a fresh data dir:** library, skin, volumes and scores came over; the osu! and McOsu folders were untouched (`find -newermt`).

- **Stable scores:** every osu!standard score in stable's `scores.db` is imported, submitted or not: 84/84 (was 54; the 30 missing were the player's own unsubmitted plays, all with replays).
- **Tooling:**
  - `tools/screens/capture.py`: 8 screens × (1280×720, 1920×1080, 2560×1440, 3440×1440, 2560×1440 at 2× UI), generated content. The 3440×1440 capture shows song select's group/sort tabs shrinking (a layout bug for F).
  - `headless_fps_max`: an opt-in frame cap for headless runs, so tests stay light while the user uses the PC.
  - CI runs `tools/diffcalc test --tolerance` (11/11). Exact mode differs in the last digit on x86-64 gcc.
  - `THIRD_PARTY_NOTICES.txt` and its generator. BASS in releases is flagged as a user decision (PLAN, H).
- **Test leftovers:** `~/.local/share/mikosu` (a config and two logs from the 2026-10-04 datadirs test) was removed, so the play-test is a real first launch.

### Verified, and how
- Linux sanity test passes on every branch tip (≈19.5 s).
- CI is green on PRs #1 and #2.
- Windows cross-build passes, and so does the Wine smoke test.
- Replay numbers: `tools/runner/replay_runner.py` over `~/.local/share/osu-stable/Data/r`.

### Performance
- **Same-day A/B against a build of neomod's code** (`BASELINE.md` → "Checks"):
  - gameplay p99 0.347 vs 0.417 ms;
  - song select idle / keys / wheel p99 0.378 / 0.501 / 0.390 vs 0.418 / 0.518 / 0.434 ms (10 runs).
  - **No regression.**
- A first run while the user was playing a game was void (everything ≈2× slower).

### Incidents
- The app quit twice mid-run. Nothing was lost: work is committed as it goes, and stray test files were cleaned up.
- The PC was rebooted overnight cleanly (orderly `systemd-reboot`, no OOM).

### Next
1. **Done:** PRs #3–#6 merged (data dirs; replay runner and simulator fixes with the Wine test and golden tests in CI; ScoreV1; first-launch detection, tools and notices). CI green on each.
2. **M1 play-test** handed to the user (`STATUS.md` → "Waiting on you"). A baseline worktree of neomod's code is kept at `.claude/worktrees/baseline` (git-ignored, with the include-dir build fix applied locally) for same-day A/B benchmarks.
3. Phase 1 leftover: the doctest unit-test target (needs doctest.h: vendor it or fetch it with a pinned hash).
4. Workstream C: spinner spin counting; the 7 no-spinner score outliers (compare combo-break placement using the replay's life-bar graph).
5. Workstream B: the SR/pp parity corpus against osu-tools.
6. Workstream F: start the "Slanted controls" redesign.

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
