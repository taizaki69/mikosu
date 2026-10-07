// Copyright (c) 2016, PG, All rights reserved.
#include "UIBackButton.h"

#include <utility>

#include "AnimationHandler.h"
#include "OsuConVars.h"
#include "Engine.h"
#include "OptionsOverlay.h"
#include "Osu.h"
#include "Skin.h"
#include "SkinImage.h"
#include "SoundEngine.h"
#include "UI.h"
#include "Graphics.h"
#include "Font.h"
#include "Icons.h"
#include "SongBrowser.h"
#include "UniString.h"
#include "UIDraw.h"
#include "UITheme.h"
#include "UIType.h"

UIBackButton::UIBackButton(float xPos, float yPos, float xSize, float ySize, std::string name)
    : CBaseUIButton(xPos, yPos, xSize, ySize, std::move(name), "") {
    // the back button draws on top of the screen body but is visited first, so rank it above the body
    this->bDrawsOnTop = true;
}

UIBackButton::~UIBackButton() = default;

bool UIBackButton::redesigned() const {
    const Skin *skin = osu->getSkin();
    return !UITheme::classic() && !this->bUseDefaultBack && skin->usesDefault(skin->i_menu_back2);
}

void UIBackButton::draw() {
    if(!this->bVisible) return;

    if(this->redesigned()) {
        // stable's pink back button, slanted like the menu bars, running in from off the left edge, with drifting
        // triangles; it brightens and the label nudges right while hovered
        const auto &theme = UITheme::current();
        const McRect r{this->getPos(), this->getSize()};
        const f32 h = r.getHeight();
        const f32 hover = std::clamp<f32>(this->fAnimation, 0.f, 1.f);
        const f32 over = UIType::px(30.f);
        const McRect block{r.getX() - over, r.getY(), r.getWidth() + over - UIType::px(8.f), h};
        const UIDraw::Shape shape = UIDraw::Shape::slanted(block, UIDraw::slant(h));
        const Color white = 0xffffffff;
        UIDraw::glow(shape, UIType::px(16.f), UITheme::fade(theme.pink, 0.3f + 0.25f * hover));
        UIDraw::fill(shape, UITheme::mix(theme.back[0], white, hover * 0.12f),
                     UITheme::mix(theme.back[1], white, hover * 0.12f),
                     UITheme::mix(theme.back[2], white, hover * 0.12f), 0.7f);
        UIDraw::triangles(shape, 31u, 6, h * 0.62f, std::max(1.f, UIType::px(1.6f)), argb(0.3f, 1.f, 1.f, 1.f),
                          (f32)engine->getTime());

        const f32 mid = r.getY() + h * 0.5f;
        const f32 labelW = UIType::width(UIType::Style::BACK, "back");
        const f32 iconW = UIType::px(30.f), gap = UIType::px(12.f);
        const f32 x = r.getX() + (r.getWidth() - UIType::px(8.f) - (iconW + gap + labelW)) * 0.5f + UIType::px(8.f) +
                      hover * UIType::px(6.f);
        UIType::icon(UIType::Style::ICON_30, Icons::CHEVRON_LEFT, {x + iconW * 0.5f, mid}, white);
        UIType::drawCentredY(UIType::Style::BACK, "back", x + iconW + gap, mid, white);

        this->bFocusStolenDelay = false;
        return;
    }

    // draw button image
    g->pushTransform();
    {
        g->setColor(0xffffffff);

        const SkinImage &backimg =
            this->bUseDefaultBack ? osu->getSkin()->i_menu_back2_DEFAULTSKIN : osu->getSkin()->i_menu_back2;

        backimg.draw(this->getPos() + (backimg.getSize() / 2.f), 1.f,
                     this->fAnimation * 0.25f /* hover animation brightness */);
    }
    g->popTransform();

    this->bFocusStolenDelay = false;
}

void UIBackButton::updateInput(CBaseUIEventCtx &c) {
    if(!this->bVisible) return;
    CBaseUIButton::updateInput(c);
}

void UIBackButton::onMouseDownInside(bool left, bool right) {
    CBaseUIButton::onMouseDownInside(left, right);

    soundEngine->play(osu->getSkin()->s_click_back_button);
}

static float button_sound_cooldown = 0.f;
void UIBackButton::onMouseInside() {
    CBaseUIButton::onMouseInside();
    if(this->bFocusStolenDelay) return;

    this->fAnimation.set(1.0f, 0.15f, anim::QuadOut);
    if(button_sound_cooldown + 0.05f < engine->getTime()) {
        button_sound_cooldown = engine->getTime();
        soundEngine->play(osu->getSkin()->s_hover_back_button);
    }
}

void UIBackButton::onMouseOutside() {
    CBaseUIButton::onMouseOutside();

    this->fAnimation.set(0.0f, this->fAnimation * 0.1f, anim::QuadOut);
}

void UIBackButton::updateLayout() {
    const SkinImage *backimg = &osu->getSkin()->i_menu_back2;

    if(OptionsOverlay *optmenu = ui ? ui->getOptionsOverlay() : nullptr;
       optmenu && optmenu->isVisible() && backimg->getSize().y > (optmenu->getSize().y / 4) &&
       (osu->getSkin()->i_menu_back2_DEFAULTSKIN.isReady())) {
        // always show default back button when options menu is showing, if its height is > 1/4 the options menu height
        backimg = &osu->getSkin()->i_menu_back2_DEFAULTSKIN;
        this->bUseDefaultBack = true;
    } else {
        this->bUseDefaultBack = false;
    }

    if(this->redesigned()) {
        // as tall as song select's bottom bar (round 6: 112px)
        this->setSize(UIType::px(240.f), UIType::px(112.f));
        return;
    }
    this->setSize(backimg->getSize());
}

void UIBackButton::resetAnimation() {
    this->fAnimation.stop();
    this->fAnimation = 0.0f;
}

void UIBackButton::onFocusStolen() {
    CBaseUIButton::onFocusStolen();

    this->bMouseInside = false;
    this->bFocusStolenDelay = true;
}
