// Copyright (c) 2026, WH, All rights reserved.
#pragma once
#include "AnimationHandler.h"
#include "CBaseUIButton.h"

class UIButtonRounded : public CBaseUIButton {
    NOCOPY_NOMOVE(UIButtonRounded)
   public:
    UIButtonRounded(float xPos = 0, float yPos = 0, float xSize = 0, float ySize = 0, std::string name = {},
                    std::string text = {}, int cornerRadius = 6);
    ~UIButtonRounded() override;
    UIButtonRounded* setCornerRadius(int radius);
    [[nodiscard]] inline int getCornerRadius() const { return this->cornerRadius; }

    // the redesign's looks (docs/renovation/DESIGN.md, round 6), used outside the classic theme:
    //   LINK  text (and a chevron if it opens a dropdown)
    //   TAB   a tab: bright with osu!'s pink underline when it's the active one
    //   MENU  the main menu's slanted bars: violet, pink and pushed out while hovered, with drifting triangles
    // FIELD is LINK (the name older call sites use)
    enum class Themed : uint8_t { NONE, FIELD, LINK, TAB, MENU };
    UIButtonRounded* setThemed(Themed style, bool dropdown = false);
    // MENU: the icon glyph (0: none) and how much of the bar's left end is hidden (behind the logo)
    UIButtonRounded* setThemedAccent(size_t menuAccentIndex, char32_t glyph);
    UIButtonRounded* setThemedInset(f32 hiddenLeft);
    UIButtonRounded* setThemedActive(bool active);

    // the width the themed look needs for its text (LINK and TAB), to lay buttons out by their labels
    [[nodiscard]] f32 getThemedWidth() const;

    void draw() override;
    void onMouseInside() override;
    void onMouseOutside() override;

   protected:
    void drawBackground() override;
    void drawFrame() override;
    void drawHoverRect(int hoverRectOffset, bool isClickHeld) override;

    // based on font dpi (more rounded for higher dpi)
    [[nodiscard]] int getRealCornerRadius() const;

   private:
    int cornerRadius{6};
    Themed themed{Themed::NONE};
    bool themedDropdown{false};
    bool themedActive{false};
    size_t themedAccent{0};
    char32_t themedGlyph{0};
    f32 themedInset{0.f};
    AnimFloat hoverAnim;
    u32 triangleSeed{0};
};
