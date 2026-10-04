# mikosu design spec

> **Status: draft.** Waiting on the user to pick a direction (mockups in `mockups/`; decision page: https://claude.ai/artifact/WPKRiAu87RZV2pKhaAQncj). The tokens below are drafted from direction **B, Refined** (recommended) and get finalised once the user picks.

## Intent

osu!stable, remastered.
- **Same layout, flow and soul.** Every element sits where a stable player's hands and eyes expect it.
- **Prettier.** Crisp at any resolution, calmer typography, tasteful depth, and motion that feels native at 240 Hz and above.
- **The beatmap's artwork is the hero.** The UI frames it and borrows its colours instead of covering it.
- **Nothing should feel like lazer.**
  - No sheared buttons or panels.
  - No triangle patterns.
  - No column-and-card mod select.
  - No full-screen overlays.
  - No flat dark panel styling.
- **No generic web-dashboard look either.**

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

## Tokens (draft, from B)

**Colour roles.** Values are defaults. `accent` and `accent-2` are recomputed per background (see "Background accent").

| Role | Default | Use |
|---|---|---|
| `ink` | `#FFFFFF` | primary text on art |
| `ink-2` | white 74% | secondary text |
| `ink-3` | white 50% | hints, labels |
| `glass` | `rgba(22,14,36,.46)` over the cached blur of the background | panels |
| `glass-edge` | white 12% | 1 px panel border |
| `shade` | `rgba(10,6,20,.28)` + vignette | dim over the background |
| `accent` | `#FF7F9E` | selection, focus, primary action |
| `accent-2` | `#FFB36B` | gradient partner, progress |
| `grade-x/s/a/b/c/d` | skin grade images first; fallback gradients | grades |
| `mod-*` | per-mod colours (HD gold, HR red, DT violet, …) | mod badges, when the skin has no mod images |
| `sr-*` | difficulty colour ramp by star rating | star-rating pills |

**Type.** Nunito (OFL; variable weight 200–1000) for UI text and numbers, with tabular figures for scores. M PLUS Rounded 1c (OFL) as the CJK fallback, which matches Nunito's rounded terminals.

| Token | Size / weight | Use |
|---|---|---|
| `display` | 44 / 800 | beatmap title in song select |
| `title` | 25 / 800 | panel titles, menu buttons (40 / 900) |
| `body` | 17–21 / 600–700 | rows, info |
| `label` | 14–16 / 800, +0.08em tracking, uppercase | section labels, key hints |
| `caption` | 15 / 600 | secondary lines |

Sizes are in reference pixels at 1080p and scale with UI scale and resolution, because the UI renders as vectors and text, not stretched bitmaps.

**Spacing:** 4-pt base: 4, 8, 12, 16, 20, 24, 32, 40.

**Radii:**
- 8: badges
- 12: rows
- 14: carousel panels
- 18: bars and buttons
- 999: pills

**Elevation:**
- 0: on background
- 1: glass panel (shadow `0 10 30 rgba(6,2,14,.35)`)
- 2: selected (accent outline + glow `0 0 34 accent@45%`)
- 3: overlays (mod select, options)

**Motion:**

| Token | Duration | Curve | Use |
|---|---|---|---|
| `instant` | 0 ms, next frame | — | press feedback |
| `quick` | 120 ms | out-cubic | hover, focus, small state |
| `nav` | 180 ms | out-quart, 4% overshoot | screen and overlay transitions (≤ 200 ms rule) |
| `carousel` | spring (stiffness 520, damping 38) | — | panel scrolling and selection |
| `beat` | per beat, 80 ms attack and 260 ms decay | — | logo pulse, accent breathing |
| `kiai` | 120 ms flash, then decay over the beat | — | kiai sections |

**Background accent.** When the background changes, compute a small palette from the cached thumbnail (k-means on a few hundred pixels). Pick the most saturated mid-lightness colour as `accent`, and its warmer or cooler neighbour as `accent-2`. Clamp the contrast against `glass` to at least 3:1 for UI accents. Fall back to the defaults for greyscale art.

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
