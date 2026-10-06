// Copyright (c) 2017, PG, All rights reserved.
#include "UISearchOverlay.h"
#include "Icons.h"
#include "UIDraw.h"
#include "UITheme.h"

#include <utility>

#include "Engine.h"
#include "Osu.h"
#include "ResourceManager.h"
#include "Font.h"
#include "Graphics.h"
#include "i18n.h"
#include "UniString.h"

UISearchOverlay::UISearchOverlay(float xPos, float yPos, float xSize, float ySize, std::string name)
    : CBaseUIElement(xPos, yPos, xSize, ySize, std::move(name)) {
    // pure-draw decoration spanning the panel: hit-transparent so it never wins the single top-most
    // hover/click/wheel over the real widgets beneath it (handle-off alone is not enough now that
    // candidacy is rect-based and no longer handle-gated)
    this->bClickThroughSelf = true;

    this->font = engine->getDefaultFont();

    this->iOffsetRight = 0;
    this->bDrawNumResults = true;

    this->iNumFoundResults = -1;

    this->bSearching = false;
}

void UISearchOverlay::drawRedesigned() {
    // a frosted pill at the top right: what's been typed (typing anywhere in song select searches, as in stable),
    // or the hint when nothing is; the result count and any fixed filter follow in a softer colour
    const auto &theme = UITheme::current();
    McFont *font = this->font;
    McFont *icons = osu->getFontIcons();
    const f32 h = font->getHeight() * 1.9f;
    const f32 textH = font->getHeight();
    const f32 pad = h * 0.45f;

    const bool typed = UniString::num_codepoints(this->sSearchString) > 0;
    const std::string searchIcon = UniString::to_utf8(std::u32string(1, Icons::SEARCH));
    const f32 iconScale = (textH * 0.85f) / icons->getHeight();
    const f32 iconW = icons->getStringWidth(searchIcon) * iconScale;

    std::string main = typed ? this->sSearchString : std::string{_("Type to search!")};
    std::string sub;
    if(typed && this->bDrawNumResults) {
        if(this->bSearching)
            sub = _("Searching, please wait ...");
        else if(this->iNumFoundResults > 0)
            sub = fmt::format("{:d} match{:s} found!", this->iNumFoundResults, this->iNumFoundResults == 1 ? "" : "es");
        else if(this->iNumFoundResults == 0)
            sub = _("No matches found. Hit ESC to reset.");
    }
    if(!this->sHardcodedSearchString.empty())
        sub = sub.empty() ? this->sHardcodedSearchString : sub + "  \u00b7  " + this->sHardcodedSearchString;

    const f32 mainW = font->getStringWidth(main);
    const f32 subW = sub.empty() ? 0.f : font->getStringWidth(sub) * 0.8f + pad * 0.6f;
    const f32 width = std::min(this->getSize().x, pad * 2.f + iconW + pad * 0.5f + mainW + subW);
    const McRect pill{this->getPos().x + this->getSize().x - width - (f32)this->iOffsetRight, this->getPos().y, width,
                      h};

    const UIDraw::Shape shape = UIDraw::Shape::rounded(pill, h * 0.5f);
    UIDraw::glass(shape, theme.bar);
    UIDraw::Shape edge = shape;
    edge.border = 1.f;
    UIDraw::fill(edge, typed ? Color(theme.lineGlow).setA(0.8f) : theme.barEdge);

    const f32 baseline = pill.getY() + h * 0.5f + textH * 0.36f;
    f32 x = pill.getX() + pad;
    g->pushClipRect(pill);
    g->setColor(theme.ink2);
    g->pushTransform();
    {
        g->scale(iconScale, iconScale);
        g->translate((f32)(i32)x, (f32)(i32)(pill.getY() + h * 0.5f + icons->getHeight() * iconScale * 0.4f));
        g->drawString(icons, searchIcon);
    }
    g->popTransform();
    x += iconW + pad * 0.5f;
    g->setColor(typed ? theme.ink : theme.ink3);
    g->pushTransform();
    {
        g->translate((f32)(i32)x, (f32)(i32)baseline);
        g->drawString(font, main);
    }
    g->popTransform();
    if(!sub.empty()) {
        x += mainW + pad * 0.6f;
        g->setColor(theme.ink3);
        g->pushTransform();
        {
            g->scale(0.8f, 0.8f);
            g->translate((f32)(i32)x, (f32)(i32)baseline);
            g->drawString(font, sub);
        }
        g->popTransform();
    }
    g->popClipRect();
}

void UISearchOverlay::draw() {
    if(!UITheme::classic()) {
        this->drawRedesigned();
        return;
    }

    // draw search text and background
    const float searchTextScale = 1.0f;
    McFont *searchTextFont = this->font;

    const std::string searchText1 = _("Search: ");
    const std::string searchText2 = _("Type to search!");
    const std::string noMatchesFoundText1 = _("No matches found. Hit ESC to reset.");
    const std::string noMatchesFoundText2 = _("Hit ESC to reset.");
    const std::string searchingText2 = _("Searching, please wait ...");

    std::string combinedSearchText = searchText1;
    combinedSearchText.append(searchText2);

    std::string offsetText = (this->iNumFoundResults < 0 ? combinedSearchText : noMatchesFoundText1);
    const uSz hardcodedSearchCodepoints = UniString::num_codepoints(this->sHardcodedSearchString);
    const uSz searchCodepoints = UniString::num_codepoints(this->sSearchString);
    bool hasSearchSubTextVisible = searchCodepoints > 0 && this->bDrawNumResults;

    const float searchStringWidth = searchTextFont->getStringWidth(this->sSearchString);
    const float offsetTextStringWidth = searchTextFont->getStringWidth(offsetText);

    const int offsetTextWidthWithoutOverflow =
        offsetTextStringWidth * searchTextScale + (searchTextFont->getHeight() * searchTextScale) + this->iOffsetRight;

    // calc global x offset for overflowing line (don't wrap, just move everything to the left)
    int textOverflowXOffset = 0;
    {
        const int actualXEnd = (int)(this->getPos().x + this->getSize().x - offsetTextStringWidth * searchTextScale -
                                     (searchTextFont->getHeight() * searchTextScale) * 0.5f - this->iOffsetRight) +
                               (int)(searchTextFont->getStringWidth(searchText1) * searchTextScale) +
                               (int)(searchStringWidth * searchTextScale);
        if(actualXEnd > osu->getVirtScreenWidth()) textOverflowXOffset = actualXEnd - osu->getVirtScreenWidth();
    }

    // draw background
    {
        const float lineHeight = (searchTextFont->getHeight() * searchTextScale);
        float numLines = 1.0f;
        {
            if(hasSearchSubTextVisible)
                numLines = 4.0f;
            else
                numLines = 3.0f;

            if(hardcodedSearchCodepoints > 0) numLines += 1.5f;
        }
        const float height = lineHeight * numLines;
        const int offsetTextWidthWithOverflow = offsetTextWidthWithoutOverflow + textOverflowXOffset;

        g->setColor(argb(searchCodepoints > 0 ? 100 : 30, 0, 0, 0));
        g->fillRect(this->getPos().x + this->getSize().x - offsetTextWidthWithOverflow, this->getPos().y,
                    offsetTextWidthWithOverflow, height);
    }

    // draw text
    g->setColor(0xffffffff);
    g->pushTransform();
    {
        g->translate(0, (int)(searchTextFont->getHeight() / 2.0f));
        g->scale(searchTextScale, searchTextScale);
        g->translate(
            (int)(this->getPos().x + this->getSize().x - offsetTextStringWidth * searchTextScale -
                  (searchTextFont->getHeight() * searchTextScale) * 0.5f - this->iOffsetRight - textOverflowXOffset),
            (int)(this->getPos().y + (searchTextFont->getHeight() * searchTextScale) * 1.5f));

        // draw search text and text
        g->pushTransform();
        {
            g->translate(1, 1);
            g->setColor(0xff000000);
            g->drawString(searchTextFont, searchText1);
            g->translate(-1, -1);
            g->setColor(0xff00ff00);
            g->drawString(searchTextFont, searchText1);

            if(hardcodedSearchCodepoints > 0) {
                const float searchText1Width = searchTextFont->getStringWidth(searchText1) * searchTextScale;

                g->pushTransform();
                {
                    g->translate(searchText1Width, 0);

                    g->translate(1, 1);
                    g->setColor(0xff000000);
                    g->drawString(searchTextFont, this->sHardcodedSearchString);
                    g->translate(-1, -1);
                    g->setColor(0xff34ab94);
                    g->drawString(searchTextFont, this->sHardcodedSearchString);
                }
                g->popTransform();

                g->translate(0, searchTextFont->getHeight() * searchTextScale * 1.5f);
            }

            g->translate((int)(searchTextFont->getStringWidth(searchText1) * searchTextScale), 0);
            g->translate(1, 1);
            g->setColor(0xff000000);
            if(searchCodepoints < 1)
                g->drawString(searchTextFont, searchText2);
            else
                g->drawString(searchTextFont, this->sSearchString);

            g->translate(-1, -1);
            g->setColor(0xffffffff);
            if(searchCodepoints < 1)
                g->drawString(searchTextFont, searchText2);
            else
                g->drawString(searchTextFont, this->sSearchString);
        }
        g->popTransform();

        // draw number of matches
        if(hasSearchSubTextVisible) {
            g->translate(0, (int)((searchTextFont->getHeight() * searchTextScale) * 1.5f *
                                  (hardcodedSearchCodepoints > 0 ? 2.0f : 1.0f)));
            g->translate(1, 1);

            if(this->bSearching) {
                g->setColor(0xff000000);
                g->drawString(searchTextFont, searchingText2);
                g->translate(-1, -1);
                g->setColor(0xffffffff);
                g->drawString(searchTextFont, searchingText2);
            } else {
                g->setColor(0xff000000);
                g->drawString(searchTextFont, this->iNumFoundResults > -1
                                                  ? (this->iNumFoundResults > 0
                                                         ? fmt::format("{:d} match{:s} found!", this->iNumFoundResults,
                                                                       this->iNumFoundResults == 1 ? "" : "es")
                                                         : noMatchesFoundText1)
                                                  : noMatchesFoundText2);
                g->translate(-1, -1);
                g->setColor(0xffffffff);
                g->drawString(searchTextFont, this->iNumFoundResults > -1
                                                  ? (this->iNumFoundResults > 0
                                                         ? fmt::format("{:d} match{:s} found!", this->iNumFoundResults,
                                                                       this->iNumFoundResults == 1 ? "" : "es")
                                                         : noMatchesFoundText1)
                                                  : noMatchesFoundText2);
            }
        }
    }
    g->popTransform();
}
