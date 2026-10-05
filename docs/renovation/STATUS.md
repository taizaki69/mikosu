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
- **Speed:** checked side by side with neomod's code on this PC. mikosu is as fast or slightly faster everywhere: gameplay and song select.

**Problems found so far** (all on the plan)
- Star rating and pp are close to the official numbers but not exact yet.
- Spinners: mikosu counts spins a little differently from stable, so scores on maps with spinners are off by a few hundred points.
- Some of neomod's automated menu tests fail on this PC (they fail the same way on neomod itself, so it's not something I broke).
- Two fonts and some skin images that came with McOsu may not be free to share. They'll be replaced before any public release.

## Waiting on you

**The first play-test ("mikosu opens"), whenever you have 15 minutes.** The command is in my last chat message. It opens mikosu in a normal window. Please check:
1. It starts and shows the mikosu main menu.
2. Song select shows your osu!stable library, in the skin you use in stable, with your local scores on the maps you've played.
3. Play a few maps you know well. Does it feel the same as McOsu or stable (timing, cursor, sliders)? Anything that feels off is worth telling me, even if it's vague.
4. If you know them, try a few extras: FPoSu, the AR/OD overrides in mod select, scrubbing through a map, the console.

mikosu keeps its own files in `~/.local/share/mikosu`. It only reads your osu! folders and never changes them.

## Done since last time

- **Design chosen:** "Slanted controls".
- Automatic builds and tests on GitHub.
- The rename to mikosu, with honest identification everywhere.
- Your own data folder, plus the portable option.
- Replay checking, with the replay and score fixes above.
- Finds your Linux osu!stable copy, its settings and all of your scores on first launch.
- A screenshot tool that shows me every screen at your monitor's size and others, for the redesign.

## What's next

1. Your play-test feedback, then fixes for anything you find.
2. Then, in parallel:
   - spinners and the last few score differences;
   - exact star rating and pp;
   - start building the new look ("Slanted controls").

## Things to try

The play-test above.
