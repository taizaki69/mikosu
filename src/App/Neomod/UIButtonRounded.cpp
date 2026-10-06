// Copyright (c) 2026, WH, All rights reserved.
#include "UIButtonRounded.h"
#include "Font.h"
#include "Graphics.h"
#include "Icons.h"
#include "Osu.h"
#include "UIDraw.h"
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

void UIButtonRounded::draw() {
    if(this->themed == Themed::NONE || UITheme::classic() || this->font == nullptr) return CBaseUIButton::draw();
    if(!this->isVisible() || !this->isVisibleOnScreen()) return;

    const auto& theme = UITheme::current();
    const McRect r{this->getPos(), this->getSize()};
    const f32 h = r.getHeight();
    const bool hover = this->bEnabled && this->isMouseInside();
    const std::string_view text = this->getText();
    const f32 scale = (h * 0.46f) / this->font->getHeight();
    const f32 textW = this->font->getStringWidth(text) * scale;
    const f32 baseline = r.getY() + h * 0.5f + this->font->getHeight() * scale * 0.36f;

    auto drawText = [&](f32 x, Color colour) {
        g->setColor(colour);
        g->pushTransform();
        {
            g->scale(scale, scale);
            g->translate((f32)(i32)x, (f32)(i32)baseline);
            g->drawString(this->font, text);
        }
        g->popTransform();
    };

    if(this->themed == Themed::FIELD) {
        const UIDraw::Shape shape = UIDraw::Shape::rounded(r, std::round(h * 0.3f));
        UIDraw::fill(shape, hover ? UITheme::mix(theme.field, Color(theme.ink).setA(theme.field.Af() * 1.8f), 0.5f)
                                  : theme.field);
        UIDraw::Shape border = shape;
        border.border = 1.f;
        UIDraw::fill(border, theme.barEdge);

        const f32 pad = h * 0.42f;
        if(this->themedDropdown) {
            McFont* icons = osu->getFontIcons();
            const std::string chevron = UniString::to_utf8(std::u32string(1, Icons::ANGLE_DOWN));
            const f32 iconScale = (h * 0.34f) / icons->getHeight();
            const f32 iconW = icons->getStringWidth(chevron) * iconScale;
            drawText(std::max(r.getX() + pad, r.getX() + (r.getWidth() - pad * 0.6f - iconW - textW) * 0.5f),
                     theme.ink);
            g->setColor(theme.ink3);
            g->pushTransform();
            {
                g->scale(iconScale, iconScale);
                g->translate((f32)(i32)(r.getX() + r.getWidth() - pad - iconW * 0.5f),
                             (f32)(i32)(r.getY() + h * 0.5f + icons->getHeight() * iconScale * 0.42f));
                g->drawString(icons, chevron);
            }
            g->popTransform();
        } else {
            drawText(r.getX() + (r.getWidth() - textW) * 0.5f, theme.ink);
        }
        return;
    }

    // tab: text only, the active one bright with a glowing underline
    const f32 x = r.getX() + (r.getWidth() - textW) * 0.5f;
    drawText(x, this->themedActive ? theme.ink : hover ? theme.ink2 : theme.ink3);
    if(this->themedActive) {
        const f32 y = r.getY() + h - std::max(2.f, h * 0.06f);
        const std::array<vec2, 2> underline{vec2{x, y}, vec2{x + textW, y}};
        UIDraw::glowLine(underline, theme.line, std::max(2.f, h * 0.08f),
                         Color(theme.lineGlow).setA(theme.lineGlow.Af() * 0.4f), h * 0.15f);
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
