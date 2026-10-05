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

**A quick re-test of the four things you reported**, whenever you play next (same command as before; rebuild not needed, it's already built):
1. Keyboard in the menus: arrows, Enter, Escape, F1 and typing in song select should all respond every time now.
2. Alt-tab out of fullscreen and back: it should come back fullscreen, at your custom resolution, every time.
3. Options → Graphics → Select Resolution → "Custom...": type any resolution, like 1600x900.
4. Options → Graphics → FPS Limiter: now goes up to 4000.

If something still misbehaves, tell me what you did and what happened.

## Done since last time

- **First play-test:** you found gameplay the same as neomod.
- **Fixed from your report:**
  - Menu keys were going missing because of the Linux input-method system (IBus). mikosu now skips it unless you turn it on (Options → Input → Keyboard; only needed for typing Japanese, Chinese and similar). Accented letters (´ + e = é) now work too.
  - Alt-tab out of fullscreen used to get stuck or lose your resolution. It now just switches windows and comes back as it was.
  - A "Custom..." resolution option in-game, no file editing.
  - FPS limiters up to 4000.

## What's next

1. Your play-test feedback, then fixes for anything you find.
2. Then, in parallel:
   - spinners and the last few score differences;
   - exact star rating and pp;
   - start building the new look ("Slanted controls").

## Things to try

The play-test above.
