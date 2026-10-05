# mikosu status

*Last updated: 2026-10-05*

## Where things stand

mikosu now has its own name and identity. Under the hood it is still neomod, the newest descendant of McOsu. The look hasn't changed yet: the redesign you picked ("Slanted controls") starts after the first play-test.

**What works today**
- The game builds on Linux and Windows with one command. GitHub also builds and tests both automatically every time something changes.
- It calls itself mikosu everywhere: the window, the files it writes, and what it tells servers. It never pretends to be the official osu! client or neomod. It has no default online server, and the self-updater is off.
- On first launch it finds your osu!stable copy in `~/.local/share/osu-stable` by itself. It brings over your stable skin, volume and **all 84 of your scores** (neomod only took the 54 that were submitted online), and shows your whole library in song select. Your osu! and McOsu folders were only read, never changed.
- It keeps its own files in `~/.local/share/mikosu` (or next to the program if you add a file called `portable` there).
- **Replay checking:** I can now play your stable replays back through mikosu's gameplay invisibly and compare the result with what stable recorded. That found and fixed two bugs in the replay player inherited from neomod:
  - it made circles too big, so replays came out better than they really were;
  - it sometimes skipped the last note.
- **Results on your 84 replays:**
  - **the judgements** (how many 300s, 100s, 50s and misses) now match stable exactly on 3 out of 4 replays (before: none);
  - **max combo** matches on 4 out of 5;
  - **total score** matches exactly on 36. That's every replay without a spinner whose judgements match, except 7. Spinners are next.
- The Windows version also runs under Wine on this PC.

**Problems found so far** (all on the plan)
- Star rating and pp are close to the official numbers but not exact yet.
- Spinners: mikosu counts spins a little differently from stable, so scores on maps with spinners are off by a few hundred points.
- Some of neomod's automated menu tests fail on this PC (they fail the same way on neomod itself, so it's not something I broke).
- Two fonts and some skin images that came with McOsu may not be free to share. They'll be replaced before any public release.

## Waiting on you

Nothing right now. When the first play-test build is ready, I'll ask you to try it. That's the next step after one more speed check, which I'll run when the PC isn't busy with a game.

## Done since last time

- **Design chosen:** "Slanted controls".
- Automatic builds and tests on GitHub.
- The rename to mikosu, with honest identification everywhere.
- Your own data folder, plus the portable option.
- Replay checking, with the replay and score fixes above.
- Finds your Linux osu!stable copy, its settings and all of your scores on first launch.
- A screenshot tool that shows me every screen at your monitor's size and others, for the redesign.

## What's next

1. A speed check against the numbers measured on day one (it has to wait for an idle PC: earlier you were playing a game, which makes the measurement meaningless).
2. **First play-test** ("mikosu opens"): you launch it, check it finds your library and that gameplay feels the same as before.
3. Then, in parallel:
   - spinners and the last few score differences;
   - exact star rating and pp;
   - start building the new look.

## Things to try

Nothing yet. The play-test build is coming next.
