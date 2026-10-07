// Copyright (c) 2026, mikosu contributors, All rights reserved.
#include "UIType.h"

#include "Engine.h"
#include "Font.h"
#include "Icons.h"
#include "Graphics.h"
#include "Osu.h"
#include "ResourceManager.h"
#include "UniString.h"

#include <array>
#include <cmath>
#include <string>

namespace UIType {
namespace {

struct Spec {
    f32 size;    // design px
    i32 weight;  // Outfit's weight axis
};

// the sizes of docs/renovation/mockups/src/round6.css
constexpr std::array<Spec, (size_t)Style::COUNT> SPECS{{
    {97.f, 600},  // LOGO
    {64.f, 600},  // TITLE
    {40.f, 600},  // MENU
    {34.f, 700},  // RANK
    {32.f, 600},  // BACK
    {26.f, 600},  // NAME
    {24.f, 600},  // HEAD
    {21.f, 600},  // STRONG
    {20.f, 400},  // BODY
    {19.f, 400},  // META
    {18.f, 600},  // TAB
    {18.f, 400},  // ARTIST
    {17.f, 600},  // SMALL_STRONG
    {17.f, 400},  // SMALL
    {16.f, 400},  // NOTE
    {15.f, 700},  // CAPS
    {15.f, 400},  // FINE
    {14.f, 700},  // LABEL
    {13.f, 700},  // TINY
    {19.f, 0},    // ICON_19
    {22.f, 0},    // ICON_22
    {30.f, 0},    // ICON_30
    {34.f, 0},    // ICON_34
}};

bool isIcon(Style s) { return s >= Style::ICON_19 && s < Style::COUNT; }

constexpr std::array<char32_t, 6> LOGO_GLYPHS{U'm', U'i', U'k', U'o', U's', U'u'};

std::array<McFont *, (size_t)Style::COUNT> fonts{};
f32 loadedScale{0.f};

// whole pixels: hinting keeps small text crisp, and the sizes are within a pixel of the design
int pixelSize(Style s, f32 sc) { return std::max(6, (int)std::lround(SPECS[(size_t)s].size * sc)); }

void reloadAll(f32 sc) {
    for(size_t i = 0; i < fonts.size(); i++) {
        McFont *f = fonts[i];
        if(f == nullptr) continue;
        const int size = pixelSize((Style)i, sc);
        if(f->getSize() == size) continue;
        f->setSize(size);
        resourceManager->reloadResource(f);
    }
    loadedScale = sc;
}

}  // namespace

f32 scale() {
    const f32 h = osu != nullptr ? (f32)osu->getVirtScreenHeight() : engine->getScreenSize().y;
    return std::max(h, 1.f) / 1080.f;
}

McFont *font(Style s) {
    const f32 sc = scale();
    if(std::abs(sc - loadedScale) > 1e-4f) reloadAll(sc);

    McFont *&f = fonts[(size_t)s];
    if(f == nullptr) {
        const Spec spec = SPECS[(size_t)s];
        const std::string name = "FONT_UI_" + std::to_string((int)s);
        // DPI 72: the font size is in pixels
        if(isIcon(s))
            f = resourceManager->loadFont("forkawesome", name, Icons::icons, pixelSize(s, sc), true, 72);
        else if(s == Style::LOGO)
            f = resourceManager->loadFont("outfit@600", name, LOGO_GLYPHS, pixelSize(s, sc), true, 72);
        else
            f = resourceManager->loadFont("outfit@" + std::to_string(spec.weight), name, pixelSize(s, sc), true, 72);
        loadedScale = sc;
    }
    return f;
}

f32 ascent(Style s) { return font(s)->getHeight(); }

f32 capHeight(Style s) { return font(s)->getGlyphHeight(s == Style::LOGO ? U'k' : U'H'); }

f32 width(Style s, std::string_view text, f32 trackingEm) {
    McFont *f = font(s);
    f32 w = f->getStringWidth(text);
    if(trackingEm != 0.f && !text.empty()) {
        const f32 track = trackingEm * (f32)f->getSize();
        w += track * (f32)(UniString::num_codepoints(text) - 1);
    }
    return w;
}

std::string fit(Style s, std::string_view text, f32 maxWidth) {
    McFont *f = font(s);
    if(f->getStringWidth(text) <= maxWidth) return std::string{text};
    return f->ellipsize(text, maxWidth);
}

void draw(Style s, std::string_view text, vec2 baseline, Color colour, f32 trackingEm) {
    if(text.empty()) return;
    McFont *f = font(s);
    g->setColor(colour);
    if(trackingEm == 0.f) {
        g->pushTransform();
        g->translate(std::round(baseline.x), std::round(baseline.y));
        g->drawString(f, text);
        g->popTransform();
        return;
    }
    // letter by letter, with the extra space after each
    const f32 track = trackingEm * (f32)f->getSize();
    f32 x = baseline.x;
    for(size_t i = 0; i < text.size();) {
        const size_t next = UniString::next(text, i);
        const std::string_view ch = text.substr(i, next - i);
        g->pushTransform();
        g->translate(std::round(x), std::round(baseline.y));
        g->drawString(f, ch);
        g->popTransform();
        x += f->getStringWidth(ch) + track;
        i = next;
    }
}

void drawScaled(Style s, std::string_view text, vec2 baseline, f32 scale, Color colour) {
    if(text.empty()) return;
    g->setColor(colour);
    g->pushTransform();
    // scaled first, then moved into place (the engine applies the calls in order)
    g->scale(scale, scale);
    g->translate(baseline.x, baseline.y);
    g->drawString(font(s), text);
    g->popTransform();
}

void drawCentredY(Style s, std::string_view text, f32 x, f32 y, Color colour, f32 trackingEm) {
    draw(s, text, {x, y + capHeight(s) * 0.5f}, colour, trackingEm);
}

void icon(Style s, char32_t glyph, vec2 centre, Color colour) {
    McFont *f = font(s);
    std::string utf8;
    // UTF-8 of one codepoint in the BMP (the icons are all in the private use area)
    if(glyph < 0x80) {
        utf8.push_back((char)glyph);
    } else if(glyph < 0x800) {
        utf8.push_back((char)(0xc0 | (glyph >> 6)));
        utf8.push_back((char)(0x80 | (glyph & 0x3f)));
    } else {
        utf8.push_back((char)(0xe0 | (glyph >> 12)));
        utf8.push_back((char)(0x80 | ((glyph >> 6) & 0x3f)));
        utf8.push_back((char)(0x80 | (glyph & 0x3f)));
    }
    // the glyph's bitmap spans [baseline - top, baseline - top + rows]
    const f32 w = f->getGlyphWidth(glyph);
    const f32 top = f->getGlyphHeight(glyph), rows = f->getGlyphRows(glyph);
    draw(s, utf8, {centre.x - w * 0.5f, centre.y + top - rows * 0.5f}, colour);
}

void release() {
    for(McFont *&f : fonts) {
        if(f != nullptr) resourceManager->destroyResource(f);
        f = nullptr;
    }
    loadedScale = 0.f;
}

}  // namespace UIType
