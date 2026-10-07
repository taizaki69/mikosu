#pragma once
// Copyright (c) 2026, mikosu contributors, All rights reserved.

// The redesign's type (docs/renovation/DESIGN.md, round 6): Outfit at a fixed set of sizes and weights, laid out in
// design pixels (a 1920x1080 screen) and rasterised at the size it is drawn on this screen, so text stays sharp at any
// resolution instead of being scaled from one atlas.

#include "Color.h"
#include "Vectors.h"
#include "types.h"

#include <string>
#include <string_view>

class McFont;

namespace UIType {

enum class Style : u8 {
    LOGO,          // 97/600, the wordmark inside the main menu's logo ("mikosu" only)
    TITLE,         // 64/600, the map's title in song select
    MENU,          // 40/600, main menu buttons
    RANK,          // 34/700, grade letters and avatar initials
    BACK,          // 32/600, the back button
    NAME,          // 26/600, card titles and user names
    HEAD,          // 24/600, the difficulty line, the song in the player
    STRONG,        // 21/600, leaderboard names and scores
    BODY,          // 20/400, the search field, the mapper
    META,          // 19/400, map details, attribute values
    TAB,           // 18/600, tabs and difficulty names on cards
    ARTIST,        // 18/400, artist lines on cards
    SMALL_STRONG,  // 17/600, group tabs and star-rating pills
    SMALL,         // 17/400, sort labels, the tip
    NOTE,          // 16/400, user details, footer links
    CAPS,          // 15/700, labels in capitals (with tracking)
    FINE,          // 15/400, leaderboard details, counts
    LABEL,         // 14/700, CS/AR/OD/HP, times, the level
    TINY,          // 13/700, mod pills, keys
    // icons (Fork Awesome, Icons.h) at the sizes the design uses
    ICON_19,
    ICON_22,
    ICON_30,
    ICON_34,
    COUNT
};

// screen pixels per design pixel (the screen's height over 1080)
[[nodiscard]] f32 scale();
[[nodiscard]] inline f32 px(f32 design) { return design * scale(); }

// the font for a style, at this screen's size (reloaded when the resolution changes)
[[nodiscard]] McFont *font(Style s);

// the height of the tallest letters above the baseline, and of capitals (screen px)
[[nodiscard]] f32 ascent(Style s);
[[nodiscard]] f32 capHeight(Style s);

// width of the text (screen px); tracking is extra space after each character, in em
[[nodiscard]] f32 width(Style s, std::string_view text, f32 trackingEm = 0.f);

// the text shortened with an ellipsis to fit
[[nodiscard]] std::string fit(Style s, std::string_view text, f32 maxWidth);

// draws with the baseline's left end at `baseline` (screen px)
void draw(Style s, std::string_view text, vec2 baseline, Color colour, f32 trackingEm = 0.f);

// draws scaled by `scale` (for text that animates with its element), the baseline's left end at `baseline`
void drawScaled(Style s, std::string_view text, vec2 baseline, f32 scale, Color colour);

// draws with the vertical middle of the capitals at `y`
void drawCentredY(Style s, std::string_view text, f32 x, f32 y, Color colour, f32 trackingEm = 0.f);

// one icon (Icons.h) centred on `centre`
void icon(Style s, char32_t glyph, vec2 centre, Color colour);

// drop every font (shutdown); they're loaded again on use
void release();

}  // namespace UIType
