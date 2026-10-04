# Feature matrix: the brief vs neomod

Surveyed at upstream `0fedcafb` (2026-10-04) by reading the code, running the Linux build headless, and testing against the user's real library. Status:

- **done**: meets the brief's requirement as it stands
- **partial**: something exists but falls short of the brief
- **missing**: nothing exists

"Evidence" points at the code or the check behind each status. Update this table as workstreams land; PLAN.md tracks the work.

## A. Foundation, branding, tooling

| Item | Status | Evidence / gap |
|---|---|---|
| Linux build (autotools) | done | built here with gcc-14 + LTO; `tools/build.sh linux` |
| Windows cross-build (MinGW) | done | llvm-mingw 20260908, the same as upstream CI; `tools/build.sh windows` |
| Windows MSVC build | done | `cmake-win/` (upstream; not exercised here) |
| CI producing Linux and Windows artifacts | partial | upstream workflows exist but target `master`, a private Docker image and neomod names |
| One-command build | partial | upstream needed several manual steps and distro quirks (gcc ≥ 14, locale-dependent automake crash, missing `libattr1-dev`). Now `tools/build.sh` |
| Unit-test framework | partial | in-binary test apps (`-testapp`, `src/App/Tests`), scripted UI tests (`tests/ui`), a sanity smoke test. No plain unit-test runner for pure logic |
| Golden-file tests | partial | `tools/diffcalc` goldens (star/pp; recorded on arm64 macOS, so exact compare fails on x86 by design) |
| Headless runner (beatmap + `.osr` + profile → JSON) | missing | `SimulatedBeatmapInterface` can replay a score without drawing; there's no CLI or JSON |
| Headless screenshots | partial | `-headless` + `take_screenshot` work, but SDL's offscreen display was stuck at 1024×768. **Fixed** with `build-aux/misc/SDL3-offscreen-display-size.patch` + `-w/-h` |
| Single branding definition | missing | `PACKAGE_NAME`, `NEOMOD_DOMAIN` (`BanchoNetworking.h`), SDL app metadata (`main.cpp`) and asset names are scattered |
| VR and Steam removed | done | no OpenVR left. Steam code is only `ACFParser`, used to find McOsu (keep it) |
| Baseline measured | done | `docs/renovation/BASELINE.md` |

## B. Star rating and pp

| Item | Status | Evidence / gap |
|---|---|---|
| Port of lazer's osu! calculator | partial | `DiffCalc/DifficultyCalculator.cpp`, `PP_ALGORITHM_VERSION 20260811`, includes the 2026 Q2 rework (Reading skill etc.). Measured against the osu-tools oracle at `2026.921.0-lazer`: SR 6.004272 vs 6.004027 (2.4e-4), reading 0.82918 vs 0.82292 (6e-3) on fixture 2785319. **Outside the 1e-4 target** |
| Follow-up fix ppy/osu#38754 (`LegacyTotalScore == null` checks) | missing | not in neomod's port (validated against 2026.730.0) |
| Arbitrary rates | partial | supported, but neomod documents a ~2e-3 median error at 1.5× (object times truncated to integer ms) |
| Algorithm id on every result; several versions side by side; recalc under any version | partial | one global `PP_ALGORITHM_VERSION`; stored scores don't carry an algorithm id |
| Lazer full-statistics input (slider tails, ticks, Classic) | missing | `FinishedScore` holds stable counts only |
| Stable scores via ScoreV1 estimation (legacy-score pieces) | partial | attributes like `maximumLegacyComboScore` and `nestedScorePerObject` exist; the legacy path needs checking against the oracle |
| Full mod space (DA, every ranked combo) | partial | McOsu-style AR/OD/CS/HP overrides and a speed override. No lazer mod settings model |
| Background SR with a persistent cache keyed by md5 + mods + rate + version | partial | `StarPrecalc`/`BatchDiffCalc` store **nomod** stars in the maps DB. There's no mod/rate/version-keyed cache |
| Live pp and pp-if-FC during play | partial | `LivePPCalc` gives live pp (the HUD scoreboard shows it). pp-if-FC still needs checking |
| pp at 95/98/99/100% in song select | missing | song select shows only SS pp ("Stars:2.28 (22pp)") |
| Per-skill breakdown on results | missing | — |
| Local profile weighted like osu-web, with bonus pp | partial | `Database::calculatePlayerStats`: 0.95ⁱ weighting + `(417 − 1/3)(1 − 0.995^min(n,1000))` bonus. Still to check against osu-web's current rules (score cap, best-per-beatmap) |

## C. Scoring and judgements

| Item | Status | Evidence / gap |
|---|---|---|
| Stable profile (notelock, hit windows, slider leniency, ScoreV1/V2) | partial | McOsu-heritage stable-like judgements. ScoreV2 exists as a mod. Exactness against stable replays is unmeasured |
| Lazer profile (slider head accuracy, tail leniency, hit policy, standardised scoring) | missing | `ModFlags::SliderHeadAccuracy`/`SliderTailAccuracy` reserved but unused |
| Lazer + Classic profile | missing | — |
| Score DB with full stats, mod settings, rate, profile, algo version, client build, replay | partial | stable stats, mods, speed and replay; no profile or algorithm id |
| Replay reproduction tests (stable `Data/r` and lazer exports) | missing | 172 stable replays (86 `.osr`) available on this PC as test data |

## D. Library and data

| Item | Status | Evidence / gap |
|---|---|---|
| Auto-detect stable on Linux | partial | checks `~/.local/share/{osu!,osu-wine,osu}` only. Misses this user's `~/.local/share/osu-stable`, NTFS mounts and Wine prefixes |
| Auto-detect stable on Windows | done | registry + `%LOCALAPPDATA%\osu!` |
| Detect lazer (incl. `storage.ini`) | missing | — |
| Detect McOsu/neomod installs | partial | McOsu import via Steam `.acf` (`SettingsImporter`) |
| Tell the user how to mount an unmounted Windows drive | missing | — |
| Use `Songs`/`osu!.db` in place | done | read in place; tested with 7,514 diffs (version 20260711) |
| `osu!.db` older format versions | partial | handles the date-based format changes it knows about; not tested on old dbs |
| `collection.db` | done | imported (read-only; neomod keeps its own collections db) |
| `scores.db` | partial | **imports only scores with an online id**: 54 of this user's 84 local scores, 30 skipped |
| Replays and Skins from the stable folder | partial | skins: yes. Replays: stable `Data/r` replays open for imported scores |
| `osu!.cfg` / `osu!.<user>.cfg` mapping | partial | volumes, songs dir and some chat options. Finds the cfg by the **OS user name** instead of the stable username (they differ on this PC). Keybinds, sensitivity, raw input, offsets, dim and skin aren't mapped |
| Slow-NTFS friendliness, case-insensitive file lookup | partial | the DB is cached in its own maps db; case-insensitive lookup still to check |
| Pick up new maps while running | partial | `DirectoryWatcher` + reconcile for its own `maps/`; not for the stable `Songs` folder |
| Shared data folder across the dual boot | missing | — |
| lazer `client.realm` reader | missing | spike planned |
| Import `.osz`/`.osk`/`.osr` (drag and drop, CLI) | partial | `.osz`/`.osk`: yes. `.osr`: opens a replay. Linux file associations: missing |
| Export `.osr` readable by stable and lazer | partial | writes `.osr` with version 40000000 (neomod-specific). Compatibility unverified |
| Stable muscle memory (F1/F2/F3, quick retry, skip, offset keys, disable mouse buttons, random rewind) | done | `OsuKeyBinds`, `SongBrowser` (F1/F2/F3/F5, previous-random history) |
| `.desktop`, MIME types, `osu://` handler | missing (Linux) / partial (Windows) | Windows registry associations exist (`NeomodEnvInterop`); nothing for Linux |

## E. Official integration, downloads, presence, private servers

| Item | Status | Evidence / gap |
|---|---|---|
| osu!api v2 login (own OAuth app, keyring storage) | missing | `Bancho*` is private-server protocol only |
| Profile header, official leaderboard, official best, ranked status, search | missing | leaderboards come from private servers only |
| In-game downloads via community mirrors | partial | `Downloader`/`MapFetcher`/`OsuDirectScreen` (catboy.best, osu.direct-style API). Mirror list, limits and terms to review |
| Discord Rich Presence | done | `RichPresence` + discord-rpc |
| Private servers | partial | full Bancho-protocol client, but it **identifies as osu! stable `b20260711.1`** with spoofed client hashes. That has to change to honest mikosu identification |

## F. Visual and UX

| Item | Status | Evidence / gap |
|---|---|---|
| Stable layout (main menu, song select, mod select, options, results) | done | McOsu-style screens (see the `docs/renovation/screens/before-*` captures) |
| Remastered look (crisp, tokens, depth, motion) | missing | design spec and mockups are the next step |
| Options search, every convar reachable | partial | options search exists. The console reaches every convar; options doesn't show them all |
| Song select search syntax | partial | `ar cs od hp star(s) bpm length objects circles sliders spinners artist creator`; no `played/unplayed/status/keys` or lazer syntax |
| Grouping and sorting | done | group: none/artist/difficulty/collections…; sort: date/artist/… |
| Strain graph | partial | `StrainGraph` exists (HUD and song browser) |
| Results: hit-error, UR, timing graph, pp breakdown, comparison | partial | hit-error and UR shown; no breakdown or comparison |
| Notifications, pause/fail/loading screens | done | stable-like |
| First-run wizard | missing | — |
| Skin picker with live preview | missing | dropdown in options only |
| Keyboard-only navigation everywhere | partial | song select and menus are keyboard-friendly; options and mod select need an audit |
| Fonts openly licensed with CJK fallback | partial | system CJK fallback works. **`tahoma.woff2` (Microsoft) and `weblysleekuisb.woff2` (license unknown) need replacing** |

## G. Gameplay features

| Item | Status | Evidence / gap |
|---|---|---|
| Storyboards | missing | upstream branch `sb` is work in progress |
| Background video | done | `FFmpegInterop`, `draw_video` |
| Lazer mods: rate adjust with custom rates and pitch option | partial | speed override + `NoPitchCorrection`; no lazer mod model |
| Difficulty Adjust | partial | AR/OD/CS/HP overrides (McOsu experimental) |
| Classic, Mirror | missing / partial | Mirror: `MirrorHorizontal`/`MirrorVertical` experimental mods. Classic: missing |
| Experimental mods kept | done | about 30 McOsu experimental mods in `ModFlags.h` |
| Replay viewer: seeking, speed, key overlay, timeline, cursor options | partial | seeking via scrubbing binds; key overlay and input trail exist. No speed control or judgement timeline yet |
| Offset calibration wizard | missing | — |
| Per-beatmap offsets imported from stable | done | `osu!.db` local offsets are read (`Database.cpp` around line 1775) |
| Section-loop practice | partial | quick save/load points (F6/F7) and scrubbing. No loop |
| FPoSu, scrubbing, live overrides, convar console | done | McOsu features intact |

## H. Release

| Item | Status | Evidence / gap |
|---|---|---|
| Windows zip and installer, Linux AppImage | missing (for mikosu) | upstream ships zips and tarballs; no installer and no AppImage |
| Versioning, changelog, opt-out update check | partial | neomod's self-updater points at `neomod.net`. It has to move to GitHub Releases, be disclosed, and allow opting out |
| LICENSE, CREDITS, THIRD_PARTY_NOTICES (incl. BASS) | partial | `assets/misc/licenses.txt` and `credits.txt` exist. BASS is proprietary and SoLoud (neoloud) is open source; the default backend is undecided |
| Asset provenance audit | missing | `assets/materials/default/` (260 skin files of McOsu lineage), fonts above, icons |
