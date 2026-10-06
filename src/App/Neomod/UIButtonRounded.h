// Copyright (c) 2026, WH, All rights reserved.
#pragma once
#include "CBaseUIButton.h"

class UIButtonRounded : public CBaseUIButton {
    NOCOPY_NOMOVE(UIButtonRounded)
   public:
    UIButtonRounded(float xPos = 0, float yPos = 0, float xSize = 0, float ySize = 0, std::string name = {},
                    std::string text = {}, int cornerRadius = 6);
    ~UIButtonRounded() override;
    UIButtonRounded* setCornerRadius(int radius);
    [[nodiscard]] inline int getCornerRadius() const { return this->cornerRadius; }

    // the redesign's looks (docs/renovation/DESIGN.md), used outside the classic theme: a frosted field (a chevron if
    // it opens a dropdown) or a text tab with a glowing underline when it's the active one
    enum class Themed : uint8_t { NONE, FIELD, TAB };
    UIButtonRounded* setThemed(Themed style, bool dropdown = false);
    UIButtonRounded* setThemedActive(bool active);

    void draw() override;

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
};
