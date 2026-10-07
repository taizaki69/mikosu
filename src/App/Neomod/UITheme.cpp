// Copyright (c) 2026, mikosu contributors, All rights reserved.
#include "UITheme.h"

#include "OsuConVars.h"

#include <algorithm>
#include <cmath>
#include <string_view>

namespace UITheme {
namespace {

// 0xRRGGBB plus an alpha
constexpr Color c(u32 rgb, f32 a = 1.f) {
    return argb(a, (int)((rgb >> 16) & 0xff), (int)((rgb >> 8) & 0xff), (int)(rgb & 0xff));
}

// the values are the mockups' (docs/renovation/mockups/src/round6.css)
constexpr Tokens DUSK{
    .id = Id::DUSK,
    .light = false,
    .ink = c(0xffffff, 0.97f),
    .ink2 = c(0xffffff, 0.74f),
    .ink3 = c(0xffffff, 0.52f),
    .ink4 = c(0xffffff, 0.3f),
    .hair = c(0xffffff, 0.12f),
    .hair2 = c(0xffffff, 0.24f),
    .scrim = c(0x0d0a1e),
    .dimSongSelect = 0.4f,
    .dimMenu = 0.5f,
    .band = 0.9f,
    .row = c(0x0d0a1e, 0.82f),
    .rowMe0 = c(0x76225c, 0.94f),
    .rowMe1 = c(0x0d0a1e, 0.88f),
    .rowMeEdge = c(0xff66aa, 0.55f),
    .field = c(0x181332, 0.75f),
    .pink = c(0xff66aa),
    .pink2 = c(0xff8fc4),
    .violet = c(0x7c5cf0),
    .violet2 = c(0x9d7bff),
    .sky = c(0x66ccff),
    .edge = {c(0xff66aa), c(0x9a72ff), c(0x66ccff)},
    .gold = c(0xffcf4a),
    .green = c(0x8ee36b),
    .blue = c(0x6cbcff),
    .orange = c(0xff9f5a),
    .red = c(0xff6b6b),
    .grey = c(0xb8b4cc),
    .setCard = c(0xcc4492),
    .setMark = c(0xff8cc6),
    .diffCard = c(0x2868d4),
    .groupCard = c(0x2c2566),
    .groupMark = c(0x9d7bff),
    .selCard = c(0xffffff),
    .cardInk = c(0xffffff),
    .cardInk2 = c(0xffffff, 0.8f),
    .selInk = c(0x141028, 0.95f),
    .selInk2 = c(0x141028, 0.66f),
    .selGlow = c(0xff66aa, 0.45f),
    .back = {c(0xff5aa5), c(0xff7cb8), c(0xff8fc4)},
    .footMarks = {c(0xff66aa), c(0xffcf4a), c(0x8ee36b), c(0x9d7bff)},
    .menuBar = {c(0x664ade, 0.92f), c(0x7e5cf0, 0.9f), c(0x926cfa, 0.86f)},
    .menuOn = {c(0xff5aa5), c(0xff74b4), c(0xff8fc4)},
    .logoRing = {c(0xff6fae), c(0x9a72ff), c(0x66ccff)},
    .logoDisc0 = c(0x2c2266),
    .logoDisc1 = c(0x140f2e),
};

// stable's own colours: black bands, its pink sets (235,73,153), blue difficulties (0,150,236) and navy groups
constexpr Tokens STABLE = [] {
    Tokens t = DUSK;
    t.id = Id::STABLE;
    t.scrim = c(0x000000);
    t.dimSongSelect = 0.45f;
    t.dimMenu = 0.45f;
    t.band = 0.85f;
    t.row = c(0x000000, 0.7f);
    t.rowMe0 = c(0x6e1c4c, 0.9f);
    t.rowMe1 = c(0x000000, 0.8f);
    t.field = c(0x000000, 0.65f);
    t.setCard = c(0xeb4999);
    t.setMark = c(0xffb3d9);
    t.diffCard = c(0x0096ec);
    t.groupCard = c(0x23328f);
    t.groupMark = c(0xa3f02c);
    t.edge = {c(0x0096ec), c(0x66ccff), c(0x0096ec)};
    t.menuBar = {c(0x6a46d8, 0.94f), c(0x8256f0, 0.92f), c(0x9a66ff, 0.9f)};
    t.logoRing = {c(0xff66aa), c(0xff66aa), c(0xff8fc4)};
    t.logoDisc0 = c(0x2a1a3a);
    t.logoDisc1 = c(0x120a1c);
    return t;
}();

// cooler: navy, sky and violet
constexpr Tokens MOON = [] {
    Tokens t = DUSK;
    t.id = Id::MOON;
    t.scrim = c(0x080c1a);
    t.row = c(0x080c1a, 0.82f);
    t.rowMe0 = c(0x1e3c78, 0.94f);
    t.rowMe1 = c(0x080c1a, 0.88f);
    t.rowMeEdge = c(0x66ccff, 0.55f);
    t.field = c(0x101830, 0.75f);
    t.setCard = c(0x5a4cc8);
    t.setMark = c(0xb9aaff);
    t.diffCard = c(0x1f78c8);
    t.groupCard = c(0x1a2652);
    t.selGlow = c(0x66ccff, 0.45f);
    t.edge = {c(0x66ccff), c(0xb9aaff), c(0x66ccff)};
    t.menuBar = {c(0x3a56c8, 0.92f), c(0x4a68dc, 0.9f), c(0x5a7aee, 0.86f)};
    t.menuOn = {c(0x4fb8ff), c(0x66ccff), c(0x8fdcff)};
    t.logoRing = {c(0x8fdcff), c(0xb9aaff), c(0x66ccff)};
    t.logoDisc0 = c(0x1c2a5a);
    t.logoDisc1 = c(0x0a1028);
    return t;
}();

// light: white bands and rows, dark text; the cards keep their colours, the selection turns dark
constexpr Tokens DAY = [] {
    Tokens t = DUSK;
    t.id = Id::DAY;
    t.light = true;
    t.ink = c(0x18142c, 0.95f);
    t.ink2 = c(0x18142c, 0.7f);
    t.ink3 = c(0x18142c, 0.5f);
    t.ink4 = c(0x18142c, 0.28f);
    t.hair = c(0x18142c, 0.12f);
    t.hair2 = c(0x18142c, 0.24f);
    t.scrim = c(0xf8f6ff);
    t.dimSongSelect = 0.35f;
    t.dimMenu = 0.35f;
    t.band = 0.9f;
    t.row = c(0xffffff, 0.85f);
    t.rowMe0 = c(0xffc4e0, 0.95f);
    t.rowMe1 = c(0xffffff, 0.9f);
    t.field = c(0xffffff, 0.75f);
    t.pink = c(0xf04f98);
    t.pink2 = c(0xff7ab6);
    t.violet = c(0x6a4fe0);
    t.violet2 = c(0x8a6cf5);
    t.gold = c(0xe0a400);
    t.green = c(0x3fae3a);
    t.blue = c(0x2f86e0);
    t.selCard = c(0x1e183a);
    t.selInk = c(0xffffff, 0.97f);
    t.selInk2 = c(0xffffff, 0.7f);
    t.logoDisc0 = c(0x3a2c7a);
    t.logoDisc1 = c(0x231a52);
    return t;
}();

constexpr Tokens CLASSIC = [] {
    Tokens t = DUSK;
    t.id = Id::CLASSIC;
    return t;
}();

}  // namespace

const Tokens &current() {
    const std::string_view name = cv::ui_theme.getString();
    if(name == "stable") return STABLE;
    if(name == "moon") return MOON;
    if(name == "day") return DAY;
    if(name == "classic") return CLASSIC;
    return DUSK;
}

Color mix(Color a, Color b, f32 t) {
    t = std::clamp(t, 0.f, 1.f);
    return argb(a.Af() + (b.Af() - a.Af()) * t, a.Rf() + (b.Rf() - a.Rf()) * t, a.Gf() + (b.Gf() - a.Gf()) * t,
                a.Bf() + (b.Bf() - a.Bf()) * t);
}

Color fade(Color col, f32 alpha) { return Color(col).setA(col.Af() * std::clamp(alpha, 0.f, 1.f)); }

Color starColour(f32 stars) {
    // ppy/osu OsuColour.STAR_DIFFICULTY_SPECTRUM (MIT)
    struct Stop {
        f32 at;
        u32 rgb;
    };
    static constexpr std::array<Stop, 12> SPECTRUM{{{0.1f, 0xaaaaaa},
                                                    {0.1f, 0x4290fb},
                                                    {1.25f, 0x4fc0ff},
                                                    {2.0f, 0x4fffd5},
                                                    {2.5f, 0x7cff4f},
                                                    {3.3f, 0xf6f05c},
                                                    {4.2f, 0xff8068},
                                                    {4.9f, 0xff4e6f},
                                                    {5.8f, 0xc645b8},
                                                    {6.7f, 0x6563de},
                                                    {7.7f, 0x18158e},
                                                    {9.0f, 0x000000}}};
    if(!(stars >= SPECTRUM[0].at)) return c(SPECTRUM[0].rgb);
    stars = std::round(stars * 100.f) / 100.f;
    for(size_t i = 1; i < SPECTRUM.size(); i++) {
        if(stars <= SPECTRUM[i].at) {
            const Stop a = SPECTRUM[i - 1], b = SPECTRUM[i];
            const f32 t = b.at > a.at ? (stars - a.at) / (b.at - a.at) : 1.f;
            return mix(c(a.rgb), c(b.rgb), t);
        }
    }
    return c(0x000000);
}

Color starTextColour(f32 stars) { return stars < 6.5f ? c(0x000000, 0.75f) : c(0xffd966); }

}  // namespace UITheme
