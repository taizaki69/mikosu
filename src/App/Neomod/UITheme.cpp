// Copyright (c) 2026, mikosu contributors, All rights reserved.
#include "UITheme.h"

#include "OsuConVars.h"

#include <algorithm>
#include <string_view>

namespace UITheme {
namespace {

// 0xRRGGBB plus an alpha
constexpr Color c(u32 rgb, f32 a = 1.f) {
    return argb(a, (int)((rgb >> 16) & 0xff), (int)((rgb >> 8) & 0xff), (int)(rgb & 0xff));
}

// the values are the mockups' (docs/renovation/mockups/src/round4.css)
constexpr Tokens DUSK{
    .id = Id::DUSK,
    .ink = c(0xffffff),
    .ink2 = c(0xffffff, 0.84f),
    .ink3 = c(0xf0ecff, 0.64f),
    .bar = c(0x2e3262, 0.8f),
    .barEdge = c(0xffffff, 0.28f),
    .row = c(0x323668, 0.78f),
    .rowMe = c(0x70428c, 0.86f),
    .field = c(0xffffff, 0.16f),
    .chip = c(0xffffff, 0.16f),
    .line = {c(0xffa8d8), c(0xc9b2ff), c(0x9fe0ff)},
    .lineGlow = c(0xffbeeb, 0.85f),
    .set = {c(0xf082c0, 0.92f), c(0x9668c0, 0.86f), c(0x464682, 0.76f)},
    .diff = {c(0x64a8f5, 0.92f), c(0x5c70c8, 0.86f), c(0x3c4682, 0.76f)},
    .sel = {c(0xffffff, 1.f), c(0xffffff, 0.97f), c(0xffffff, 0.88f)},
    .panelInk = c(0xffffff),
    .panelInk2 = c(0xffffff, 0.86f),
    .selInk = c(0x232a4a),
    .selInk2 = c(0x5a6386),
    .selGlow = c(0xffb4e6, 0.55f),
    .panelEdgeAccents = false,
    .starOn = c(0xffffff),
    .starOff = c(0xffffff, 0.3f),
    .selStarOn = c(0xff7cbc),
    .selStarOff = c(0x232a4a, 0.16f),
    .back = {c(0xff9fd0), c(0xc3a2ff)},
    .backInk = c(0xffffff),
    .footMarks = {c(0xff9fd0), c(0xff9a9a), c(0x8ff0b8), c(0x8fd4ff)},
    .menuAccents = {c(0xff9fd0), c(0x8fd4ff), c(0xc9b2ff), c(0xffb09a)},
    .logoRing = {c(0xffa8d8), c(0xc9b2ff), c(0x9fe0ff)},
    .logoWord = c(0xffffff),
    .logoEdgeAlpha = 0.55f,
    .bgBrightnessSongSelect = 0.86f,
    .bgBrightnessMenu = 0.92f,
};

constexpr Tokens STABLE{
    .id = Id::STABLE,
    .ink = c(0xffffff),
    .ink2 = c(0xffffff, 0.8f),
    .ink3 = c(0xffffff, 0.58f),
    .bar = c(0x101428, 0.8f),
    .barEdge = c(0xffffff, 0.14f),
    .row = c(0x101428, 0.78f),
    .rowMe = c(0x781e50, 0.62f),
    .field = c(0xffffff, 0.1f),
    .chip = c(0xffffff, 0.14f),
    .line = {c(0xff66aa), c(0xc58cff), c(0x66ccff)},
    .lineGlow = c(0xff8cd2, 0.75f),
    .set = {c(0xff5ca5, 0.97f), c(0xf6529c, 0.88f), c(0xf6529c, 0.7f)},
    .diff = {c(0x3a9cff, 0.97f), c(0x348af0, 0.88f), c(0x348af0, 0.7f)},
    .sel = {c(0xffffff, 1.f), c(0xffffff, 0.97f), c(0xffffff, 0.88f)},
    .panelInk = c(0xffffff),
    .panelInk2 = c(0xffffff, 0.84f),
    .selInk = c(0x1c2236),
    .selInk2 = c(0x4a5470),
    .selGlow = c(0xffffff, 0.55f),
    .panelEdgeAccents = false,
    .starOn = c(0xffffff),
    .starOff = c(0xffffff, 0.32f),
    .selStarOn = c(0xffb02e),
    .selStarOff = c(0x1c2236, 0.18f),
    .back = {c(0xff5ea8), c(0xff3f8e)},
    .backInk = c(0xffffff),
    .footMarks = {c(0xff66aa), c(0xff5b6e), c(0x5fdc7a), c(0x4fb4ff)},
    .menuAccents = {c(0xff66aa), c(0x4fb4ff), c(0xa0a8d0), c(0xff7a86)},
    .logoRing = {c(0xff66aa), c(0xc58cff), c(0x66ccff)},
    .logoWord = c(0xffffff),
    .logoEdgeAlpha = 0.45f,
    .bgBrightnessSongSelect = 0.78f,
    .bgBrightnessMenu = 0.92f,
};

constexpr Tokens MOON{
    .id = Id::MOON,
    .ink = c(0xf2f6ff),
    .ink2 = c(0xe2ecff, 0.78f),
    .ink3 = c(0xcddcfa, 0.55f),
    .bar = c(0x0e162c, 0.86f),
    .barEdge = c(0xaad7ff, 0.16f),
    .row = c(0x101a32, 0.82f),
    .rowMe = c(0x1e3c6e, 0.78f),
    .field = c(0xaad7ff, 0.08f),
    .chip = c(0xaad7ff, 0.12f),
    .line = {c(0x7fd0ff), c(0xffffff), c(0x7fd0ff)},
    .lineGlow = c(0x8cd7ff, 0.85f),
    .set = {c(0x141e3a, 0.96f), c(0x141e3a, 0.88f), c(0x141e3a, 0.72f)},
    .diff = {c(0x1e325a, 0.96f), c(0x1e325a, 0.88f), c(0x1e325a, 0.72f)},
    .sel = {c(0xecf5ff, 1.f), c(0xecf5ff, 0.96f), c(0xecf5ff, 0.86f)},
    .panelInk = c(0xffffff),
    .panelInk2 = c(0xffffff, 0.84f),
    .selInk = c(0x13203c),
    .selInk2 = c(0x4c5f86),
    .selGlow = c(0x8cd7ff, 0.55f),
    .panelEdgeAccents = true,
    .starOn = c(0xcfeeff),
    .starOff = c(0xc8e1ff, 0.25f),
    .selStarOn = c(0x2f8fe8),
    .selStarOff = c(0x13203c, 0.16f),
    .back = {c(0x14203c, 0.85f), c(0x14203c, 0.55f)},
    .backInk = c(0xf2f6ff),
    .footMarks = {c(0xff8fc4), c(0xffb36b), c(0x8ff0b0), c(0x7fd0ff)},
    .menuAccents = {c(0xff8fc4), c(0x7fd0ff), c(0xc9d4f0), c(0xffa08a)},
    .logoRing = {c(0xa8e6ff), c(0xffffff), c(0xa8e6ff)},
    .logoWord = c(0xf2f6ff),
    .logoEdgeAlpha = 0.4f,
    .bgBrightnessSongSelect = 0.58f,
    .bgBrightnessMenu = 0.66f,
};

constexpr Tokens DAY{
    .id = Id::DAY,
    .ink = c(0x1f2b45),
    .ink2 = c(0x1f2b45, 0.78f),
    .ink3 = c(0x1f2b45, 0.55f),
    .bar = c(0xffffff, 0.82f),
    .barEdge = c(0xffffff, 0.75f),
    .row = c(0xffffff, 0.8f),
    .rowMe = c(0xffd6ea, 0.82f),
    .field = c(0xffffff, 0.75f),
    .chip = c(0x1f2b45, 0.08f),
    .line = {c(0xff9fd0), c(0xc7a8ff), c(0x86d8ff)},
    .lineGlow = c(0xffaadc, 0.9f),
    .set = {c(0xffc4e2, 0.98f), c(0xffe2f1, 0.92f), c(0xffffff, 0.8f)},
    .diff = {c(0xb2deff, 0.98f), c(0xdaf0ff, 0.92f), c(0xffffff, 0.8f)},
    .sel = {c(0xffffff, 1.f), c(0xffffff, 0.95f), c(0xffffff, 0.8f)},
    .panelInk = c(0x1f2b45),
    .panelInk2 = c(0x1f2b45, 0.72f),
    .selInk = c(0x1a2440),
    .selInk2 = c(0x55617d),
    .selGlow = c(0xff96cd, 0.55f),
    .panelEdgeAccents = false,
    .starOn = c(0xff7cbc),
    .starOff = c(0x1f2b45, 0.18f),
    .selStarOn = c(0xff7cbc),
    .selStarOff = c(0x1f2b45, 0.16f),
    .back = {c(0xff8cc6), c(0xff6fb4)},
    .backInk = c(0xffffff),
    .footMarks = {c(0xff7cbc), c(0xff8a8a), c(0x5fd69a), c(0x4cb8f5)},
    .menuAccents = {c(0xff7cbc), c(0x4cb8f5), c(0x9aa6c8), c(0xff8a7a)},
    .logoRing = {c(0xff9fd0), c(0xc7a8ff), c(0x86d8ff)},
    .logoWord = c(0x2a3555),
    .logoEdgeAlpha = 0.9f,
    .bgBrightnessSongSelect = 1.f,
    .bgBrightnessMenu = 1.f,
};

// classic: only the id matters (the old drawing code ignores the tokens); the rest is dusk's so nothing is blank
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

}  // namespace UITheme
