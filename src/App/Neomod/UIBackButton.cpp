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
        // the theme's back: a graded block ending in the diagonal of song select's header, a chevron and "back"
        const auto &theme = UITheme::current();
        const McRect r{this->getPos(), this->getSize()};
        const f32 h = r.getHeight();
        const f32 hover = std::clamp<f32>(this->fAnimation, 0.f, 1.f);
        const Color white = 0xffffffff;
        UIDraw::Shape shape{.rect = r, .cut = h * 0.34f};
        UIDraw::fill(shape, UITheme::mix(theme.back[0], white, hover * 0.18f),
                     UITheme::mix(UITheme::mix(theme.back[0], theme.back[1], 0.5f), white, hover * 0.18f),
                     UITheme::mix(theme.back[1], white, hover * 0.18f));

        McFont *icons = osu->getFontIcons();
        McFont *font = osu->getSongBrowserFont();
        const std::u32string chevron(1, Icons::CHEVRON_LEFT);
        const f32 iconScale = (h * 0.2f) / icons->getHeight();
        const f32 textScale = (h * 0.3f) / font->getHeight();
        const f32 x = r.getX() + h * 0.34f + hover * h * 0.05f;
        g->setColor(theme.backInk);
        g->pushTransform();
        {
            g->scale(iconScale, iconScale);
            g->translate(x, r.getY() + h * 0.5f + icons->getHeight() * iconScale * 0.5f);
            g->drawString(icons, UniString::to_utf8(chevron));
        }
        g->popTransform();
        g->pushTransform();
        {
            g->scale(textScale, textScale);
            g->translate(x + h * 0.24f, r.getY() + h * 0.5f + font->getHeight() * textScale * 0.36f);
            g->drawString(font, "back");
        }
        g->popTransform();

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
        // as tall as song select's bottom bar
        const f32 h = SongBrowser::getUIScale() * 101.f;
        this->setSize(h * 2.45f, h);
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
