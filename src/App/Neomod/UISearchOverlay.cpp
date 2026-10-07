// Copyright (c) 2017, PG, All rights reserved.
#include "UISearchOverlay.h"
#include "Icons.h"
#include "UIDraw.h"
#include "UITheme.h"
#include "UIType.h"

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
    // round 6: a slanted field (typing anywhere in song select searches, as in stable): the search icon, what's been
    // typed or the hint, and the result count or any fixed filter right-aligned in a softer colour
    using Style = UIType::Style;
    const auto &theme = UITheme::current();
    const bool typed = UniString::num_codepoints(this->sSearchString) > 0;

    std::string sub;
    if(typed && this->bDrawNumResults) {
        if(this->bSearching)
            sub = _("Searching, please wait ...");
        else if(this->iNumFoundResults > 0)
            sub = fmt::format("{:d} match{:s}", this->iNumFoundResults, this->iNumFoundResults == 1 ? "" : "es");
        else if(this->iNumFoundResults == 0)
            sub = _("No matches found. Hit ESC to reset.");
    }
    if(!this->sHardcodedSearchString.empty())
        sub = sub.empty() ? this->sHardcodedSearchString : sub + "  ·  " + this->sHardcodedSearchString;
    const std::string main = typed ? this->sSearchString : std::string{_("Type to search")};

    // song select gives the field its size; elsewhere (options) it sizes to its text at the right
    const f32 pad = UIType::px(30.f), iconW = UIType::px(22.f), gap = UIType::px(14.f);
    const f32 subW = sub.empty() ? 0.f : UIType::width(Style::FINE, sub) + gap;
    McRect field{this->getPos(), this->getSize()};
    if(field.getHeight() < UIType::px(40.f)) {
        const f32 h = UIType::px(46.f);
        const f32 w = std::min(this->getSize().x, pad * 2.f + iconW + gap + UIType::width(Style::BODY, main) + subW);
        field = McRect{this->getPos().x + this->getSize().x - w - (f32)this->iOffsetRight, this->getPos().y, w, h};
    }
    const f32 h = field.getHeight();
    const UIDraw::Shape shape = UIDraw::Shape::slanted(field, UIType::px(12.f));
    UIDraw::fill(shape, theme.light ? argb(0.07f, 0.094f, 0.078f, 0.172f) : argb(0.1f, 1.f, 1.f, 1.f));
    if(typed) {
        const std::array<UIDraw::Stop, 3> lit{{{0.f, UITheme::fade(theme.pink, 0.22f)},
                                               {0.6f, UITheme::fade(theme.pink, 0.06f)},
                                               {1.f, UITheme::fade(theme.pink, 0.f)}}};
        UIDraw::fillStops(shape, lit);
    }

    const f32 mid = field.getY() + h * 0.5f;
    f32 x = field.getX() + pad;
    UIType::icon(Style::ICON_22, Icons::SEARCH, {x + iconW * 0.5f, mid}, typed ? theme.pink : theme.ink);
    x += iconW + gap;
    const f32 right = field.getX() + field.getWidth() - pad;
    if(!sub.empty()) {
        const std::string fitted =
            UIType::fit(Style::FINE, sub, std::max(right - x - UIType::px(120.f), UIType::px(40.f)));
        const f32 w = UIType::width(Style::FINE, fitted);
        UIType::drawCentredY(Style::FINE, fitted, right - w, mid, theme.ink4);
        UIType::drawCentredY(Style::BODY,
                             UIType::fit(Style::BODY, main, std::max(right - w - gap - x, UIType::px(20.f))), x, mid,
                             typed ? theme.ink : theme.ink3);
    } else {
        UIType::drawCentredY(Style::BODY, UIType::fit(Style::BODY, main, std::max(right - x, UIType::px(20.f))), x, mid,
                             typed ? theme.ink : theme.ink3);
    }
}

void UISearchOverlay::draw() {
    if(this->bRedesigned && !UITheme::classic()) {
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
