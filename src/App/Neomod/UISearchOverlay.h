#pragma once
// Copyright (c) 2017, PG, All rights reserved.
#include "CBaseUIElement.h"

#include <utility>

class McFont;

class UISearchOverlay final : public CBaseUIElement {
   public:
    UISearchOverlay(float xPos, float yPos, float xSize, float ySize, std::string name);

    void draw() override;

    void setDrawNumResults(bool drawNumResults) { this->bDrawNumResults = drawNumResults; }
    // the redesign's field (song select); elsewhere (options) the search keeps its centred hint
    void setRedesigned(bool redesigned) { this->bRedesigned = redesigned; }
    void setOffsetRight(int offsetRight) { this->iOffsetRight = offsetRight; }

    inline void setSearchString(std::string searchString, std::string hardcodedSearchString = {}) {
        this->sSearchString = std::move(searchString);
        this->sHardcodedSearchString = std::move(hardcodedSearchString);
    }
    void setNumFoundResults(int numFoundResults) { this->iNumFoundResults = numFoundResults; }

    void setSearching(bool searching) { this->bSearching = searching; }

   private:
    void drawRedesigned();  // the redesign's search pill (docs/renovation/DESIGN.md)
    McFont *font;

    int iOffsetRight;
    bool bDrawNumResults;

    std::string sSearchString;
    std::string sHardcodedSearchString;
    int iNumFoundResults;

    bool bSearching;
    bool bRedesigned{false};
};
