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
    f32 cutLeft{0.f};                              // diagonal cut at the left end: how much shorter the top is
    f32 softness{1.f};                             // about 1 antialiases; more gives a soft shadow or glow
    f32 border{0.f};                               // 0 = filled, else a border this wide inside the edge
    f32 opacity{1.f};                              // multiplies the whole shape (fades)

    static Shape rounded(const McRect &r, f32 radius) { return {.rect = r, .radii = {radius, radius, radius, radius}}; }
    // a parallelogram leaning like '/' (both ends cut by `slant`)
    static Shape slanted(const McRect &r, f32 slant) { return {.rect = r, .cut = slant, .cutLeft = slant}; }
};

// how far the redesign's slanted ends lean over a height (stable's menu bars: 12 degrees)
inline f32 slant(f32 height) { return height * 0.2126f; }

// a horizontal gradient through up to three stops (left, at `mid`, right)
void fill(const Shape &s, Color left, Color middle, Color right, f32 mid = 0.55f);
inline void fill(const Shape &s, Color colour) { fill(s, colour, colour, colour); }

// a horizontal gradient through any number of stops (positions 0..1 across the shape, increasing)
struct Stop {
    f32 at;
    Color colour;
};
void fillStops(const Shape &s, std::span<const Stop> stops);

// frosted glass: the blurred background under a tint whose alpha is its opacity; a plain fill until the blurred
// background exists
void glass(const Shape &s, Color left, Color middle, Color right, f32 mid = 0.55f);
inline void glass(const Shape &s, Color tint) { glass(s, tint, tint, tint); }

// a convex polygon (a fan from the first point) as frosted glass; no antialiasing, so put a line on its edges
void glassPolygon(std::span<const vec2> points, Color tint);

// a convex polygon in one colour (no antialiasing)
void fillPolygon(std::span<const vec2> points, Color colour);

// a rectangle graded from top to bottom
void fillVertical(const McRect &r, Color top, Color bottom);

// bars radiating from a circle of radius `r0` around `centre`, `rounds` times around (each round offset by a full turn
// over `rounds`), bar i of a round at angle `rotation` + i/n of a turn; lengths in pixels (stable's menu visualiser)
void radialBars(vec2 centre, f32 r0, std::span<const f32> lengths, f32 width, Color colour, f32 rotation, int rounds);

// lazer-style outlined triangles drifting upwards inside the shape (seeded, so each element keeps its own pattern);
// `size` is the triangles' width, `stroke` their outline, `time` in seconds drives the drift
void triangles(const Shape &clip, u32 seed, int count, f32 size, f32 stroke, Color colour, f32 time);

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
