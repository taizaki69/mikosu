# mikosu status

*Last updated: 2026-10-06*

## Where things stand

The new look (round 6) is in the game: song select and the main menu now look like the mockups you approved, at your
screen's real size. **Dusk** is the default; **Stable, Moon and Day** are in Options → Skin → Theme, and **Classic** is the
old neomod look.

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
1. Song select: the cards, the rankings on the left, typing to search, the bottom buttons.
2. The main menu: the logo with the bars sliding out, the visualiser moving with the music, your card top left.
3. Options → Skin → Theme → Look: Dusk, Stable, Moon, Day (and Classic).
4. With your Aristia skin, song select's carousel panels, bottom buttons and back button come from the skin (as in stable).
   Should the new look win over your skin for those pieces? I'd add a setting for it; until you say otherwise the skin
   wins.

If something looks off, a screenshot plus a sentence is perfect.

## Done since last time

- **Round 5** (minimal) and **round 6** (bigger, with osu!'s feel), shaped by your feedback step by step: solid like stable,
  the map's art on the cards, the bars coming out from behind the logo, sharp rankings, smooth rounded cards.
- **Built into the game**, this time at the design's own sizes (round 4 had kept neomod's, which is why it looked off):
  - song select: the header with the map's details and the star rating, the rankings, the search, the cards, the
    bottom bar with stable's pink back button and your card;
  - main menu: the logo, the violet bars (pink when you point at one), your card, the music player, and the visualiser
    around the logo moving with the music, like stable's.
- Still light on the PC: under 1 ms per frame in song select at 2560x1440 (the limit I set for menus is 1.5 ms).
- neomod's latest work is merged.

## What's next

1. Merge the new look into the main version.
2. The other screens in the same look: mod select, options, results, then pause, fail and loading.
3. In parallel: spinners and the last score differences; exact star rating and pp.

## Things to try

The new look, above.
