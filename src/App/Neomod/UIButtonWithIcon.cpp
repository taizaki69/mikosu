// Copyright (c) 2025, kiwec, All rights reserved.
#include "UIButtonWithIcon.h"
#include "CBaseUILabel.h"
#include "Graphics.h"
#include "Osu.h"
#include "UniString.h"
#include "UITheme.h"
#include "UIType.h"

UIButtonWithIcon::UIButtonWithIcon(std::string text, char32_t icon) : CBaseUIContainer(0, 0, 0, 0, ""), glyph(icon) {
    // the container itself is the button: opt back into hit candidacy; the labels are
    // decoration and must not win the single-target click over it
    this->bClickThroughSelf = false;

    this->icon = new CBaseUILabel(0, 0, 0, 0, "", UniString::to_utf8(std::u32string_view{&icon, 1}));
    this->icon->setDrawBackground(false);
    this->icon->setDrawFrame(false);
    this->icon->setDrawTextShadow(true);
    this->icon->setFont(osu->getFontIcons());
    this->icon->setHandleLeftMouse(false);
    this->addBaseUIElement(this->icon);

    this->text = new CBaseUILabel(0, 0, 0, 0, "", std::move(text));
    this->text->setDrawBackground(false);
    this->text->setDrawFrame(false);
    this->text->setDrawTextShadow(true);
    this->text->setHandleLeftMouse(false);
    this->addBaseUIElement(this->text);

    this->onResized();
}

void UIButtonWithIcon::draw() {
    if(!UITheme::classic()) {
        // the redesign: a small icon and the text in the soft ink, brighter while hovered
        if(!this->isVisible()) return;
        const auto& theme = UITheme::current();
        const Color ink = this->isMouseInside() ? theme.ink : theme.ink2;
        const f32 mid = this->getPos().y + this->getSize().y * 0.5f;
        const f32 iconW = UIType::px(19.f);
        UIType::icon(UIType::Style::ICON_19, this->glyph, {this->getPos().x + iconW * 0.5f, mid}, ink);
        UIType::drawCentredY(UIType::Style::NOTE, this->text->getText(), this->getPos().x + iconW + UIType::px(9.f),
                             mid, ink);
        return;
    }
    CBaseUIContainer::draw();

    // draw frame when hovered
    if(this->isMouseInside()) {
        g->drawRect(this->getPos(), this->getSize());
    }
}

void UIButtonWithIcon::onResized() {
    if(!UITheme::classic()) {
        this->setSize(UIType::px(19.f + 9.f) + UIType::width(UIType::Style::NOTE, this->text->getText()),
                      UIType::px(26.f));
        return;
    }
    this->icon->setFont(osu->getFontIcons());  // calls onResized()
    this->icon->setSizeToContent();
    this->text->onResized();
    this->text->setSizeToContent();

    const f32 dpiScale = Osu::getUIScale();
    const f32 inner_margin = 4.f * dpiScale;
    const f32 btn_width = this->icon->getSize().x + inner_margin + this->text->getSize().x;
    const f32 btn_height = std::max(this->icon->getSize().y, this->text->getSize().y);

    this->text->setRelPos(this->icon->getSize().x + inner_margin, btn_height / 2 - this->text->getSize().y / 2);

    this->setSize(btn_width, btn_height);
}
