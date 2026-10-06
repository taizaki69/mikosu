#pragma once
// Copyright (c) 2026, mikosu contributors, All rights reserved.

// The redesign's themes (docs/renovation/DESIGN.md, round 4): Dusk (the default), Stable, Moon and Day share stable's
// layout and differ only in these tokens. "classic" keeps neomod's look while the redesign lands screen by screen.

#include "Color.h"
#include "types.h"

#include <array>

namespace UITheme {

enum class Id : u8 { CLASSIC, DUSK, STABLE, MOON, DAY };

struct Tokens {
    Id id;

    // text: primary, secondary, hints
    Color ink, ink2, ink3;

    // frosted glass tints (the alpha is the opacity over the blurred background) and hairlines
    Color bar, barEdge, row, rowMe, field, chip;

    // the thin glowing lines (header edge, bottom bar, selection underlines): three stops left to right
    std::array<Color, 3> line;
    Color lineGlow;

    // carousel panels, graded left to right (at 0, 55% and 100% of the width)
    std::array<Color, 3> set, diff, sel;
    Color panelInk, panelInk2, selInk, selInk2;
    Color selGlow;
    bool panelEdgeAccents;  // moon: colour as a thin left edge instead of a fill

    // stars: lit and dim, on normal panels and on the selected one
    Color starOn, starOff, selStarOn, selStarOff;

    // song select's back button (graded) and the markers above mode, mods, random and options
    std::array<Color, 2> back;
    Color backInk;
    std::array<Color, 4> footMarks;

    // main menu buttons (play, browse, options, exit) and the placeholder logo
    std::array<Color, 4> menuAccents;
    std::array<Color, 3> logoRing;
    Color logoWord;
    f32 logoEdgeAlpha;

    // multiplies the song select and menu backgrounds (1 = as drawn by the game, 0 = black)
    f32 bgBrightnessSongSelect, bgBrightnessMenu;
};

// the tokens for the ui_theme convar
const Tokens &current();

// true while the old look is selected
inline bool classic() { return current().id == Id::CLASSIC; }

// lerp between two colours, alpha included
Color mix(Color a, Color b, f32 t);

}  // namespace UITheme
