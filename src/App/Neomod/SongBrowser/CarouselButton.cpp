// Copyright (c) 2016, PG, All rights reserved.
#include "CarouselButton.h"

#include <functional>
#include <utility>

#include "StarPrecalc.h"
#include "Logging.h"
#include "SongBrowser.h"
#include "BeatmapCarousel.h"
#include "SongDifficultyButton.h"
// ---

#include "AnimationHandler.h"
#include "CBaseUIScrollView.h"
#include "OsuConVars.h"
#include "Environment.h"
#include "Engine.h"
#include "Graphics.h"
#include "Mouse.h"
#include "Osu.h"
#include "Skin.h"
#include "SoundEngine.h"
#include "UI.h"
#include "UIContextMenu.h"
#include "ContainerRanges.h"
#include "UIDraw.h"
#include "UITheme.h"
#include "UIType.h"

using namespace neomod::sbr;

// Color Button::inactiveDifficultyBackgroundColor = argb(255, 0, 150, 236); // blue

CarouselButton::CarouselButton(float xPos, float yPos, float xSize, float ySize, std::nullptr_t nullp)
    : CBaseUIButton(nullp, xPos, yPos, xSize, ySize) {
    this->setHandleRightMouse(true);
    this->font = osu->getSongBrowserFont();
    this->bVisible = false;
    this->fTargetRelPosY = yPos;
    this->rect.setSize(CarouselButton::scaledBaseSize);
}

CarouselButton::CarouselButton(float xPos, float yPos, float xSize, float ySize, std::string name)
    : CBaseUIButton(xPos, yPos, xSize, ySize, std::move(name)) {
    this->setHandleRightMouse(true);
    this->font = osu->getSongBrowserFont();
    this->bVisible = false;
    this->fTargetRelPosY = yPos;
    this->rect.setSize(CarouselButton::scaledBaseSize);
}

CarouselButton::~CarouselButton() { this->deleteAnimations(); }

void CarouselButton::updateResolution() {
    if(redesigned()) {
        // round 6 (docs/renovation/mockups/songselect6.html): 100px cards with 12px between them, wide enough that
        // the selected one starts at about half the screen's width (the stable layout's offsets do the rest)
        const f32 sc = UIType::scale();
        actualScaledOffsetWithMargin = vec2{0.f, 0.f};
        scaledBaseSize = vec::ceil(vec2{1108.f, 112.f} * sc);
        bgImageScale = 1.f;
        return;
    }
    const f32 currentUIScale = Osu::getUIScale(baseOsuPixelsScale);
    actualScaledOffsetWithMargin = vec::ceil(vec2{(int)marginPixelsX, (int)(marginPixelsY)} * currentUIScale);
    scaledBaseSize = vec::ceil(baseSize * currentUIScale);

    // complete BS sizing/rounding/etc.
    // it seems that osu stable also doesn't scale these images in any way, though
    // NOTE: the only reason we can use the current skin's menu-button-background scale here is because this
    // function is also called after (re)loading a skin
    bgImageScale = (currentUIScale + 0.005f /* ??? */) / (osu->getSkin()->i_menu_button_bg.scale());
}

void CarouselButton::deleteAnimations() {
    this->fCenterOffsetAnimation.stop();
    this->fCenterOffsetVelocityAnimation.stop();
    this->fHoverOffsetAnimation.stop();
    this->fHoverMoveAwayAnimation.stop();
}

void CarouselButton::draw() {
    if(!this->bVisible) return;

    this->drawMenuButtonBackground();

    // debug inner bounding box
    if(cv::debug_osu.getBool()) {
        // scaling
        const vec2 pos = this->getActualPos();
        const vec2 size = this->getActualSize();

        g->setColor(0xffff00ff);
        g->drawLine(pos.x, pos.y, pos.x + size.x, pos.y);
        g->drawLine(pos.x, pos.y, pos.x, pos.y + size.y);
        g->drawLine(pos.x, pos.y + size.y, pos.x + size.x, pos.y + size.y);
        g->drawLine(pos.x + size.x, pos.y, pos.x + size.x, pos.y + size.y);
    }

    // debug outer/actual bounding box
    if(cv::debug_osu.getBool()) {
        g->setColor(0xffff0000);
        g->drawLine(this->getPos().x, this->getPos().y, this->getPos().x + this->getSize().x, this->getPos().y);
        g->drawLine(this->getPos().x, this->getPos().y, this->getPos().x, this->getPos().y + this->getSize().y);
        g->drawLine(this->getPos().x, this->getPos().y + this->getSize().y, this->getPos().x + this->getSize().x,
                    this->getPos().y + this->getSize().y);
        g->drawLine(this->getPos().x + this->getSize().x, this->getPos().y, this->getPos().x + this->getSize().x,
                    this->getPos().y + this->getSize().y);
    }
}

bool CarouselButton::redesigned() {
    const Skin *skin = osu->getSkin();
    return !UITheme::classic() && skin->usesDefault(skin->i_menu_button_bg);
}

Color CarouselButton::textColour(bool selectedStyle, bool secondary) const {
    if(!redesigned()) {
        const Skin *skin = osu->getSkin();
        return selectedStyle ? skin->c_song_select_active_text : skin->c_song_select_inactive_text;
    }
    const auto &theme = UITheme::current();
    if(selectedStyle) return secondary ? theme.selInk2 : theme.selInk;
    return secondary ? theme.cardInk2 : theme.cardInk;
}

McRect CarouselButton::cardRect() const {
    // the card inside the button's slot (the gap is split above and below), running off the screen's right edge
    const vec2 pos = this->getActualPos();
    const vec2 size = this->getActualSize();
    const f32 gap = UIType::px(6.f);
    const f32 right = (f32)osu->getVirtScreenWidth() + UIType::px(4.f);
    return {pos.x, pos.y + gap, std::max(right - pos.x, 1.f), std::max(size.y - 2.f * gap, 1.f)};
}

Color CarouselButton::cardMark() const {
    const auto &theme = UITheme::current();
    switch(this->panelKind()) {
        case PanelKind::GROUP:
            return theme.groupMark;
        case PanelKind::DIFF:
            return theme.diffCard;
        default:
            return theme.setMark;
    }
}

void CarouselButton::drawCard(const Image *art, f32 artAlpha) {
    // stable's colours by kind (pink sets, blue difficulties, white selection) as solid cards with a rounded left
    // end; the map's art shows through towards the right, under a faint drift of triangles
    const auto &theme = UITheme::current();
    const McRect r = this->cardRect();
    const f32 radius = UIType::px(24.f);
    const UIDraw::Shape card{.rect = r, .radii = {radius, 0.f, 0.f, radius}};
    const PanelKind kind = this->panelKind();
    const Color base = this->bSelected            ? theme.selCard
                       : kind == PanelKind::DIFF  ? theme.diffCard
                       : kind == PanelKind::GROUP ? theme.groupCard
                                                  : theme.setCard;

    // a soft shadow below, and osu!'s pink around the selection
    UIDraw::Shape shadow = card;
    shadow.rect = McRect{r.getX(), r.getY() + UIType::px(6.f), r.getWidth(), r.getHeight()};
    UIDraw::glow(shadow, UIType::px(10.f), argb(0.3f, 0.f, 0.f, 0.f));
    if(this->bSelected) UIDraw::glow(card, UIType::px(14.f), theme.selGlow);

    UIDraw::fill(card, base);
    if(art != nullptr && artAlpha > 0.f) {
        UIDraw::image(card, art, argb(artAlpha, 1.f, 1.f, 1.f));
        // the colour over the art: solid behind the text, thinning out to the right
        const std::array<UIDraw::Stop, 7> tint{{{0.f, UITheme::fade(base, 1.f)},
                                                {0.3f, UITheme::fade(base, 1.f)},
                                                {0.42f, UITheme::fade(base, 0.9f)},
                                                {0.55f, UITheme::fade(base, 0.68f)},
                                                {0.7f, UITheme::fade(base, 0.45f)},
                                                {0.85f, UITheme::fade(base, 0.3f)},
                                                {1.f, UITheme::fade(base, 0.24f)}}};
        UIDraw::fillStops(card, tint);
    }

    // the coloured edge, following the rounded corners
    const f32 edge = UIType::px(8.f) / std::max(r.getWidth(), 1.f);
    const Color mark = this->cardMark();
    const std::array<UIDraw::Stop, 3> markStops{{{0.f, mark}, {edge, mark}, {edge, UITheme::fade(mark, 0.f)}}};
    UIDraw::fillStops(card, markStops);

    const u32 seed = (u32)(std::hash<const void *>{}(this) & 0xffffffffu);
    UIDraw::triangles(card, seed, 8, UIType::px(64.f), std::max(1.f, UIType::px(1.5f)),
                      this->bSelected ? UITheme::fade(theme.pink, 0.16f) : argb(0.07f, 1.f, 1.f, 1.f),
                      (f32)engine->getTime());
}

void CarouselButton::drawMenuButtonBackground() {
    if(redesigned()) {
        this->drawCard(nullptr, 0.f);
        return;
    }

    g->setColor(this->bSelected ? this->getActiveBackgroundColor() : this->getInactiveBackgroundColor());
    g->pushTransform();
    {
        g->scale(bgImageScale, bgImageScale);
        g->translate(this->getPos().x - 1.f /* random? */, this->getPos().y + this->getSize().y / 2);
        g->drawImage(osu->getSkin()->i_menu_button_bg, AnchorPoint::LEFT);
    }
    g->popTransform();
}

void CarouselButton::tick() {
    CBaseUIButton::tick();
    if(!this->bVisible) return;

    // animations need constant layout updates while visible
    this->updateLayoutEx();
}

void CarouselButton::updateInput(CBaseUIEventCtx &c) {
    if(!this->bVisible) return;

    // HACKHACK: absolutely disgusting
    // temporarily fool CBaseUIElement with modified position and size
    {
        vec2 posBackup = this->getPos();
        vec2 sizeBackup = this->getSize();

        this->rect.setPos(this->getActualPos());
        this->rect.setSize(this->getActualSize());
        {
            CBaseUIButton::updateInput(c);
        }
        this->rect.setPos(posBackup);
        this->rect.setSize(sizeBackup);
    }
}

void CarouselButton::updateLayoutEx() {
    if(this->bVisible) {  // lag prevention (animationHandler overflow)
        const float centerOffsetAnimationTarget =
            ((cv::songbrowser_button_anim_y_curve.getBool() && !g_carousel->isScrollingFast())
                 ? 1.0f - std::clamp<float>(std::abs((this->getPos().y + (CarouselButton::scaledBaseSize.y / 2) -
                                                      g_carousel->getPos().y - g_carousel->getSize().y / 2) /
                                                     (g_carousel->getSize().y / 2)),
                                            0.0f, 1.0f)
                 : 1.f) +
            (this->bSelected && !cv::songbrowser_button_anim_y_curve.getBool() ? 0.5f : 0.0f);

        if(std::abs(this->fCenterOffsetAnimation - centerOffsetAnimationTarget) > 0.001f) {
            this->fCenterOffsetAnimation.set(centerOffsetAnimationTarget, 0.5f, anim::QuadOut);
        }

        float centerOffsetVelocityAnimationTarget =
            std::clamp<float>((std::abs(g_carousel->getVelocity().y)) / 3500.0f, 0.0f, 1.0f);

        if(g_carousel->isScrollingFast() || !cv::songbrowser_button_anim_x_push.getBool())
            centerOffsetVelocityAnimationTarget = 0.0f;

        if(g_carousel->isScrolling())
            this->fCenterOffsetVelocityAnimation.set(0.0f, 1.0f, anim::QuadOut);
        else
            this->fCenterOffsetVelocityAnimation.set(centerOffsetVelocityAnimationTarget, 1.25f, anim::QuadOut);
    }

    const float percentCenterOffsetAnimation = 0.05f;
    const float percentVelocityOffsetAnimation = 0.35f;
    const float percentHoverOffsetAnimation = 0.075f;

    // this is the minimum offset necessary to not clip into the score scrollview (including all possible max animations
    // which can push us to the left, worst case)
    float minOffset = g_carousel->getSize().x * (percentCenterOffsetAnimation + percentHoverOffsetAnimation);

    // also respect the width of the button image: push to the right until the edge of the button image can never be
    // visible even if all animations are fully active the 0.91f here heuristically pushes the buttons a bit further
    // to the right than would be necessary, to make animations work better on lower resolutions (would otherwise
    // hit the left edge too early)
    // NOTE @spec: this 0.91 and 0.09 relationship is extremely important for the actual offset amount somehow
    const float buttonWidthCompensation = std::max(g_carousel->getSize().x - this->getActualWidth() * 0.91f, 0.0f);
    minOffset += buttonWidthCompensation;

    float offsetX =
        minOffset - g_carousel->getSize().x *
                        (percentCenterOffsetAnimation * this->fCenterOffsetAnimation *
                             (1.0f - this->fCenterOffsetVelocityAnimation) +
                         percentHoverOffsetAnimation * this->fHoverOffsetAnimation -
                         percentVelocityOffsetAnimation * this->fCenterOffsetVelocityAnimation + this->fOffsetPercent);
    offsetX = std::clamp<float>(
        offsetX, 0.0f,
        g_carousel->getSize().x -
            this->getActualWidth() * 0.09f);  // WARNING: hardcoded to match 0.91f above for buttonWidthCompensation

    this->setRelPosX(offsetX);
    this->setRelPosY(this->fTargetRelPosY + CarouselButton::scaledBaseSize.y * 0.125f * this->fHoverMoveAwayAnimation);

    // using setRect instead of setPos/setSize to avoid expensive comparisons
    // (which do nothing since CarouselButtons don't use onResized/onMoved)
    this->setRect(McRect{g_carousel->container.getPos() + this->getRelPos(), CarouselButton::scaledBaseSize});
}

CarouselButton *CarouselButton::setVisible(bool visible) {
    CBaseUIButton::setVisible(visible);

    this->deleteAnimations();

    if(this->bVisible) {
        // scrolling pinch effect
        this->fCenterOffsetAnimation = 1.0f;
        this->fHoverOffsetAnimation = 0.0f;

        float centerOffsetVelocityAnimationTarget =
            std::clamp<float>((std::abs(g_carousel->getVelocity().y)) / 3500.0f, 0.0f, 1.0f);

        if(g_carousel->isScrollingFast() || !cv::songbrowser_button_anim_x_push.getBool())
            centerOffsetVelocityAnimationTarget = 0.0f;

        this->fCenterOffsetVelocityAnimation = centerOffsetVelocityAnimationTarget;

        // force early layout update
        this->updateLayoutEx();
    }

    return this;
}

void CarouselButton::select(SelOpts opts) {
    const bool wasSelected = this->bSelected;
    this->bSelected = true;

    // callback
    if(!opts.noCallbacks) this->onSelected(wasSelected, opts);
}

void CarouselButton::deselect() { this->bSelected = false; }

void CarouselButton::resetAnimations() { this->setMoveAwayState(MOVE_AWAY_STATE::MOVE_CENTER, false); }

void CarouselButton::onClicked(bool left, bool right) {
    CBaseUIButton::onClicked(left, right);
    if(left) {
        soundEngine->play(osu->getSkin()->s_select_difficulty);

        this->select();
    }
}

void CarouselButton::onMouseInside() {
    CBaseUIButton::onMouseInside();

    // hover sound
    if(engine->getTime() > lastHoverSoundTime + 0.05f)  // to avoid earraep
    {
        if(env->winFocused()) soundEngine->play(osu->getSkin()->s_menu_hover);

        lastHoverSoundTime = engine->getTime();
    }

    // hover anim
    this->fHoverOffsetAnimation.set(1.0f, 1.0f * (1.0f - this->fHoverOffsetAnimation), anim::QuadOut);

    // all elements must be CarouselButtons, at least
    const auto &elements{g_carousel->container.getElementsAs<CarouselButton>()};

    // move the rest of the buttons away from hovered-over one
    bool foundCenter = false;
    for(auto element : elements) {
        if(element == this) {
            foundCenter = true;
            element->setMoveAwayState(MOVE_AWAY_STATE::MOVE_CENTER);
        } else
            element->setMoveAwayState(foundCenter ? MOVE_AWAY_STATE::MOVE_DOWN : MOVE_AWAY_STATE::MOVE_UP);
    }
}

void CarouselButton::onMouseOutside() {
    CBaseUIButton::onMouseOutside();

    // reverse hover anim
    this->fHoverOffsetAnimation.set(0.0f, 1.0f * this->fHoverOffsetAnimation, anim::QuadOut);

    // only reset all other elements' state if we still should do so (possible frame delay of onMouseOutside coming
    // together with the next element already getting onMouseInside!)
    if(this->moveAwayState == MOVE_AWAY_STATE::MOVE_CENTER) {
        const auto &elements{g_carousel->container.getElementsAs<CarouselButton>()};
        for(auto *element : elements) {
            element->setMoveAwayState(MOVE_AWAY_STATE::MOVE_CENTER);
        }
    }
}

void CarouselButton::setTargetRelPosY(float targetRelPosY) {
    this->fTargetRelPosY = targetRelPosY;
    this->setRelPosY(this->fTargetRelPosY);
}

void CarouselButton::setMoveAwayState(CarouselButton::MOVE_AWAY_STATE moveAwayState, bool animate) {
    this->moveAwayState = moveAwayState;

    // if we are not visible, destroy possibly existing animation
    if(this->bWasAnimationEverStarted && (!this->isVisible() || !animate)) {
        this->fHoverMoveAwayAnimation.stop();
        this->bWasAnimationEverStarted = false;
    }

    // only submit a new animation if we are visible, otherwise we would overwhelm the animationhandler with a shitload
    // of requests every time for every button (if we are not visible then we can just directly set the new value)
    switch(this->moveAwayState) {
        case MOVE_AWAY_STATE::MOVE_CENTER: {
            if(!this->isVisible() || !animate)
                this->fHoverMoveAwayAnimation = 0.0f;
            else {
                this->bWasAnimationEverStarted = true;
                this->fHoverMoveAwayAnimation.set(
                    0.f, 0.7f, anim::QuartOut,
                    this->isMouseInside()
                        ? 0.0f
                        : 0.05f);  // add a tiny bit of delay to avoid jerky movement if the cursor is briefly
                                   // between songbuttons while moving
            }
        } break;

        case MOVE_AWAY_STATE::MOVE_UP: {
            if(!this->isVisible() || !animate)
                this->fHoverMoveAwayAnimation = -1.0f;
            else {
                this->bWasAnimationEverStarted = true;
                this->fHoverMoveAwayAnimation.set(-1.0f, 0.7f, anim::QuartOut);
            }
        } break;

        case MOVE_AWAY_STATE::MOVE_DOWN: {
            if(!this->isVisible() || !animate)
                this->fHoverMoveAwayAnimation = 1.0f;
            else {
                this->bWasAnimationEverStarted = true;
                this->fHoverMoveAwayAnimation.set(1.0f, 0.7f, anim::QuartOut);
            }
        } break;
    }
}

void CarouselButton::setChildren(std::vector<SongButton *> children) {
    this->lastChildSortStarPrecalcIdx = 0xFF;
    this->children = std::move(children);
}

void CarouselButton::addChild(SongButton *child) {
    this->lastChildSortStarPrecalcIdx = 0xFF;
    this->children.push_back(child);
}

void CarouselButton::addChildren(std::vector<SongButton *> children) {
    this->lastChildSortStarPrecalcIdx = 0xFF;
    Mc::ranges::append(this->children, std::move(children));
}

bool CarouselButton::childrenNeedSorting() const {
    return this->lastChildSortStarPrecalcIdx != StarPrecalc::active_idx;
}

Color CarouselButton::getActiveBackgroundColor() const { return ARGB_CV_TO_COL(songbrowser_button_active_color); }
Color CarouselButton::getInactiveBackgroundColor() const { return ARGB_CV_TO_COL(songbrowser_button_inactive_color); }

bool CarouselButton::isIndependentDiffButton() const {
    if(!this->parentSongButton || !this->parentSongButton->isSelected()) return true;

    const auto *songDiffBtn = this->as<const SongDifficultyButton>();
    assert(songDiffBtn != nullptr);  // only SongDifficultyButtons have parentSongButton set

    // TODO: this logic is very weird and only works "accidentally";
    // you'd think returning true IFF (sibling->isSearchMatch() && sibling == this) would be enough,
    // but it doesn't work as expected...

    // check if this is the only visible sibling
    int visibleSiblings = 0;
    for(const auto *sibling : songDiffBtn->getSiblingsAndSelf()) {
        if(sibling->isSearchMatch()) {
            visibleSiblings++;
            if(visibleSiblings > 1) return false;  // early exit
        }
    }

    return (visibleSiblings == 1);
}
