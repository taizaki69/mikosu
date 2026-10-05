# mikosu renovation plan

The plan for turning neomod into mikosu.

- Requirements: `BRIEF.md`.
- Starting point:
  - `FEATURES.md`: brief vs neomod, item by item;
  - `ARCHITECTURE.md`: code map;
  - `BASELINE.md`: performance numbers.
- Session log: `PROGRESS.md`.
- Plain-language status for the user: `STATUS.md`.
- Upstream handling: `UPSTREAM.md`.

## How the work is organised

- **Phases end in milestones.** Each milestone hands the user a build with a short "try these things" list.
- **Main always builds and plays** on Linux and Windows. Feature work happens on branches, in small commits, and merges after a self-review (a review subagent for anything significant).
- **Parallel work:** once Phase 1's foundations land, workstreams B (star rating and pp), D (library) and F (UI) run in parallel as subagents in separate git worktrees, integrated often. C (judgements) follows B's and A's tooling.
- **Every claim in `PROGRESS.md` needs evidence:** a test, a measurement or a screenshot.
- **Performance gate:** `tools/bench/bench.py` runs before merging anything that touches the frame loop, rendering, gameplay or song select. A gameplay p99 regression above 5% (method in `BASELINE.md`) blocks the merge.

## Phases

### Phase 0: Bootstrap (done 2026-10-04)

- [x] Fork neomod at `0fedcafb` with an `upstream` remote; `ref/` checkouts of McOsu, McEngine, ppy/osu `2026.921.0-lazer`, osu-tools and rosu-pp
- [x] Tools installed; Linux build; Windows cross-build toolchain (llvm-mingw); osu-tools oracle built offline
- [x] Found the user's data: stable on Windows and its Linux copy, lazer, McOsu (read-only)
- [x] `ARCHITECTURE.md`, `FEATURES.md`, `BASELINE.md` (FrameStats recorder + `tools/bench`), `CLAUDE.md`, `STATUS.md`, `UPSTREAM.md`
- [x] Design mockups (three directions) → user decision

### Phase 1: Foundation (workstream A)

Goal: a mikosu-branded build that's honest online, plus the tooling every later phase relies on.

1. **Branding module.**
   - **Decided first:** the code location for it. Name it in the decision log before starting the module.
   - **The definition:** one header with the identity: product name, display name, version scheme, data-dir names, window title, user agent and client identification, URLs, logo asset names.
   - **Route identity through it:**
     - `PACKAGE_NAME` users that are identity;
     - SDL app metadata;
     - window title;
     - `NEOMOD_DOMAIN`;
     - the Bancho version string;
     - the replay writer's version field;
     - Discord app id.
   - **Keep internal code names** (decision 11).
2. **Neutralise neomod's network identity.** No default server. Self-updater off and pointed at mikosu GitHub Releases, opt-in and disclosed (H). The user agent says mikosu. Private-server login sends an honest mikosu version string (E side quest).
3. **Data directories.** XDG on Linux (`~/.local/share/mikosu`, `~/.config/mikosu`, `~/.cache/mikosu`), `%APPDATA%\mikosu` on Windows, and a portable mode (a marker file next to the exe). `-datadir` stays.
4. **CI on GitHub Actions.** Needs the GitHub repo (a user decision).
   - **Builds:** Linux and Windows x64 via `tools/build.sh`, on Ubuntu runners with the same packages as `CLAUDE.md`.
   - **Tests:** the sanity test on Linux (llvmpipe) and on Windows runners (WARP); unit and golden tests; the headless runner.
   - **Artifacts:** both builds uploaded on every push.
5. **Tests.**
   - **Pure logic:** `doctest` (single header, MIT) in a `tests/unit` target built with CMake like `tools/diffcalc`. Covers parsers (`.osu`, `osu!.db`, `scores.db`, `collection.db`, `.osr`), hit-window and mod math, and pp weighting.
   - **Engine-dependent:** the existing `-testapp` mechanism.
   - **Goldens:** JSON files under `tests/golden/`.
6. **Headless runner.** `neomod -headless-run <map.osu> <replay.osr> [--profile stable|lazer|classic] [--json]`, built on `SimulatedBeatmapInterface`. It prints judgements, combo, score, star rating, pp and the algorithm id as JSON. Used by B and C.
7. **Screenshot tool.** `tools/screens/capture.py` renders a named screen headless at 1280×720, 1920×1080, 2560×1440, 3440×1440 and 2× UI scale, using generated content, for the F QA loop.
8. **Windows smoke test under Wine,** with a dedicated `WINEPREFIX` under `~/.local/share/mikosu-dev/` so it never touches `~/.wine` (which library detection scans).
9. **Asset provenance audit.**
   - Inventory every file in `assets/`: fonts, `materials/default` (260 files), icons, sounds.
   - Record source and licence in `docs/renovation/ASSETS.md`. Mark what has to be replaced.
   - Replacements happen in F. `tahoma` and `weblysleekuisb` are first.
10. **`CREDITS.md`, `THIRD_PARTY_NOTICES`.** Credit McKay (McOsu, McEngine), the neomod authors (kiwec, whrvt/spectator and contributors) and the dependency licences, including BASS's terms.

**Milestone M1, "mikosu opens":** a mikosu-branded Linux and Windows build with honest network identity, plus CI green on both. Play-test checklist:
- the build launches;
- it finds your stable library from the Linux copy;
- old neomod features still work: FPoSu, scrubbing, overrides, the console;
- gameplay feels unchanged.

### Phase 2: Correctness (workstreams B and C)

**B: star rating and pp.**

1. **Corpus of 200+ cases.**
   - `tests/parity/corpus.json` lists beatmap ids, md5s, mods, mod settings, rates and categories covering the brief's edge cases.
   - `.osu` files are fetched by script into a git-ignored cache; we never commit map content.
   - Expected values come from the osu-tools oracle at `2026.921.0-lazer`. Only numbers are stored, as JSON goldens.
2. **Find and fix divergences.** Root-cause each one (start with the Reading skill, integer-ms object times at non-1.0 rates, and slider path resampling), then port ppy/osu#38754. Never widen a tolerance.
3. **Version the calculator.**
   - An algorithm id on every result.
   - Several versions side by side: the current port becomes `2026.921`, kept frozen. A future rework is a new version, not an edit.
   - Recalculating stored scores under any version.
4. **Lazer full statistics** (slider tails, ticks, Classic) and the stable ScoreV1-based estimation path.
5. **SR cache.** A persistent cache keyed by md5, mods with settings, rate and version, filled in the background. Song select never waits, even with 100k+ difficulties.
6. **Display** (with F): live pp and pp-if-FC; pp at 95/98/99/100%; a per-skill breakdown; local profile weighting matched to osu-web (verify the current rules: 0.95ⁿ weighting, the bonus pp formula, and which scores count).

**C: scoring and judgements.**

1. **Ruleset profiles** (stable, lazer, lazer + Classic), implemented as data plus policies around the existing hit-object code, not a rewrite.
2. **Hit windows, mod math and score formulas** taken from ppy/osu, with unit tests.
3. **Replay reproduction suite.**
   - Inputs: the user's 86 stable replays (read in place, never copied into the repo) and lazer exports (the user exports a few).
   - Goal: exact matches on hit counts, max combo and total score.
   - Report the match rate per profile and categorise every mismatch.
4. **Score DB schema v2:**
   - full statistics;
   - mod settings;
   - rate, profile and algorithm version;
   - client build;
   - the replay.

   Migration from neomod's DB.

**Milestone M2, "the numbers are right":**
- parity report: SR within 1e-4 and pp within 0.01 across the corpus; replay match rates;
- song select shows SR after mods and pp at 95/98/99/100% (in the chosen design).

### Phase 3: Library and data (workstream D)

1. **Detection.**
   - Scan for `osu!.db` and `osu!.exe` in:
     - NTFS mounts (`/media`, `/run/media`, `/mnt`);
     - Wine prefixes (osu-winello, `~/.wine`, Lutris, Bottles);
     - common copies (`~/.local/share/osu-stable` is this user's).
   - lazer: default dirs plus `storage.ini`.
   - McOsu and neomod: Steam `.acf` and default folders.
   - Fall back to a folder picker.
   - Report an unmounted Windows drive in plain words, with the exact command.
2. **Stable data in place:**
   - every `osu!.db` version;
   - **all** local `scores.db` scores, not just online ones;
   - collections;
   - replays (`Data/r` and `Replays`);
   - skins;
   - `osu!.<user>.cfg` mapping: keybinds, sensitivity, raw input, offsets, volumes, dim, skin. Find the file by the stable username, not the OS username.
   - Case-insensitive file resolution, with a cache that avoids rescans on slow NTFS.
   - Live pickup of new maps.
3. **Checksum guard.** `tools/data/checksum.py` manifests the user's osu! directories before and after runs; that's D's acceptance proof.
4. **lazer spike (time-boxed to one session).**
   - Read `client.realm` with realm-core's C API from a temporary copy, together with the `files/` store.
   - Write a feasibility and maintenance-cost report before any build-out.
   - Fallback: lazer's own exports.
5. **Shared data folder across the dual boot** (opt-in, with the Fast Startup explanation).
6. **Formats.**
   - `.osz`, `.osk` and `.osr` via drag-and-drop and the CLI.
   - `.osr` export compatible with stable and lazer (lazer extra data).
7. **OS integration.**
   - Linux: `.desktop`, MIME types and associations.
   - Windows: the equivalents.
   - An optional `osu://` handler, off by default.

**Milestone M3, "your library, everywhere":**
- the first run on this PC finds stable, lazer and McOsu on both OSes;
- the full library, collections and scores show up;
- checksums prove nothing changed.

Some of this needs a Windows reboot, so it's batched.

### Phase 4: Visual renovation (workstream F), after the design decision

1. **`DESIGN.md`:** intent, tokens (colour roles, type scale, spacing, radii, elevation, motion), component inventory, skin interaction rules. Starts from the chosen mockup.
2. **Theme module in the engine.** Tokens become ConVars or skin-overridable values. Text and vector shapes are crisp at any scale. Blurred background caches are rendered once per background change, never per frame.
3. **New default skin and UI assets:** original or OFL/CC0 only, attributions recorded. The mikosu logo is a branding decision for the user, with options shown as images.
4. **Screens, one at a time.** Each one goes through the QA loop (4 resolutions plus 2× scale, critique against `DESIGN.md`, fix, save to `screens/`), works keyboard-only, renders with the user's skins and three community skins, and stays within the menu frame-time budget. Order:
   1. song select (search syntax, grouping, detail panel with strain graph)
   2. main menu
   3. mod select (lazer mods with settings)
   4. options (searchable, every convar reachable)
   5. results (pp breakdown, hit-error histogram, UR, timing graph, comparisons)
   6. pause, fail and loading screens
   7. notifications
   8. first-run wizard
   9. downloads
   10. profile
   11. skin picker

**Milestone M4, "remastered":** the redesigned core screens, for the user to play-test.

### Phase 5: Official integration and downloads (workstream E)

1. **osu!api v2.**
   - Checked 2026-10-04: only the Authorization Code and Client Credentials grants are documented; there's no PKCE or device flow, and the limit is 60 requests per minute. So the default is a guided flow where the user registers their own OAuth app.
   - Tokens live in libsecret or Windows Credential Manager, with a user-only file as fallback.
   - A token-exchange relay is offered as a decision; hosting would be the user's.
2. **Using the API:** profile header, official leaderboard (`GET /beatmaps/{id}/scores` and whichever types are allowed), official best beside local best, ranked/loved status, beatmap search. All of it rate-limited and cached.
3. **Mirror downloads:**
   - a configurable mirror list, re-checked for liveness (checked 2026-10-04: catboy.best, nerinyan.moe and osu.direct respond; api.nerinyan.moe returned 530);
   - a queue, progress and auto-import;
   - each mirror's limits and terms respected.
4. **Discord Rich Presence** under a mikosu Discord application (the user creates it; free).

### Phase 6: Gameplay features (workstream G)

Storyboards (check upstream `sb` first), lazer mods (rate with a pitch option, Difficulty Adjust, Classic, Mirror, then popular fun mods), replay viewer (speed, judgement timeline, cursor options), offset calibration wizard, section-loop practice. Every experimental mod stays.

### Phase 7: Release (workstream H)

- **Packages:** an AppImage (Flatpak later), plus a Windows zip and an installer.
- **Versions and updates:** versioning, a changelog, and a disclosed update check that can be turned off.
- **Licensing:** `LICENSE`, `CREDITS.md`, `THIRD_PARTY_NOTICES`. Decide whether the default audio backend becomes SoLoud (open source) or stays BASS, from latency measurements.
- **README:** install, feature tour, parity guarantees and their limits, credits.
- **Publishing:** walk the user through the GitHub release.

### Side quest: private servers (after the core)

- Honest mikosu identification.
- Presets for servers that accept third-party clients. Research them and read each server's rules.
- Score submission only where a server explicitly allows mikosu.
- A list of servers that would need an admin opt-in.

## Decision log

| # | Date | Decision | Why |
|---|---|---|---|
| 1 | 2026-10-04 | Fork at upstream master `0fedcafb`, not tag v43.13 | CI green on all platforms; carries three more weeks of fixes; we cherry-pick from master anyway |
| 2 | 2026-10-04 | Reference checkouts live in `ref/` (git-ignored) inside the repo | Reachable from every session and worktree without extra permissions; never built or committed |
| 3 | 2026-10-04 | Oracle: osu-tools master `8ce45b3` built against ppy/osu `2026.921.0-lazer` (newest non-prerelease) via `UseLocalOsu.sh`; .NET 10 SDK from Ubuntu's archive | The brief pins the calculator to the newest release tag; the July 2026 rework is ppy/osu#37850 plus follow-ups #38253, #38219, #38200, #38199, #38754 |
| 4 | 2026-10-04 | Generated autotools files (`configure`, `Makefile.in`, `aclocal.m4`, `src/config.h.in`, `src/Makefile.sources`) are no longer tracked; `tools/build.sh` runs `autogen.sh` when inputs change | Distro autotools versions rewrote them on every run, burying real changes in diffs; upstream CI regenerates them anyway |
| 5 | 2026-10-04 | `tools/build.sh` uses gcc-14 through a PATH shim when the default compiler is older | `configure.ac` hard-codes `gcc`/`g++`; neomod needs GCC ≥ 14 (`<print>`). The shim holds compiler drivers **only** (see 6) |
| 6 | 2026-10-04 | Every heavy command runs under `tools/dev/guarded` (systemd scope with memory, task and priority caps) | A shim with `ar → gcc-ar-14` recursed forever and froze the PC twice (OOM fork bomb) |
| 7 | 2026-10-04 | Builds run with `LC_ALL=C.UTF-8` | perl (automake) intermittently segfaults at exit with this machine's `LC_NUMERIC=agr_PE` |
| 8 | 2026-10-04 | Windows cross-builds use llvm-mingw `20260908`, downloaded by `tools/build.sh` with a pinned SHA-256 | Same toolchain as upstream CI; no system packages needed |
| 9 | 2026-10-04 | `FrameStats` recorder (`-benchout`), measurement only and inert otherwise | The baseline needs per-frame CPU/GPU times, input-to-present latency, startup and memory; nothing like it existed |
| 10 | 2026-10-04 | SDL3 patch: offscreen display size from `SDL_VIDEO_OFFSCREEN_DISPLAY_SIZE` | SDL's offscreen display is fixed at 1024×768; headless screenshots and benchmarks need 1440p and up |
| 11 | 2026-10-04 | Keep internal code names (`namespace neomod`, `src/App/Neomod`, `NEOMOD_*` macros); rebrand only user-facing identity through the branding module | Renaming thousands of identifiers would break every upstream cherry-pick for no user-visible gain |
| 12 | 2026-10-04 | Benchmarks run headless with offscreen GL/Vulkan on the real GPU at 2560×1440, uncapped, median of 3 runs; on-screen latency is checked at play-test time | Reproducible, doesn't take over the user's screen, and measures CPU/GPU work at the user's resolution. Presentation adds compositor and vsync effects measured separately |
| 13 | 2026-10-04 | The build no longer rewrites tracked `.pot`/`.po` unless the set of strings changed | gettext 0.21 (Ubuntu 24.04) formats them differently and dirtied the tree on every clean build |
| 14 | 2026-10-04 | Commits are authored as `taizaki69` with the GitHub noreply address; pre-publication history was rewritten from a local placeholder before the first push, and personal paths were scrubbed from the docs | Never publish the user's personal email or machine details in a public repo |
| 15 | 2026-10-04 | Main branch `main`; feature branches; bootstrap docs and tooling committed straight to `main` (no remote yet) | Bootstrap work has no reviewer or CI to gate yet |
| 16 | 2026-10-04 | Mockups are HTML/CSS rendered to PNG with headless Firefox, using generated art and OFL fonts (Nunito, M PLUS Rounded 1c), and a placeholder logo | Fast to iterate on three directions; no copyrighted beatmap art; fonts are candidates for the real UI |
| 17 | 2026-10-04 | **User decision:** round-1 looks rejected as too old-style. New direction: stable layout with modern styling borrowed from lazer (slanted shapes, clean geometric font, flat dark panels); clean and calm | The user's words override the brief's "nothing like lazer" for styling; lazer's *layout* stays excluded |
| 18 | 2026-10-04 | **User decision:** the project is public on GitHub, at `taizaki69/mikosu`, as a standalone repository (not a GitHub fork) | Free CI for public repos; a standalone repo gives mikosu its own identity and releases. Attribution lives in `CREDITS.md`, `README.md` and the git history |
| 19 | 2026-10-04 | **User decision:** design direction is round-2 option 2, "Slanted controls"; tokens are in `DESIGN.md` | Chosen from three modern variants built on the user's picks (slanted shapes, Outfit-style font, flat dark panels, calm) |
| 20 | 2026-10-04 | Identity comes from `AC_INIT` (`mikosu`, `0.1`, the GitHub URL) and `src/Branding.h`; neomod's server-protocol names (endpoints, form fields, `neomod.net`) stay literal at their call sites. The version is shown as written ("0.1"), and neomod's config migrations only run for a `version.txt` from the neomod lineage (30 and up) | One place to change the name; protocol names are what servers implement, not who we are. neomod's migrations compare against 35-43, so mikosu's 0.x would have re-run them (and remapped keybinds) on every launch |
| 21 | 2026-10-05 | Every HTTP request sends mikosu's user agent, including private-server (Bancho protocol) requests, which neomod sent as `osu!`. Windows registrations use mikosu's own ProgID and `mikosu://` only | Ground rule 1: identify honestly, never pass a server's checks as another client. Servers that only accept `osu!` (bancho.py checks it) will refuse mikosu; supporting them means them allowing mikosu (private-server side quest) |

## Open questions for the user

See `STATUS.md` → "Waiting on you". Answers get recorded here as decisions.
