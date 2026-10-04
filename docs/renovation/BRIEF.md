# mikosu: project brief

> The brief the user gave on 2026-10-04, kept verbatim as the source of requirements. Decisions made since then are in `PLAN.md` (decision log); the current state is in `STATUS.md` and `PROGRESS.md`.

## Mission

You are the lead engineer and designer of **mikosu**, a new osu! client built by renovating **neomod**, the actively developed descendant of McOsu. When you're done, someone who plays osu!stable and osu!lazer can adopt mikosu as a third client without any friction:

- It finds their library, scores, replays, skins, collections and settings by itself, on Linux and on Windows.
- It calculates star rating, pp and score exactly the way the official clients do today.
- It shows official leaderboards and the player's profile, and downloads maps in-game.
- It looks like osu!stable, remastered: stable's layout and soul, made far prettier.

The person you work for is a player, not a programmer. They've asked you to work autonomously and come to them only for decisions; see "Working with the user" below.

The project will span many sessions. Work in phases, and keep the game building and playable on both platforms at every commit. Keep the repo documented well enough that a fresh session can pick up from the docs alone.

## Starting point

### neomod: the base

**neomod** (`neomodnet/neomod`, GPL-3.0; formerly neosu; by kiwec and whrvt) already has:
- autotools builds on Linux, MinGW cross-builds for Windows, and CMake + MSVC builds on Windows (`cmake-win/`), all with GitHub Actions CI;
- OpenGL, DX11 and SDL_GPU renderers;
- low-latency audio (their SoLoud fork, neoloud, plus BASS);
- importers for maps, collections, scores and settings;
- in-game downloads, a replay viewer, and CJK text;
- online play over the Bancho protocol on private servers, with a protocol documented for server operators.

Fork neomod at a pinned upstream commit, and keep an `upstream` remote.
- Review upstream at each milestone, and cherry-pick fixes worth having (engine, gameplay, importers, server protocol).
- Don't try to stay merge-compatible in areas you're redesigning.
- Log what you took and what you skipped in `docs/renovation/UPSTREAM.md`.

### McOsu: the ancestor

**McOsu** (`McKay42/McOsu`, GPL-3.0) and its engine **McEngine** (`McKay42/McEngine`, MIT, archived July 2026) are the ancestors. Use them as a reference for anything neomod changed or dropped.

### What players love, and you must preserve

- very low input latency and steady frame pacing at uncapped FPS;
- the practice tools: scrubbing, and live speed and AR/CS/OD/HP overrides;
- FPoSu;
- the experimental mods;
- the convar console;
- faithful rendering of stable skins.

### Ground truth references

- **`ppy/osu` (C#):**
  - `osu.Game.Rulesets.Osu/Difficulty/`: the difficulty and performance calculators, skills and evaluators.
  - `osu.Game.Rulesets.Osu/{Scoring,Judgements,Mods}/` and `osu.Game/Rulesets/Scoring/`.
  - `osu.Game/Scoring/Legacy/`: encoding and decoding `.osr` files, including the extra score data lazer appends.
- **`ppy/osu-tools`:** the `PerformanceCalculator` CLI is your oracle for star rating and pp.
- **osu! wiki:** the file-format pages for `.osu`, `.osb`, `.osr`, `osu!.db`, `collection.db`, `scores.db` and `skin.ini`, plus the osu!api v2 docs.
- **`rosu-pp` (Rust):** a cross-check.

### The user's machine

- **Dev machine:** Linux Mint 22.3 (Ubuntu 24.04 base), Cinnamon on X11, with an NVIDIA GeForce RTX 4070 SUPER. This is also the reference machine for performance numbers.
- **Windows side:** the same PC dual-boots Windows from the NVMe drive (`nvme0n1p3`, NTFS, not BitLocker-encrypted). The drive isn't mounted by default.
  - osu!stable and osu!lazer are installed on Windows.
  - From Linux, their data is by default under `<mount>/Users/<name>/AppData/Local/osu!` and `<mount>/Users/<name>/AppData/Roaming/osu`. Search for them if they aren't there.
- **McOsu:** installed through Steam at `~/.local/share/Steam/steamapps/common/McOsu`.
- **Tools:** when this brief was written, `gcc`, `make`, `python3` and `dotnet` were present. `git`, `cmake`, `clang`, autotools, `nasm`, `wine`, MinGW and `Xvfb` were missing. The user may have installed some since.

### Product decisions already made

- **Name:** mikosu, for a public release.
- **Platforms:** Linux and Windows, both first-class.
- **VR and Steam integration:** not wanted. Remove them if keeping them costs time.
- **Private servers:** wanted as a side feature, after the core work.
- **Look:** osu!stable's layout and feel, remastered. The user strongly dislikes lazer's styling.
- **Ruleset profiles:** all three (see workstream C).

## Ground rules

1. **Stay off the official servers.**
   - Never connect to Bancho or to official score submission.
   - Never spoof client versions or hashes.
   - Never use lazer's OAuth client credentials or the `lazer` API scope.
   - Identify honestly as mikosu to every server. Never pretend to be stable, lazer or neomod to get past a server's checks.

   ppy doesn't allow modified or third-party clients on the live servers, and it's the user's account that gets restricted. Official integration means read-only osu!api v2 access, at no more than 60 requests per minute. Secrets never go in the repo.
2. **Treat the user's osu! data as read-only.** That covers stable, lazer, McOsu, and anything on the Windows drive.
   - Any write-back, such as saving collections to stable's `collection.db`, is opt-in, atomic, and backed up first.
   - Never write into lazer's data directory.
   - Never write to an NTFS volume that Windows left hibernated (Fast Startup). If the drive is mounted read-only, work with that.
3. **Licensing and branding.**
   - Code stays GPL-3.0. Keep the copyright notices, and credit McKay (McOsu, McEngine) and neomod's authors in `CREDITS.md`.
   - Audit the provenance of every asset you ship: fonts, images, sounds and skins. The McOsu lineage may include files derived from osu!'s default skin. Replace anything that isn't clearly redistributable with original or openly licensed (OFL, CC0, CC-BY) work, and record the attribution.
   - Ship nothing of ppy's: no `osu-resources` files, no Torus font, and no osu! logo or cookie. mikosu gets its own logo.
   - Define the branding in one place: name, logo, window title, data-directory names and client identification. That keeps a later rename cheap.
4. **Gameplay feel comes first.** Before you change anything, measure a baseline:
   - CPU and GPU frame times (p50 and p99) on a dense reference map, and in song select with the user's full library;
   - input-to-present latency, as far as software can measure it;
   - startup time;
   - memory use.

   A gameplay p99 regression of more than 5% is a bug. So is any change to judgement timing outside the deliberate parity work below. Set a frame-time budget for menus from the baseline, and stay within it.
5. **Keep it shippable.** Main always builds and plays on Linux and Windows, and CI proves both on every push. Work on branches, in small, reviewable commits. Evolve the code instead of rewriting it. When you do rewrite a subsystem, explain in `PLAN.md` why a rewrite is smaller and safer.
6. **Never drop a feature players would notice without asking.** VR and Steam are already approved for removal.

## Workstreams

Each workstream has a goal and acceptance criteria. You sequence them and split them into phases in `PLAN.md`.

### A. Foundation, branding and tooling

- **Fork and rebrand.**
  - Fork neomod and set up the upstream policy described above.
  - Rebrand to mikosu everywhere, through the single branding definition.
  - Remove VR and Steam if they get in the way.
- **Builds.** Each platform builds with one documented command, and CI produces Linux and Windows artifacts on every push.
- **Tests.**
  - Add a unit-test framework and golden-file tests.
  - Add a **headless runner**. Given a beatmap, an `.osr` and a ruleset profile, it prints judgements, combo, score, star rating and pp as JSON. Most of the parity work below runs through it.
  - Add **headless screenshots**: render any screen to a PNG offscreen (for example with Xvfb and Mesa llvmpipe), so you can inspect UI work yourself.
- **Windows checks from Linux.**
  - Cross-compile with MinGW and smoke-test under Wine where that's practical. Rely on CI's Windows runners for the rest.
  - Real-Windows play-tests need the user to reboot, so batch them.
  - Get test builds to the Windows side the simplest safe way. That could be a dedicated mikosu folder on the Windows drive, if it's mounted read-write and not hibernated, or a GitHub pre-release once the repo exists.
- **Baseline.** Measure everything listed in ground rule 4, and record it in `docs/renovation/BASELINE.md`.

**Done when:**
- a clean clone builds with one command on each platform;
- the tests and the headless runner run in CI;
- the baseline is recorded.

### B. Star rating and pp

- **Check the starting point.** See what neomod already implements, and how far it is from current `ppy/osu`.
- **Port the calculators.** Port osu!standard difficulty and performance calculation from `ppy/osu`, pinned to the newest release tag when you start. The port must include the July 2026 rework and its follow-up fixes. Among other things, that rework added:
  - a Reading skill replacing the AR and HD bonuses;
  - harmonic speed summation;
  - variable-length strain chunks;
  - flow aim;
  - deviation-based speed scaling.

  Record the tag in the code.
- **Version the algorithm.**
  - Every result carries an algorithm id.
  - Several versions can coexist.
  - A future rework becomes a new version, not an in-place edit.
  - Stored scores can be recalculated under any version.
- **Use one calculator for two kinds of input.**
  - Lazer scores carry full statistics: slider tails, and large and small ticks, with or without the Classic mod.
  - Stable scores only record 300/100/50/miss counts and combo. The current calculator uses the ScoreV1 total to estimate what those hide, such as slider breaks, so port the legacy-score pieces it depends on.
- **Cover the full mod space:** arbitrary clock rates, Difficulty Adjust, and every ranked mod combination.
- **Calculate star rating in the background.** Use a persistent cache keyed by beatmap MD5, mods with their settings, rate, and algorithm version. Song select never waits on it, even with more than 100,000 difficulties.
- **Show it:**
  - live pp, and pp if FC, during play;
  - pp at 95/98/99/100% in song select;
  - a per-skill breakdown on the results screen;
  - a local profile whose total pp is weighted the same way osu-web weights it. Look up the current formula, including bonus pp.

**Done when** star rating matches osu-tools within 1e-4, and pp within 0.01, across a corpus of at least 200 beatmap/mod/rate cases. Choose the cases to cover edge cases:
- very long and very short maps;
- high-BPM streams;
- low AR and extreme CS;
- every common mod combination;
- custom rates and Difficulty Adjust;
- spinner-heavy and slider-heavy maps;
- aspire-style maps.

Never widen a tolerance or special-case an input to make a test pass. Find the cause.

### C. Scoring and judgements

Lazer pp only means something when gameplay produces lazer statistics, so judgements have to be faithful too. Every play runs under one of three **ruleset profiles**:

- **Stable:**
  - stable judgements: whole-slider judging, stable notelock and hit windows, slider leniency;
  - ScoreV1, with ScoreV2 available as a mod.
- **Lazer:**
  - lazer judgements: slider head accuracy, tail leniency, and lazer's hit policy;
  - standardised scoring, displayed as standardised or classic.
- **Lazer + Classic:** lazer with the Classic mod's behaviour.

Take hit windows, mod math (OD and AR under rate changes, HR and EZ caps) and score formulas from the source.

The local score database keeps everything a future recalculation needs:
- full statistics;
- mods with their settings;
- rate;
- profile;
- algorithm version;
- client build;
- the replay.

**Done when:**
- The headless runner reproduces hit counts, max combo and total score from two sources:
  - the user's own stable replays, in `Data/r` and `Replays`;
  - replays exported from lazer.

  Each `.osr` file carries the expected values.
- You aim for exact matches, report the match rate for each profile, and categorise every mismatch.

### D. Library and data: "the third official client"

- **Auto-detect everything on first run.**
  - On Linux, look for stable and lazer on mounted Windows (NTFS) drives. Scan `/media`, `/run/media` and `/mnt` for `Users/*/AppData/Local/osu!` and `Users/*/AppData/Roaming/osu`.
  - Also check Wine prefixes, for other users: osu-winello's defaults, `~/.wine`, Lutris and Bottles.
  - On Windows, check the standard install paths.
  - Check the custom lazer location recorded in lazer's `storage.ini`.
  - Find McOsu and neomod installs, including Steam's, so their scores, settings and collections can be imported too.
  - Fall back to a manual folder picker.

  If a Windows drive exists but isn't mounted, tell the user in plain words how to mount it.
- **Use stable's data in place, without copying it.** That means:
  - `Songs`;
  - `osu!.db`, in every format version found in the wild;
  - `collection.db` and `scores.db`;
  - replays and `Skins`;
  - `osu!.cfg` and `osu!.<user>.cfg`: map keybinds, sensitivity, raw input, offsets, volumes, dim and skin onto mikosu's settings.

  The library may live on an NTFS drive behind a slow driver, so lean on `osu!.db` and caches instead of rescanning thousands of files. Resolve file names case-insensitively, because maps often reference files with the wrong case. Pick up new maps while the game is running.
- **Optionally share mikosu's data across the dual boot.** Let mikosu keep its own data (scores, settings, caches) in a folder on a drive both operating systems can reach, so progress follows the user between Linux and Windows. Explain the Fast Startup caveat whenever it applies.
- **Time-box a lazer spike.** Try reading `client.realm` with realm-core's C API, opened from a temporary copy, together with lazer's hashed `files/` store. Cover beatmaps, scores, collections, skins, keybindings, and `game.ini` / `framework.ini`. Report the feasibility and the ongoing maintenance cost before building it out; lazer's schema changes often. The fallback is importing lazer's own exports.
- **Support the file formats.**
  - Import `.osz`, `.osk` and `.osr` files, by drag-and-drop or from the command line.
  - Export `.osr` files that both stable and lazer can open, including lazer's extra score data where it applies.
- **Keep stable's muscle memory.** Stable's default keybinds and habits work the way players expect: F1/F2/F3, quick retry, skip, the local offset keys, disabling mouse buttons, and random with rewind.
- **Integrate with the OS.**
  - On Linux: a `.desktop` entry, MIME types and file associations.
  - On Windows: the equivalents.
  - An optional `osu://` handler, off by default.

**Done when:**
- On this PC, first launch finds stable, lazer and McOsu, on Linux and on Windows.
- It shows the full library with collections and scores.
- Checksums of the user's osu! directories, taken before and after, prove nothing changed.

### E. Official integration, downloads, presence and private servers

- **Log in to osu!api v2.**
  - A public client can't safely ship an OAuth client secret, and the API docs don't document PKCE or a device flow. Check what osu! supports today.
  - The default is a guided first-run flow in which the user registers their own OAuth app in about a minute, with step-by-step instructions and links.
  - If a smoother option exists, such as a small token-exchange relay, present it to the user as a decision; the hosting and maintenance would fall on them.
  - Store tokens in libsecret or the Windows Credential Manager. Fall back to a file only the user can read.
- **Once logged in, show:**
  - a profile header with avatar, rank and pp;
  - the official leaderboard for the selected map, through `GET /beatmaps/{id}/scores`, with whatever leaderboard types the API allows;
  - the user's official best on the map, next to their local best;
  - ranked/loved status;
  - beatmap search.
- **In-game downloads.**
  - Download through a configurable list of community mirrors, for example catboy.best, nerinyan.moe and osu.direct. Check which ones are still alive.
  - Provide a queue, progress, and auto-import.
  - Respect each mirror's limits and terms.
  - The official `/beatmapsets/{id}/download` endpoint requires the lazer scope, so it's off-limits.
- **Discord Rich Presence.**
- **Private servers: a side quest, after the core workstreams.**
  - Ship built-in presets for the most popular private servers that accept third-party clients. Research which servers those are now, and read each one's rules.
  - Build on neomod's protocol work.
  - Enable score submission only where a server explicitly allows this client.
  - List the servers that would need an admin to opt in, so the user can contact them if they want.

### F. Visual and UX renovation: "osu!stable, remastered"

The user's brief: keep osu!stable's layout, flow and soul, but make it far prettier. They strongly dislike how lazer styled its menus, so nothing should feel like lazer.

**Stable's soul, which you keep:**
- **Main menu:** a big central logo that pulses to the music, with a visualizer around it. Clicking the logo slides out the menu buttons. mikosu needs an original logo to fill this role.
- **Song select:** the carousel of beatmap panels on the right, beatmap info at the top left, the leaderboard on the left, and the bottom bar with back, mode, mods, random and options.
- **Mod select:** the stable-style mod overlay, with mods grouped in rows and the score multiplier shown.
- **Options:** the panel that slides in from the left, with sectioned settings and a search box.
- **Results:** the big grade, score, judgement counts, accuracy and max combo, the performance graph, and replay and retry buttons.
- **Everywhere:**
  - snappy and music-reactive, with beat pulses and kiai flashes;
  - playful motion;
  - sound feedback for every interaction;
  - visuals that skins can drive.

**What "remastered" means:**
- Crisp at any resolution and UI scale. Stable stretches bitmaps, which blur at high resolutions.
- Refined typography and consistent spacing.
- Tasteful depth: soft shadows, glow, and cached translucency and blur.
- Smoother motion that feels native at high frame rates.
- An accent color taken from the current background, and a richer visualizer.
- More useful information where stable already shows information, such as pp and star rating after mods.

**Avoid:**
- lazer's visual language: sheared buttons and panels, triangle patterns, a column-and-card mod select, full-screen overlays, and its flat dark panel styling;
- the generic flat web-dashboard look;
- moving things away from where stable players expect them.

**Write the design spec first.** Put it in `docs/renovation/DESIGN.md`, covering:
- the intent;
- design tokens: color roles, type scale, spacing, radii, elevation, motion durations and curves;
- a component inventory;
- how user skins interact with the new UI.

Stable skins win wherever stable lets a skin define something: gameplay elements, hitsounds, cursor, numbers, grades and song-select elements. The new look is the default skin, plus the UI chrome around it.

**Mock it up before you build it.** Make 2–3 variations that all keep stable's layout but take the remaster to different lengths, for example faithful, richer and bold. Show the user images, explain each in one plain sentence, recommend one, and let them pick.

**Principles:**
- The information you need mid-session is readable at a glance.
- The beatmap background is the hero. Never blur per frame.
- Animations don't depend on frame rate, can be interrupted, and are short: navigation takes 200 ms or less. Input gets feedback on the next frame.
- Everything works from the keyboard.
- Fonts are openly licensed, with a CJK fallback.

**Screens to redesign:**
- **Song select:**
  - search syntax that works like stable's and lazer's;
  - grouping, sorting and filters;
  - a beatmap detail panel with a strain graph.
- **Mod select:**
  - lazer mods alongside stable's;
  - settings for each mod.
- **Settings:** searchable, with every convar reachable.
- **Results:**
  - the pp breakdown;
  - a hit-error histogram;
  - UR;
  - a timing graph;
  - comparison with the user's previous best and the official leaderboard.
- **Screens that are new or only lightly touched in stable:**
  - local and official profile;
  - downloads;
  - notifications;
  - the first-run wizard;
  - pause, fail and loading screens;
  - a skin picker with live preview.

**Run a visual QA loop for every screen:**
1. Render headless screenshots at 1280×720, 1920×1080, 2560×1440 and 3440×1440, plus a 2× UI-scale variant.
2. Look at each one and critique it against `DESIGN.md`.
3. Fix what's wrong.
4. Save the final screenshots in `docs/renovation/screens/`.

**Done when** every screen above:
- is redesigned;
- passes the QA loop;
- works keyboard-only;
- renders correctly with the user's own skins and 3 popular community skins;
- stays within the performance budget.

### G. Gameplay features

- **Storyboards and video.** Support storyboards (`.osb` files and storyboards inside maps) and background video. Check what's supported today, and fill the gaps without hurting frame times.
- **Lazer mods.** Start with the ones that affect ranked play and pp: rate adjust with custom rates and a pitch option, Difficulty Adjust, Classic and Mirror. Then add the popular fun mods. Keep every existing experimental mod.
- **Replay viewer:** seeking, speed control, a key overlay, a judgement timeline and cursor options.
- **Offsets:** an offset calibration wizard, plus per-beatmap offsets imported from stable.
- **Practice:** a section-loop practice mode built on the existing scrubbing.

**Out of scope unless the user says otherwise:**
- taiko, catch and mania;
- the beatmap editor;
- official multiplayer and spectating, which are impossible without the official servers;
- official chat, unless the API docs clearly allow it for user-authorized apps.

### H. Release

- **Packages.** CI builds them and publishes them as GitHub Releases.
  - Windows: a portable zip and an installer.
  - Linux: an AppImage, with Flatpak as a later option.
- **Versioning:** version numbers, a changelog, and an update check that's clearly disclosed and can be turned off.
- **License compliance:**
  - source availability;
  - `LICENSE`, `CREDITS.md` and `THIRD_PARTY_NOTICES`, including BASS's terms if you ship it.

  Consider making the open-source audio backend the default, if its latency is just as good.
- **Money:** anything that costs money, such as code signing, a domain or hosting, is the user's decision.
- **Publishing:** when it's time to publish, walk the user through creating the GitHub repo and logging in to `gh`. You can't create accounts or enter their passwords.

## Working with the user

- **Make technical decisions yourself.** Record each one, with its rationale, in the decision log in `PLAN.md`.
- **Go to the user only for:**
  - product and taste decisions: the design direction, feature priorities that involve real trade-offs, and branding;
  - anything that touches their osu! data or accounts;
  - anything that costs money or carries a ToS or legal risk;
  - things only they can do: typing their password (for example for `sudo`), registering the OAuth app, creating the GitHub repo, booting into Windows to test;
  - play-testing.
- **When you ask:**
  - Use plain language, with no jargon, and batch your questions.
  - For each question, give your recommendation and what you'll do if they don't answer.
  - Show visual decisions as images.
- **When they need to do something:**
  - Give them numbered, exact steps.
  - Give them one copy-paste command at a time, and say in one sentence what each command does.
  - Batch all system packages into a single `sudo apt install …` command.
- **Keep a plain-language status page** at `docs/renovation/STATUS.md`: what works now, what to try, what's next, and which decisions are waiting.
- **Hand over something to play at each milestone:** a build, plus a short "try these things" list. Their feedback on feel is the most valuable input they can give you.
- **Review your own work.** Nobody else will read the code. Before merging anything significant, review it with fresh eyes (for example with a review subagent), and keep the tests green.
- **Keep going.** Don't stop to report after small steps. Work through the plan until you reach a decision only the user can make, a play-test checkpoint, or a milestone.

## How to work

- **Follow the session protocol.**
  - Start each session by reading `CLAUDE.md`, `docs/renovation/PLAN.md`, `docs/renovation/PROGRESS.md` and the recent `git log`.
  - End each session by updating `PROGRESS.md` and committing. Record what changed, what's verified and how, what's next, and any open questions.
  - Keep machine-readable status in `docs/renovation/status.json`: parity pass rates, performance numbers, and the QA state of each screen.
- **Investigate before you change anything.** Read the code you're about to modify, and its callers. Don't make claims about code you haven't opened.
- **Verify; don't assume.** Back every claim in `PROGRESS.md` with a test, a measurement or a screenshot. Report failures plainly.
- **Finish before you start something new.** One thing completed and verified beats several half-done threads.
- **Run independent work in parallel.** Workstreams B, D and F can run at the same time, as subagents in separate git worktrees. Integrate often.
- **Match the code around you.** Leave the codebase more coherent than you found it, without gold-plating.

## Definition of done

- Workstreams A–H meet their acceptance criteria.
- The parity and performance numbers in `status.json` can be reproduced with one command.
- On a fresh Linux or Windows machine, a new user can install mikosu and, without configuring anything, have their stable and lazer library, scores, skins, collections and settings working within two minutes.
- A public release is out, with a README that covers:
  - installing;
  - a feature tour;
  - the parity guarantees, and what isn't guaranteed;
  - credits and licenses.

## Your first session

1. **Get the tools.** Check which build tools are still missing. Then ask the user to run one command that installs all of them, and tell them in one sentence what it does.
2. **Set up the repos.**
   - Create the mikosu repo in the working directory by cloning neomod at a pinned commit, and add the `upstream` remote.
   - Clone McOsu and McEngine alongside it for reference.
   - Build neomod as it is on Linux, and run it.
3. **Find the user's data:** the Windows drive, stable, lazer and McOsu. Don't write anything. If the drive needs mounting, give the user plain steps.
4. **Survey.**
   - Map the architecture in `docs/renovation/ARCHITECTURE.md`.
   - Measure the baseline.
   - Build a feature matrix of this brief against neomod, marking each item done, partial or missing.
5. **Write the docs:**
   - `PLAN.md`;
   - `CLAUDE.md`, with the build, run and test commands and the project's conventions;
   - `STATUS.md`.
6. **Get a design direction.** Make the design mockups, and ask the user to pick one, together with any other decisions you need. Then continue autonomously through the plan.
