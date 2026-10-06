# mikosu design spec

> **Status: being revised (2026-10-05).** The user wasn't convinced by round 2's flat look and asked for "a modern, almost lazer like revamp ... keep the layout tho, i dont like the lazer layout". Round 3 (below) shows three treatments of that; the tokens further down are round 2's and get replaced by the round-3 pick.
> - **History:** round 1 rejected as "way too old style"; round 2 picked "Slanted controls" (2026-10-04); round 3 requested 2026-10-05.
> - **Still binding:** stable's layout and flow; lazer's layout stays out; nothing of ppy's ships (no Torus, no osu! logo or cookie, no osu-resources); Outfit is the UI font.

## Intent

osu!stable's layout, with a modern, clean look.
- **Same layout and flow as stable.** Every element sits where a stable player's hands and eyes expect it: song select with the carousel on the right, info top-left, leaderboard on the left, and the bottom bar; main menu with the logo and buttons sliding out of it; mod overlay; options sliding in from the left.
- **Modern styling, almost lazer's** (the user's calls: borrowed from lazer on 2026-10-04, "almost lazer like" on 2026-10-05):
  - rounded corners, sheared controls and wedges, dark panels tinted by one hue, soft shadows and glows;
  - lazer's colour language: star ratings, grades and mods coloured the way lazer colours them;
  - a crisp geometric sans-serif (Outfit, openly licensed; never lazer's Torus).
- **Readable at a glance.** Colour carries meaning (difficulty, grade, mod type); effects stay soft so a glance mid-session still works.
- **The beatmap's artwork is the hero.**
- **Lazer's *layout* stays out.** That means no top toolbar, no column-and-card mod select, no full-screen settings and no lazer-style results screen. The first brief's "nothing like lazer" rule is narrowed to layout by the user's later instruction.

## Principles (from the brief, binding)

1. **Readable mid-session at a glance.** Information hierarchy beats decoration.
2. **The background is the hero. Never blur per frame.** Blurred or translucent backdrops are rendered once per background change and cached.
3. **Animations:**
   - independent of frame rate (time-based) and interruptible;
   - navigation takes ≤ 200 ms;
   - input gets visible feedback on the next frame.
4. **Everything works from the keyboard,** with visible focus.
5. **Fonts are openly licensed,** with a CJK fallback.
6. **Music-reactive and playful:**
   - beat pulse on the logo and accents;
   - kiai flashes;
   - a sound for every interaction;
   - motion with a little overshoot.
7. **Skins win wherever stable lets a skin define something.** The new look is the default skin plus the UI chrome around it.
8. **Performance:** stays inside the menu budget in `BASELINE.md`.

## Directions shown to the user

### Round 1 (rejected 2026-10-04: "way too old style")

All three keep stable's layout (mockups: `mockups/{mainmenu,songselect}-{faithful,refined,bold}.jpg`; source in `mockups/src/`).

- **A. Faithful.** Stable as remembered, sharp and tidy:
  - stable's pink, blue and white panels;
  - solid coloured menu buttons;
  - a white visualizer;
  - the same information stable shows.
- **B. Refined** (recommended). Stable's layout with depth:
  - frosted panels carrying the map's art;
  - an accent colour taken from the background;
  - star rating after mods, pp at 95/98/99/100% and a strain graph in song select;
  - a glowing visualizer.
- **C. Bold.** The same layout turned up:
  - huge numbers and type;
  - gradient pill buttons;
  - light rays and particles;
  - neon selection.

### Round 2 (picked option 2 on 2026-10-04, then superseded by round 3)

All three keep stable's layout and use Outfit, flat dark panels and a calm visualizer. Mockups: `mockups/{mainmenu2,songselect2}-{all,controls,accents}.jpg`; source in `mockups/src/{modern.css,*2.html}`.

1. **Slanted everything:** every panel, button and leaderboard row slanted; one fixed accent.
2. **Slanted controls (chosen):**
   - buttons, tabs, dropdowns, search, map panels, the info wedge and the user card are slanted;
   - lists and long text (leaderboard rows, options rows) stay straight for reading;
   - the accent comes from the map's background.
3. **Slanted accents:** straight panels with slanted edges, underlined tabs and accent stripes.

### Round 3 (shown 2026-10-05, waiting for the pick)

Stable's layout styled almost exactly like today's lazer. Mockups: `mockups/{mainmenu3,songselect3}-{lazer,tinted,upright}.jpg`; source in `mockups/src/{round3.css,round3.js,*3.html}`. The values come from lazer's code (ppy/osu, MIT) at the pinned tag, scaled by 1.40625 (lazer's 1024×768 reference at 1080p):
- **Shape:** 14 px corner radius everywhere; a 0.2 shear (≈ 11.3°) on controls, wedges and leaderboard rows; the carousel's map panels stay upright and rounded, as in lazer.
- **Colour:** dark panels tinted by one hue (lazer's `OverlayColourProvider`: backgrounds at 10% saturation and 10–40% lightness, text at 40% saturation, a highlight at full saturation and 70% lightness). Info wedges fade out towards the screen centre.
- **Star ratings** use lazer's colour spectrum (blue → green → yellow → red → purple → black, yellow text from 6.5★): star pills, difficulty strips and dots on set panels, the CS/AR/OD/HP bars.
- **Grades and mods** use lazer's colours (S teal, A green, B amber…; difficulty-increase mods red, reduction lime).
- **Footer:** stable's bottom bar built from lazer's footer pieces: a magenta Back button, tiles with an icon, label and coloured indicator bar (mods lime, random blue, options purple), the selected mods with their multiplier, the user card, the play logo.
- **Main menu:** stable's buttons beside the logo, each in lazer's colour (play purple, browse lime, options grey, exit pink) with outlined triangles; lazer's visualiser (200 touching bars, the spectrum drawn five times around the logo).
- **Effects:** soft shadows, a glow in the difficulty's colour on the selected panel (pulsing with the beat), outlined triangles on difficulty panels and menu buttons, a blurred background in song select (rendered once per background, never per frame).

The three treatments:
1. **Lazer:** lazer's own blue-grey (hue 200) and light-blue highlight.
2. **Tinted:** the same, with the hue taken from the current map's background (plum for the test art), so every map recolours the panels and the highlight.
3. **Upright:** the same palette as 1 with no shear anywhere.

## Tokens (round 2's working set, to be replaced by the round-3 pick)

**Shape:**
- slant: `skewX(-11°)` (≈ 0.2 horizontal shear), with content counter-skewed so text stays upright;
- corners: sharp (radius 0);
- borders: 1 px hairline `line`.
- Slanted elements:
  - buttons (bottom bar, main menu, music controls);
  - tabs, dropdowns and the search field;
  - carousel panels (sets and difficulties);
  - the song-select info wedge, which bleeds off the left edge;
  - the user card and avatars.
- Straight elements:
  - leaderboard and score rows;
  - options rows;
  - dialogs;
  - text blocks;
  - gameplay HUD (skin-driven).

**Colour roles:**

| Role | Default | Use |
|---|---|---|
| `ink` / `ink-2` / `ink-3` | `#F3F5F9` / `#AAB0BE` / `#6F7686` | primary / secondary / hint text |
| `panel` | map-tinted dark, e.g. `rgba(22,15,26,.94)` (neutral fallback `rgba(17,19,26,.94)`) | flat panels |
| `panel-2` | `rgba(36,24,40,.97)` | selected or raised panel |
| `panel-3` | `#3A2A40` | button fill |
| `line` | white 7% | hairline borders |
| `accent` | from the map background (coral `#FF7D85` for the twilight test art; neutral fallback `#FF5C8A`) | selection bar, primary button fill, progress, focus |
| `accent-ink` | very dark version of the accent | text on accent fills |
| `info` | `#5AC8FF` | secondary highlight (rare) |
| `grade-*` | S `#FFCF5A`, A `#73E0A0`, B `#6FB8FF`, … (skin grade images win) | grades |
| `dim` | left-weighted dark gradient over the background (72% → 30%) | readability over art |

There's no glass, no blur, no glow and no gradients on panels; the background art is the only rich surface. Star ratings use the accent colour, not a difficulty spectrum (the user didn't pick colour-coding).

**Type.** **Outfit** (OFL, variable 100–900) for all UI text, with tabular figures for numbers. **M PLUS 1** (OFL, variable) as the CJK fallback.

| Token | Size / weight | Use |
|---|---|---|
| `display` | 46 / 700, −1.5% tracking | song-select title |
| `subhead` | 24 / 600, accent | difficulty name next to the title |
| `button` | 34 / 600 (main menu), 16 / 600 (bars) | buttons |
| `panel-title` | 22 / 600 | carousel titles |
| `body` | 18–19 / 400–600 | info lines, rows |
| `meta` | 14–15 / 400–500, `ink-3` | secondary lines |
| `label` | 13–14 / 600, uppercase, +0.1em | field labels (Group, Sort, CS/AR…) |

Sizes are reference pixels at 1080p and scale with resolution and UI scale; everything is drawn as vectors and text.

**Spacing:** 4-pt grid. Carousel gap 8. List gap 6. Bar padding 22. Screen margins 28–34.

**Elevation:** none (flat). Selection is shown by
- an accent bar (6 px, inset on the slanted left edge),
- a slightly lighter panel,
- for the focused control, a 2 px accent outline.

**Motion:**

| Token | Duration | Curve | Use |
|---|---|---|---|
| `instant` | next frame | — | press feedback |
| `quick` | 120 ms | out-cubic | hover and focus |
| `nav` | 180 ms | out-quart | screen and overlay transitions (≤ 200 ms rule) |
| `carousel` | spring (stiffness 520, damping 38) | — | scrolling and selection |
| `beat` | 80 ms attack, 260 ms decay | — | logo pulse; a subtle accent tick in the visualizer |

Calm: no particles, light rays or kiai flashes beyond a soft accent pulse.

**Background accent.** When the background changes, compute a small palette from the cached thumbnail (k-means on a few hundred pixels).
- `accent`: the most saturated mid-lightness colour, contrast-checked against `panel` to at least 3:1.
- `panel` tint: the darkest dominant hue at 6–10% saturation.
- Fallback to neutral for greyscale art.

## Component inventory (to be detailed per screen in Phase 4)

**Shared building blocks:**
- glass panel
- carousel panel (set, difficulty, selected)
- leaderboard row
- grade badge
- mod badge
- star-rating pill
- metric chips and meters (CS/AR/OD/HP after mods)
- pp row (95/98/99/SS)
- strain graph
- search field with syntax hints
- dropdown pill
- bottom bar button with key hint
- user card
- placeholder logo, later the real logo with beat pulse and visualizer ring
- main-menu button
- music controls
- notification toast
- tooltip
- options section and row with search highlighting
- mod tile (stable rows; lazer mods with settings)

**Results screen:** results panel, hit-error histogram, timing graph, pp breakdown.

## Skins and the new UI

- **The skin's own elements win everywhere stable lets a skin draw them:**
  - gameplay elements, cursor, hitsounds;
  - numbers, grades, mod icons;
  - song-select panel images (`menu-button-background`, `selection-*`), the back button, the score panel and the pause and fail screens.

  If a user skin provides these, they're used as stable would use them.
- **The new look is mikosu's default skin plus the UI chrome around skinnable elements:** layout, glass, type, motion, accent.
- **When a skin overrides a song-select element,** the frosted treatment steps back for that element (no glass behind a skinned panel) so the skin reads as intended.
- **skin.ini `[Colours]`** (`SongSelectActiveText`, `SongSelectInactiveText`, combo colours) are respected. A `[mikosu]` section may set UI tokens (accent override, glass opacity); neomod's `[neomod]` convar section keeps working.
- **QA:** every redesigned screen is checked with the user's skins (Rafis HDDT edits, WhiteCat, Aristia edit, …) and three popular community skins before it ships.

## Logo

The mockups use a placeholder: an approach ring closing on a hit circle, with the wordmark. It's deliberately unlike the osu! cookie: a ring rather than a filled disc, different colours, no "osu!" text. The real mikosu logo is a branding decision for the user, shown as options in Phase 4.
