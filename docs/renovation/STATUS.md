# mikosu status

*Last updated: 2026-10-06*

## Where things stand

The new look you chose is going into the game. **Dusk** (between Moon and Day) is the default, and **Stable, Moon and Day** are themes you can switch to in Options → Skin → Theme. Song select and the main menu are done; the other screens follow one by one. I also merged neomod's 71 new commits (more on that below).

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

**Try the new look** when you next play (the commands are in my message). Things to check:
1. Song select and the main menu in Dusk. Do they feel like stable, just nicer?
2. Options → Skin → Theme → Look: switch between Dusk, Stable, Moon and Day (and Classic, the old neomod look).
3. With your Aristia skin, song select's carousel panels, bottom buttons and back button come from the skin (as in stable). Should the new look win over your skin for those pieces? I'd add a setting for that; for now the skin wins.

If something looks off, a screenshot plus a sentence is perfect.

## Done since last time

- **The redesign is chosen:** round 4 with Dusk as the default, after your feedback on rounds 3 and 4: no Frutiger Aero, sharper main-menu buttons, less see-through panels. Typing in song select still goes straight into the search.
- **Built into the game:**
  - **song select:** the frosted header with stable's stepped edge, the map info, the search, the carousel, the leaderboard, the bottom bar and your user card;
  - **main menu:** the flat logo, the four buttons, the music player.
  - It's light on the PC: about half a millisecond per frame in song select, well under the limit I set for the remastered menus.
- **neomod's new work is in:** 71 commits.
  - A more accurate .osu reader: exact timing points, plus several file-format fixes.
  - A rebuilt music clock.
  - Gameplay drawing and judging refactors.
  - I checked that nothing you'd feel changed: star ratings, your 84 replays and gameplay speed all came out exactly as before.
- Play-test fixes and the DT star-rating fix are merged.

## What's next

1. Finish merging (neomod first, then the new look).
2. The remaining screens in the new look: mod select, options, results, then pause, fail and loading screens.
3. In parallel: spinners and the last score differences; exact star rating and pp.

## Things to try

The new look, above.
