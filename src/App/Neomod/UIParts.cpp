// Copyright (c) 2026, mikosu contributors, All rights reserved.
#include "UIParts.h"

#include "Icons.h"
#include "UIDraw.h"
#include "UITheme.h"
#include "score.h"

#include <array>
#include <cmath>
#include <string>

#include "fmt/format.h"

namespace UIParts {
namespace {

// the chip's height, star size and side padding (design px)
struct PillSize {
    f32 h, star, pad, gap;
    UIType::Style text;
};
constexpr PillSize PILL{30.f, 15.f, 14.f, 6.f, UIType::Style::SMALL_STRONG};
constexpr PillSize PILL_SMALL{26.f, 13.f, 12.f, 5.f, UIType::Style::FINE};

}  // namespace

f32 starPillWidth(f32 stars, bool small) {
    const PillSize p = small ? PILL_SMALL : PILL;
    const std::string value = fmt::format("{:.2f}", stars);
    return UIType::px(p.pad * 2.f + p.star + p.gap) + UIType::width(p.text, value);
}

f32 starPill(vec2 leftMid, f32 stars, bool small, f32 opacity) {
    const PillSize p = small ? PILL_SMALL : PILL;
    const std::string value = fmt::format("{:.2f}", stars);
    const f32 h = UIType::px(p.h);
    const f32 w = starPillWidth(stars, small);
    const McRect r{leftMid.x, leftMid.y - h * 0.5f, w, h};
    UIDraw::fill(UIDraw::Shape::slanted(r, UIType::px(6.f)), UITheme::fade(UITheme::starColour(stars), opacity));

    const Color ink = UITheme::fade(UITheme::starTextColour(stars), opacity);
    const f32 starX = r.getX() + UIType::px(p.pad) + UIType::px(p.star) * 0.5f;
    UIType::icon(UIType::Style::ICON_19, Icons::STAR, {starX, leftMid.y}, ink);
    UIType::drawCentredY(p.text, value, starX + UIType::px(p.star) * 0.5f + UIType::px(p.gap), leftMid.y, ink);
    return w;
}

f32 modPill(vec2 leftMid, std::string_view acronym, f32 opacity) {
    // lazer's colours by kind: easier (lime), harder (salmon), automation (sky), the rest violet
    static constexpr std::array<std::string_view, 3> EASIER{"EZ", "NF", "HT"};
    static constexpr std::array<std::string_view, 8> HARDER{"HR", "SD", "PF", "DT", "NC", "HD", "FL", "FI"};
    static constexpr std::array<std::string_view, 5> AUTO{"AT", "RX", "AP", "SO", "CN"};
    Color colour = argb(1.f, 0.55f, 0.4f, 1.f);
    for(auto m : EASIER)
        if(m == acronym) colour = argb(1.f, 0.7f, 1.f, 0.4f);
    for(auto m : HARDER)
        if(m == acronym) colour = argb(1.f, 1.f, 0.4f, 0.4f);
    for(auto m : AUTO)
        if(m == acronym) colour = argb(1.f, 0.4f, 0.8f, 1.f);

    const f32 h = UIType::px(22.f);
    const f32 w = UIType::px(20.f) + UIType::width(UIType::Style::TINY, acronym, 0.04f);
    const McRect r{leftMid.x, leftMid.y - h * 0.5f, w, h};
    UIDraw::fill(UIDraw::Shape::slanted(r, UIType::px(4.f)), UITheme::fade(colour, opacity));
    UIType::drawCentredY(UIType::Style::TINY, acronym, r.getX() + UIType::px(10.f), leftMid.y,
                         argb(0.78f * opacity, 0.f, 0.f, 0.f), 0.04f);
    return w;
}

void avatarTile(const McRect &r, std::string_view name, UIType::Style initial, f32 radius, f32 opacity) {
    // a pair of colours from the name, so each player keeps theirs
    static constexpr std::array<std::array<u32, 2>, 7> PAIRS{{{0xff9ccb, 0xa18bff},
                                                              {0x8ee3c8, 0x4f9dff},
                                                              {0xffcf7a, 0xff7a9c},
                                                              {0xb9a6ff, 0x6b7cff},
                                                              {0x7fe3ff, 0x7a8cff},
                                                              {0xffb38a, 0xd56bff},
                                                              {0xa5f08a, 0x3fbf9f}}};
    u32 hash = 2166136261u;
    for(char ch : name) hash = (hash ^ (u8)ch) * 16777619u;
    const auto &pair = PAIRS[hash % PAIRS.size()];
    auto col = [opacity](u32 rgb) {
        return argb(opacity, (int)((rgb >> 16) & 0xff), (int)((rgb >> 8) & 0xff), (int)(rgb & 0xff));
    };
    UIDraw::fill(UIDraw::Shape::rounded(r, radius), col(pair[0]), UITheme::mix(col(pair[0]), col(pair[1]), 0.5f),
                 col(pair[1]), 0.5f);

    std::string letter;
    if(!name.empty()) {
        // the first codepoint, upper-cased when it's ASCII
        size_t len = 1;
        const auto lead = (u8)name[0];
        if(lead >= 0xf0)
            len = 4;
        else if(lead >= 0xe0)
            len = 3;
        else if(lead >= 0xc0)
            len = 2;
        letter = std::string{name.substr(0, std::min(len, name.size()))};
        if(letter.size() == 1 && letter[0] >= 'a' && letter[0] <= 'z') letter[0] = (char)(letter[0] - 'a' + 'A');
    }
    const f32 w = UIType::width(initial, letter);
    UIType::drawCentredY(initial, letter, r.getCenter().x - w * 0.5f, r.getCenter().y, argb(opacity, 1.f, 1.f, 1.f));
}

Color gradeColour(ScoreGrade g) {
    const auto &theme = UITheme::current();
    switch(g) {
        case ScoreGrade::XH:
        case ScoreGrade::SH:
            // silver: light on dark rows, slate on light ones
            return theme.light ? argb(1.f, 0.42f, 0.47f, 0.58f) : argb(1.f, 0.86f, 0.9f, 0.98f);
        case ScoreGrade::X:
        case ScoreGrade::S:
            return theme.gold;
        case ScoreGrade::A:
            return theme.green;
        case ScoreGrade::B:
            return theme.blue;
        case ScoreGrade::C:
            return theme.orange;
        case ScoreGrade::D:
            return theme.red;
        default:
            return theme.grey;
    }
}

std::string_view gradeText(ScoreGrade g) {
    switch(g) {
        case ScoreGrade::XH:
        case ScoreGrade::X:
            return "SS";
        case ScoreGrade::SH:
        case ScoreGrade::S:
            return "S";
        case ScoreGrade::A:
            return "A";
        case ScoreGrade::B:
            return "B";
        case ScoreGrade::C:
            return "C";
        case ScoreGrade::D:
            return "D";
        case ScoreGrade::F:
            return "F";
        default:
            return "";
    }
}

}  // namespace UIParts
