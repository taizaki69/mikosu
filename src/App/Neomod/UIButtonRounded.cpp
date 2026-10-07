// Copyright (c) 2026, WH, All rights reserved.
#include "UIButtonRounded.h"
#include "Font.h"
#include "Graphics.h"
#include "Icons.h"
#include "Osu.h"
#include "UIDraw.h"
#include "UIType.h"
#include "Engine.h"
#include "UITheme.h"
#include "UniString.h"

#include <algorithm>
#include <cmath>

UIButtonRounded::UIButtonRounded(float xPos, float yPos, float xSize, float ySize, std::string name, std::string text,
                                 int cornerRadius)
    : CBaseUIButton(xPos, yPos, xSize, ySize, std::move(name), std::move(text)), cornerRadius(cornerRadius) {}
UIButtonRounded::~UIButtonRounded() = default;

UIButtonRounded* UIButtonRounded::setCornerRadius(int radius) {
    this->cornerRadius = radius;
    return this;
}

UIButtonRounded* UIButtonRounded::setThemed(Themed style, bool dropdown) {
    this->themed = style;
    this->themedDropdown = dropdown;
    return this;
}

UIButtonRounded* UIButtonRounded::setThemedActive(bool active) {
    this->themedActive = active;
    return this;
}

UIButtonRounded* UIButtonRounded::setThemedAccent(size_t menuAccentIndex, char32_t glyph) {
    this->themedAccent = menuAccentIndex;
    this->themedGlyph = glyph;
    this->triangleSeed = (u32)menuAccentIndex * 7919u + 17u;
    return this;
}

UIButtonRounded* UIButtonRounded::setThemedInset(f32 hiddenLeft) {
    this->themedInset = hiddenLeft;
    return this;
}

void UIButtonRounded::onMouseInside() {
    CBaseUIButton::onMouseInside();
    if(this->themed == Themed::MENU) this->hoverAnim.set(1.f, 0.18f, anim::QuadOut);
}

void UIButtonRounded::onMouseOutside() {
    CBaseUIButton::onMouseOutside();
    if(this->themed == Themed::MENU) this->hoverAnim.set(0.f, 0.25f, anim::QuadOut);
}

namespace {
UIType::Style themedTextStyle(UIButtonRounded::Themed t) {
    return t == UIButtonRounded::Themed::TAB ? UIType::Style::SMALL_STRONG : UIType::Style::SMALL;
}
}  // namespace

f32 UIButtonRounded::getThemedWidth() const {
    const f32 text = UIType::width(themedTextStyle(this->themed), this->getText());
    return text + (this->themedDropdown ? UIType::px(6.f + 15.f) : 0.f);
}

void UIButtonRounded::draw() {
    if(this->themed == Themed::NONE || UITheme::classic() || this->font == nullptr) return CBaseUIButton::draw();
    if(!this->isVisible() || !this->isVisibleOnScreen()) return;

    const auto& theme = UITheme::current();
    const McRect r{this->getPos(), this->getSize()};
    const f32 h = r.getHeight();
    const bool hover = this->bEnabled && this->isMouseInside();
    const std::string_view text = this->getText();

    if(this->themed == Themed::MENU) {
        // stable's bars: slanted at both ends, violet, turning pink and sliding out while hovered; the left end hides
        // behind the logo. Fades in with the frame colour's alpha (the menu opening).
        const f32 fade = this->frameColor.Af();
        if(fade <= 0.f) return;
        const f32 lit = this->hoverAnim;
        const f32 push = lit * h * 0.38f;
        const McRect bar{r.getX() + push, r.getY(), r.getWidth(), h};
        const UIDraw::Shape shape{.rect = bar, .cut = UIDraw::slant(h), .cutLeft = UIDraw::slant(h), .opacity = fade};
        if(lit > 0.f) UIDraw::glow(shape, h * 0.3f, UITheme::fade(theme.selGlow, lit * fade));
        UIDraw::glow(shape, h * 0.12f, argb(0.25f * fade, 0.05f, 0.02f, 0.2f));
        UIDraw::fill(shape, UITheme::mix(theme.menuBar[0], theme.menuOn[0], lit),
                     UITheme::mix(theme.menuBar[1], theme.menuOn[1], lit),
                     UITheme::mix(theme.menuBar[2], theme.menuOn[2], lit), 0.6f);
        UIDraw::triangles(shape, this->triangleSeed, 7, h * 0.85f, std::max(1.f, h * 0.018f),
                          argb((0.09f + 0.09f * lit) * fade, 1.f, 1.f, 1.f), (f32)engine->getTime());

        const Color ink = argb(fade, 1.f, 1.f, 1.f);
        f32 x = bar.getX() + this->themedInset;
        const f32 mid = bar.getY() + h * 0.5f;
        if(this->themedGlyph != 0) {
            const f32 size = UIType::px(34.f);
            UIType::icon(UIType::Style::ICON_34, this->themedGlyph, {x + size * 0.5f, mid}, ink);
            x += size + UIType::px(22.f);
        }
        UIType::drawCentredY(UIType::Style::MENU, text, x, mid, ink);
        return;
    }

    const UIType::Style style = themedTextStyle(this->themed);
    const f32 mid = r.getY() + h * 0.5f;
    if(this->themed == Themed::TAB) {
        // a tab: the active one bright with osu!'s pink underline along the bottom of the button
        const Color ink = this->themedActive ? theme.ink : hover ? theme.ink2 : theme.ink3;
        UIType::drawCentredY(style, text, r.getX(), mid, ink);
        const f32 textW = UIType::width(style, text);
        if(this->themedDropdown) {
            UIType::icon(UIType::Style::ICON_19, Icons::ANGLE_DOWN, {r.getX() + textW + UIType::px(6.f + 7.5f), mid},
                         this->themedActive ? theme.ink2 : ink);
        }
        if(this->themedActive) {
            const f32 lineH = std::max(2.f, UIType::px(3.f));
            const McRect line{r.getX(), r.getY() + h - lineH, this->getThemedWidth(), lineH};
            UIDraw::glow(UIDraw::Shape{.rect = line}, UIType::px(6.f), UITheme::fade(theme.pink, 0.5f));
            UIDraw::fill(UIDraw::Shape{.rect = line}, theme.pink);
        }
        return;
    }

    // a link: text (and a chevron), brighter while hovered
    const Color ink = hover ? theme.ink : theme.ink2;
    UIType::drawCentredY(style, text, r.getX(), mid, ink);
    if(this->themedDropdown) {
        UIType::icon(UIType::Style::ICON_19, Icons::ANGLE_DOWN,
                     {r.getX() + UIType::width(style, text) + UIType::px(6.f + 7.5f), mid}, ink);
    }
}

void UIButtonRounded::drawBackground() {
    g->setColor(this->backgroundColor);
    g->fillRoundedRect((int)this->getPos().x + 1, (int)this->getPos().y + 1, (int)this->getSize().x - 1,
                       (int)this->getSize().y - 1, this->getRealCornerRadius() - 1);
}

void UIButtonRounded::drawFrame() {
    g->setColor(this->frameColor);
    g->drawRoundedRect(this->getPos(), this->getSize(), this->getRealCornerRadius());
}

void UIButtonRounded::drawHoverRect(int hoverRectOffset, bool isClickHeld) {
    // rounded buttons need about half the distance of square hover rects
    float distance = std::round((float)hoverRectOffset / 2.f);
    float thickness = 1.f;

    if(isClickHeld && distance > 1.f) {
        // thick outline close to the button for a "pressed" look;
        // band spans from ~button edge (distance - thickness/2) to just past hover distance
        distance = std::ceil(distance / 2.f);
        thickness = distance * 2.f + 1.f;
    }

    g->drawRectf(Graphics::RectOptions{
        .x = (float)(int)this->getPos().x - distance + 0.5f,
        .y = (float)(int)this->getPos().y - distance + 0.5f,
        .width = (float)(int)this->getSize().x + distance * 2.f,
        .height = (float)(int)this->getSize().y + distance * 2.f,
        .lineThickness = thickness,
        .cornerRadius = (float)this->getRealCornerRadius() + distance,
    });
}

// based on font dpi (more rounded for higher dpi)
// NOTE (@spec): this is a hack... obviously a button with no font (e.g. no text) shouldn't have different corner rounding!
// but i can't think of a better solution atm
int UIButtonRounded::getRealCornerRadius() const {
    if(!this->font) return this->cornerRadius;
    return (int)std::round((f32)this->cornerRadius * ((f32)this->font->getDPI() / 96.f));
}
