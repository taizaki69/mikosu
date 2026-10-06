// Copyright (c) 2026, mikosu contributors, All rights reserved.
#include "UIDraw.h"

#include "Engine.h"
#include "Graphics.h"
#include "Image.h"
#include "Matrices.h"
#include "Osu.h"
#include "RenderTarget.h"
#include "ResourceManager.h"
#include "Shader.h"
#include "UITheme.h"
#include "VertexArrayObject.h"

#include <algorithm>
#include <cmath>

namespace UIDraw {
namespace {

// the blurred background is kept this wide (its height follows the image); the blur is 17 texels wide, so about
// 1/4 of a 1920-wide screen per texel gives roughly the mockups' frosting
constexpr int BACKDROP_WIDTH = 480;

Shader *shapeShader{nullptr};
Shader *blurShader{nullptr};

VertexArrayObject strip{DrawPrimitive::TRIANGLE_STRIP};
VertexArrayObject fan{DrawPrimitive::TRIANGLE_FAN};

struct {
    const Image *wanted{nullptr};  // what the game draws behind the UI now
    const Image *built{nullptr};   // what the blurred copy shows
    RenderTarget *rt{nullptr}, *tmp{nullptr};
} backdrop;

bool shapeReady() {
    if(shapeShader == nullptr) shapeShader = resourceManager->createShaderAuto("uishape");
    return shapeShader != nullptr && shapeShader->isReady();
}

bool backdropReady() {
    return backdrop.built != nullptr && backdrop.built == backdrop.wanted && backdrop.rt != nullptr &&
           backdrop.rt->isReady();
}

struct UVWindow {
    f32 u0, v0, du, dv;
};

// texture coordinates in the blurred background for a screen position: the inverse of how backgrounds are drawn
// (scaled to fill the screen, centred), flipped where render targets are stored upside down
vec2 backdropUV(vec2 p) {
    const vec2 isz = backdrop.built->getSize();
    const vec2 screen = osu->getVirtScreenSize();
    const f32 scale = Osu::getImageScaleToFillResolution(backdrop.built, screen);
    vec2 uv{((p.x - screen.x * 0.5f) / scale + isz.x * 0.5f) / isz.x,
            ((p.y - screen.y * 0.5f) / scale + isz.y * 0.5f) / isz.y};
    if(g->hasFlippedTextureOrigin()) uv.y = 1.f - uv.y;
    return uv;
}

UVWindow backdropWindow(const McRect &q) {
    const vec2 a = backdropUV(q.getMin());
    const vec2 b = backdropUV(q.getMax());
    return {a.x, a.y, b.x - a.x, b.y - a.y};
}

void setParams(const McRect &quad, const Shape &s, bool textured, UVWindow uv, f32 alpha) {
    shapeShader->setUniform4f("quad", quad.getWidth(), quad.getHeight(), s.rect.getX() - quad.getX(),
                              s.rect.getY() - quad.getY());
    shapeShader->setUniform4f("shape", s.rect.getWidth(), s.rect.getHeight(), s.softness, s.border);
    shapeShader->setUniform4f("radii", s.radii[0], s.radii[1], s.radii[2], s.radii[3]);
    shapeShader->setUniform4f("extra", s.cut, textured ? 1.f : 0.f, 0.f, 0.f);
    shapeShader->setUniform4f("uvmap", uv.u0, uv.v0, uv.du, uv.dv);
    shapeShader->setUniform4f("col", 1.f, 1.f, 1.f, alpha * s.opacity);
}

// the shader treats everything as inside: for polygons and plain quads, which bring their own geometry
void setParamsUnshaped(bool textured) {
    shapeShader->setUniform4f("quad", 1.f, 1.f, -1.0e4f, -1.0e4f);
    shapeShader->setUniform4f("shape", 4.0e4f, 4.0e4f, 1.f, 0.f);
    shapeShader->setUniform4f("radii", 0.f, 0.f, 0.f, 0.f);
    shapeShader->setUniform4f("extra", 0.f, textured ? 1.f : 0.f, 0.f, 0.f);
    shapeShader->setUniform4f("uvmap", 0.f, 0.f, 1.f, 1.f);
    shapeShader->setUniform4f("col", 1.f, 1.f, 1.f, 1.f);
}

// a quad as a strip of three columns (left, `mid`, right), texcoords 0..1 across the whole quad
void drawStrip(const McRect &q, Color l, Color m, Color r, f32 mid) {
    const f32 x0 = q.getX(), y0 = q.getY(), w = q.getWidth(), h = q.getHeight();
    const f32 xm = x0 + w * mid;
    strip.clear();
    auto add = [](f32 x, f32 y, f32 u, f32 v, Color c) {
        strip.addVertex(x, y);
        strip.addColor(c);
        strip.addTexcoord(u, v);
    };
    add(x0, y0, 0.f, 0.f, l);
    add(x0, y0 + h, 0.f, 1.f, l);
    add(xm, y0, mid, 0.f, m);
    add(xm, y0 + h, mid, 1.f, m);
    add(x0 + w, y0, 1.f, 0.f, r);
    add(x0 + w, y0 + h, 1.f, 1.f, r);
    g->drawVAO(&strip);
}

// the drawn quad: the shape plus room for its soft edge
McRect padded(const Shape &s, f32 extra = 0.f) {
    const f32 pad = std::max(1.f, s.softness) + extra;
    return {s.rect.getX() - pad, s.rect.getY() - pad, s.rect.getWidth() + 2.f * pad, s.rect.getHeight() + 2.f * pad};
}

f32 midInQuad(const Shape &s, const McRect &q, f32 mid) {
    return std::clamp((s.rect.getX() + s.rect.getWidth() * mid - q.getX()) / std::max(q.getWidth(), 1.f), 0.f, 1.f);
}

void blurInto(RenderTarget *dst, RenderTarget *src, f32 stepX, f32 stepY) {
    dst->enable();
    blurShader->enable();
    blurShader->setUniform4f("blur_step", stepX, stepY, 0.f, 0.f);
    src->setColor(0xffffffff);
    src->draw(0, 0, (int)src->getWidth(), (int)src->getHeight());
    blurShader->disable();
    dst->disable();
}

void buildBackdrop(const Image *img) {
    if(blurShader == nullptr) blurShader = resourceManager->createShaderAuto("blur");
    if(blurShader == nullptr || !blurShader->isReady()) return;

    const vec2 isz = img->getSize();
    if(isz.x < 1.f || isz.y < 1.f) return;
    const int w = BACKDROP_WIDTH;
    const int h = std::clamp((int)std::lround(BACKDROP_WIDTH * isz.y / isz.x), 1, 4 * BACKDROP_WIDTH);

    if(backdrop.rt == nullptr) {
        backdrop.rt = resourceManager->createRenderTarget(0, 0, w, h);
        backdrop.tmp = resourceManager->createRenderTarget(0, 0, w, h);
    } else if((int)backdrop.rt->getWidth() != w || (int)backdrop.rt->getHeight() != h) {
        backdrop.rt->rebuild(w, h);
        backdrop.tmp->rebuild(w, h);
    }
    if(!backdrop.rt->isReady() || !backdrop.tmp->isReady()) return;

    // whatever transform the caller has, the backdrop is drawn in its own pixels
    g->pushTransform();
    Matrix4 identity;
    g->setWorldMatrix(identity);

    // the image, scaled down into the backdrop
    backdrop.rt->enable();
    g->setColor(0xffffffff);
    g->pushTransform();
    {
        g->scale((f32)w / isz.x, (f32)h / isz.y);
        g->translate((f32)w * 0.5f, (f32)h * 0.5f);
        g->drawImage(img);
    }
    g->popTransform();
    backdrop.rt->disable();

    // two rounds of horizontal and vertical gaussian passes
    for(int round = 0; round < 2; round++) {
        blurInto(backdrop.tmp, backdrop.rt, 1.f / (f32)w, 0.f);
        blurInto(backdrop.rt, backdrop.tmp, 0.f, 1.f / (f32)h);
    }
    g->popTransform();
    backdrop.built = img;
}

}  // namespace

void fill(const Shape &s, Color left, Color middle, Color right, f32 mid) {
    if(!shapeReady()) {
        g->setColor(middle);
        g->fillRect(s.rect);
        return;
    }
    const McRect q = padded(s);
    shapeShader->enable();
    setParams(q, s, false, {0.f, 0.f, 1.f, 1.f}, 1.f);
    drawStrip(q, left, middle, right, midInQuad(s, q, mid));
    shapeShader->disable();
}

void glass(const Shape &s, Color left, Color middle, Color right, f32 mid) {
    if(!backdropReady() || !shapeReady()) {
        // no backdrop yet: the tint, opaque, so the panel still reads
        fill(s, Color(left).setA(1.f), Color(middle).setA(1.f), Color(right).setA(1.f), mid);
        return;
    }
    const McRect q = padded(s);
    shapeShader->enable();
    setParams(q, s, true, backdropWindow(q), 1.f);
    backdrop.rt->bind();
    drawStrip(q, left, middle, right, midInQuad(s, q, mid));
    backdrop.rt->unbind();
    shapeShader->disable();
}

void glassPolygon(std::span<const vec2> points, Color tint) {
    if(points.size() < 3 || !shapeReady()) return;
    const bool textured = backdropReady();
    const Color c = textured ? tint : Color(tint).setA(1.f);
    shapeShader->enable();
    setParamsUnshaped(textured);
    if(textured) backdrop.rt->bind();
    fan.clear();
    for(const vec2 &p : points) {
        fan.addVertex(p.x, p.y);
        fan.addColor(c);
        fan.addTexcoord(textured ? backdropUV(p) : vec2{0.f, 0.f});
    }
    g->drawVAO(&fan);
    if(textured) backdrop.rt->unbind();
    shapeShader->disable();
}

void image(const Shape &s, const Image *img, Color tint) {
    if(img == nullptr || !img->isReady() || !shapeReady()) return;
    const vec2 isz = img->getSize();
    if(isz.x < 1.f || isz.y < 1.f || s.rect.getWidth() < 1.f || s.rect.getHeight() < 1.f) return;

    // the part of the image that fills the shape (cropped, centred), then widened to the padded quad
    const f32 shapeAspect = s.rect.getWidth() / s.rect.getHeight(), imageAspect = isz.x / isz.y;
    f32 u0 = 0.f, v0 = 0.f, du = 1.f, dv = 1.f;
    if(imageAspect > shapeAspect) {
        du = shapeAspect / imageAspect;
        u0 = (1.f - du) * 0.5f;
    } else {
        dv = imageAspect / shapeAspect;
        v0 = (1.f - dv) * 0.5f;
    }
    const McRect q = padded(s);
    const f32 perPxU = du / s.rect.getWidth(), perPxV = dv / s.rect.getHeight();
    const UVWindow uv{u0 - (s.rect.getX() - q.getX()) * perPxU, v0 - (s.rect.getY() - q.getY()) * perPxV,
                      q.getWidth() * perPxU, q.getHeight() * perPxV};

    shapeShader->enable();
    setParams(q, s, true, uv, tint.Af());
    shapeShader->setUniform4f("col", tint.Rf(), tint.Gf(), tint.Bf(), tint.Af() * s.opacity);
    img->bind();
    const Color none = argb(0.f, 1.f, 1.f, 1.f);  // no colour over the image
    drawStrip(q, none, none, none, 0.5f);
    img->unbind();
    shapeShader->disable();
}

void glow(const Shape &s, f32 spread, Color colour) {
    if(spread <= 0.f || !shapeReady()) return;
    Shape soft = s;
    soft.softness = spread * 2.f;
    soft.border = 0.f;
    const McRect q = padded(s, spread);
    shapeShader->enable();
    setParams(q, soft, false, {0.f, 0.f, 1.f, 1.f}, 1.f);
    drawStrip(q, colour, colour, colour, 0.5f);
    shapeShader->disable();
}

void glowLine(std::span<const vec2> points, const std::array<Color, 3> &grad, f32 width, Color glowColour,
              f32 glowWidth) {
    if(points.size() < 2 || !shapeReady()) return;

    f32 total = 0.f;
    for(size_t i = 1; i < points.size(); i++) total += vec::length(points[i] - points[i - 1]);
    if(total <= 0.f) return;
    auto colourAt = [&](f32 t) {
        return t < 0.5f ? UITheme::mix(grad[0], grad[1], t * 2.f) : UITheme::mix(grad[1], grad[2], (t - 0.5f) * 2.f);
    };
    // a quad from four corners, each with its colour
    auto quad = [](vec2 tl, vec2 tr, vec2 br, vec2 bl, Color ctl, Color ctr, Color cbr, Color cbl) {
        strip.clear();
        strip.addVertex(tl.x, tl.y);
        strip.addColor(ctl);
        strip.addTexcoord(0.f, 0.f);
        strip.addVertex(bl.x, bl.y);
        strip.addColor(cbl);
        strip.addTexcoord(0.f, 1.f);
        strip.addVertex(tr.x, tr.y);
        strip.addColor(ctr);
        strip.addTexcoord(1.f, 0.f);
        strip.addVertex(br.x, br.y);
        strip.addColor(cbr);
        strip.addTexcoord(1.f, 1.f);
        g->drawVAO(&strip);
    };

    shapeShader->enable();
    setParamsUnshaped(false);
    const Color glowOut = Color(glowColour).setA(0.f);
    f32 walked = 0.f;
    for(size_t i = 1; i < points.size(); i++) {
        const vec2 a = points[i - 1], b = points[i];
        const f32 len = vec::length(b - a);
        if(len <= 0.f) continue;
        const vec2 d = (b - a) / len;
        const vec2 n{-d.y, d.x};
        const f32 ta = walked / total, tb = (walked + len) / total;
        walked += len;
        const vec2 hw = n * (width * 0.5f), gw = n * (width * 0.5f + glowWidth);
        if(glowWidth > 0.f) {
            quad(a + gw, b + gw, b + hw, a + hw, glowOut, glowOut, glowColour, glowColour);
            quad(a - hw, b - hw, b - gw, a - gw, glowColour, glowColour, glowOut, glowOut);
        }
        const Color ca = colourAt(ta), cb = colourAt(tb);
        quad(a + hw, b + hw, b - hw, a - hw, ca, cb, cb, ca);
    }
    shapeShader->disable();
}

void setBackdrop(const Image *bg) {
    backdrop.wanted = (bg != nullptr && bg->isReady()) ? bg : nullptr;
    if(backdrop.wanted != nullptr && backdrop.wanted != backdrop.built) buildBackdrop(backdrop.wanted);
}

void releaseBackdrop() {
    if(backdrop.rt != nullptr) resourceManager->destroyResource(backdrop.rt);
    if(backdrop.tmp != nullptr) resourceManager->destroyResource(backdrop.tmp);
    backdrop = {};
}

}  // namespace UIDraw
