#pragma once
// Copyright (c) 2026, mikosu contributors, All rights reserved.

// Drawing pieces for the redesign (docs/renovation/DESIGN.md, round 4): antialiased rounded shapes with an optional
// diagonal cut, frosted glass over the blurred background, rounded images, soft glows and glowing lines. Everything
// goes through one shader (assets/shaders/VK_uishape_*.glsl). The blurred background is built once per background
// image (assets/shaders/VK_blur_*.glsl), never per frame.

#include "Color.h"
#include "Rect.h"
#include "Vectors.h"
#include "types.h"

#include <array>
#include <span>

class Image;

namespace UIDraw {

struct Shape {
    McRect rect;
    std::array<f32, 4> radii{0.f, 0.f, 0.f, 0.f};  // top-left, top-right, bottom-right, bottom-left
    f32 cut{0.f};                                  // diagonal cut at the right end: how much shorter the bottom is
    f32 softness{1.f};                             // about 1 antialiases; more gives a soft shadow or glow
    f32 border{0.f};                               // 0 = filled, else a border this wide inside the edge
    f32 opacity{1.f};                              // multiplies the whole shape (fades)

    static Shape rounded(const McRect &r, f32 radius) { return {.rect = r, .radii = {radius, radius, radius, radius}}; }
};

// a horizontal gradient through up to three stops (left, at `mid`, right)
void fill(const Shape &s, Color left, Color middle, Color right, f32 mid = 0.55f);
inline void fill(const Shape &s, Color colour) { fill(s, colour, colour, colour); }

// frosted glass: the blurred background under a tint whose alpha is its opacity; a plain fill until the blurred
// background exists
void glass(const Shape &s, Color left, Color middle, Color right, f32 mid = 0.55f);
inline void glass(const Shape &s, Color tint) { glass(s, tint, tint, tint); }

// a convex polygon (a fan from the first point) as frosted glass; no antialiasing, so put a line on its edges
void glassPolygon(std::span<const vec2> points, Color tint);

// an image cropped to fill the shape; `tint` multiplies it
void image(const Shape &s, const Image *img, Color tint = 0xffffffff);

// a soft shadow or glow: the shape blurred outwards by `spread` pixels
void glow(const Shape &s, f32 spread, Color colour);

// a line through the points, graded along its length through three colours, with a soft glow on both sides
void glowLine(std::span<const vec2> points, const std::array<Color, 3> &grad, f32 width, Color glow, f32 glowWidth);

// the image drawn behind the UI, as the game draws backgrounds (scaled to fill the screen, centred); the blurred copy
// for the glass is rebuilt when it changes. Call at the top level of drawing (no transform pushed).
void setBackdrop(const Image *bg);

// drop the blurred background (resolution change, shutdown)
void releaseBackdrop();

}  // namespace UIDraw
