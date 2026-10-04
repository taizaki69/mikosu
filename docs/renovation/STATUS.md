# mikosu status

*Last updated: 2026-10-04 (first session)*

## Where things stand

mikosu now exists as a project. It starts from neomod, the newest descendant of McOsu, and it builds and runs on this PC. Nothing looks different from neomod yet: this first session was about setting things up, measuring, and planning.

**What works today**
- The game builds on Linux with one command, and also builds for Windows from Linux.
- It found your osu!stable library: 7,514 difficulties, 13 skins, 172 replays. It found the Windows copy and the Linux copy in `~/.local/share/osu-stable`. It also found osu!lazer's data and McOsu.
- None of your osu! files were changed. Everything was read only.
- It can run invisibly in the background, play maps on autoplay, and take screenshots. That lets me check the redesign myself without bothering you.
- I measured how fast and responsive it is today (the "baseline"). From here on, every change gets checked against those numbers, so the game never gets slower without us noticing.
- The official osu! pp calculator runs on this PC, offline. I'll use it to check mikosu's star rating and pp.

**Problems found so far** (all on the plan)
- neomod's star rating and pp are close to the official numbers but not exact yet. The new "reading" part of the calculation is off the most.
- neomod imports only 54 of your 84 stable scores. It skips the ones that were never submitted online.
- neomod wouldn't find your Linux copy of osu! by itself, or a Windows drive, or osu!lazer.
- neomod logs in to private servers pretending to be the official osu! client. mikosu will always say it's mikosu.
- Two fonts and some skin images that came with McOsu may not be free to share. They'll be replaced before any public release.

**About the two freezes on 2026-10-04:** that was my fault. A build helper I set up kept starting copies of itself until your PC ran out of memory. It's fixed, and every heavy job now runs inside a box with hard limits, so it can't take your PC down again.

## Waiting on you

1. **Pick a design direction.** There are three mockups (faithful, refined, bold); I recommend refined. See the link in my message.
2. **GitHub.** Do you want the project on GitHub now? That's needed for automatic Windows and Linux test builds. Details are in my message.

## What's next

1. Rename everything to mikosu in one place: name, window title, where it saves data, how it identifies itself online. Remove neomod's update server and default online server.
2. Automatic test builds for Windows and Linux, plus a test that replays maps invisibly.
3. In parallel:
   - make star rating and pp match the official numbers exactly;
   - find your libraries automatically (Windows drive, Linux copy, lazer, McOsu);
   - start the redesign in the direction you pick.
4. First thing for you to play: a mikosu-branded build that opens straight into your library.

## Things to try

Nothing to try yet. The first play-test build comes at the first milestone.
