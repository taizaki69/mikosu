#pragma once
// Copyright (c) 2026, mikosu contributors, All rights reserved.

// Small pieces the redesign repeats (docs/renovation/DESIGN.md, round 6): star-rating and mod chips, placeholder
// avatars, grade letters. Sizes are design pixels (UIType).

#include "Color.h"
#include "Rect.h"
#include "UIType.h"
#include "Vectors.h"
#include "types.h"

#include <string_view>

enum class ScoreGrade : uint8_t;

namespace UIParts {

// lazer's star rating: a slanted chip in the spectrum colour with a star and the value; returns its width (screen px)
f32 starPill(vec2 leftMid, f32 stars, bool small, f32 opacity = 1.f);
[[nodiscard]] f32 starPillWidth(f32 stars, bool small);

// a mod's acronym on a slanted chip, coloured by what the mod does; returns its width
f32 modPill(vec2 leftMid, std::string_view acronym, f32 opacity = 1.f);

// the name's initial on a gradient picked from the name (for players without an avatar)
void avatarTile(const McRect &r, std::string_view name, UIType::Style initial, f32 radius, f32 opacity = 1.f);

// grade letters: their colour, and the text ("SS", "S", "A", ...)
[[nodiscard]] Color gradeColour(ScoreGrade g);
[[nodiscard]] std::string_view gradeText(ScoreGrade g);

}  // namespace UIParts
