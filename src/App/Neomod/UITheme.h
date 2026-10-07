#pragma once
// Copyright (c) 2026, mikosu contributors, All rights reserved.

// The redesign's themes (docs/renovation/DESIGN.md, round 6): Dusk (the default), Stable, Moon and Day share the layout
// and differ only in these tokens. "classic" keeps neomod's look.

#include "Color.h"
#include "types.h"

#include <array>

namespace UITheme {

enum class Id : u8 { CLASSIC, DUSK, STABLE, MOON, DAY };

struct Tokens {
    Id id;
    bool light;  // dark text on light surfaces (Day)

    // text, from primary to faint, and hairlines
    Color ink, ink2, ink3, ink4;
    Color hair, hair2;

    // the dim over backgrounds and the solid bands (song select's header and footer): the colour, and how strongly
    Color scrim;
    f32 dimSongSelect, dimMenu, band;

    // surfaces: leaderboard rows (the personal best graded from the first colour), the search field
    Color row, rowMe0, rowMe1, rowMeEdge;
    Color field;

    // accents: osu!'s pink and its neighbours; the header's edge line runs pink, violet, sky
    Color pink, pink2, violet, violet2, sky;
    std::array<Color, 3> edge;

    // grades
    Color gold, green, blue, orange, red, grey;

    // carousel cards: solid on the left, the map's art showing through on the right (stable's pink sets, blue
    // difficulties and white selection)
    Color setCard, setMark, diffCard, groupCard, groupMark, selCard;
    Color cardInk, cardInk2;  // text on sets and difficulties
    Color selInk, selInk2;    // text on the selected card
    Color selGlow;

    // song select's footer: the back button (graded) and the bars over mode, mods, random and options
    std::array<Color, 3> back;
    std::array<Color, 4> footMarks;

    // main menu: the bars (graded) and the hovered one; the logo's ring and disc (centre, edge)
    std::array<Color, 3> menuBar, menuOn;
    std::array<Color, 3> logoRing;
    Color logoDisc0, logoDisc1;
};

// the tokens for the ui_theme convar
const Tokens &current();

// true while the old look is selected
inline bool classic() { return current().id == Id::CLASSIC; }

// lerp between two colours, alpha included
Color mix(Color a, Color b, f32 t);

// the colour with its alpha multiplied
Color fade(Color c, f32 alpha);

// lazer's star-rating spectrum (ppy/osu OsuColour, MIT), and the text colour on it
Color starColour(f32 stars);
Color starTextColour(f32 stars);

}  // namespace UITheme
