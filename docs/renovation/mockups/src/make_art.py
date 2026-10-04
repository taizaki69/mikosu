#!/usr/bin/env python3
"""Generates the original, license-free artwork the mockups use: a twilight 'beatmap background' and a
few small set thumbnails in other palettes. Deterministic (fixed seeds). Needs numpy + Pillow."""

import math
from pathlib import Path

import numpy as np
from PIL import Image, ImageFilter

OUT = Path(__file__).resolve().parent / "art"


def lerp(a, b, t):
    return a + (b - a) * t


def gradient(h, w, stops):
    """vertical gradient through (pos, rgb) stops"""
    ys = np.linspace(0, 1, h)[:, None]
    img = np.zeros((h, w, 3), np.float32)
    for (p0, c0), (p1, c1) in zip(stops, stops[1:]):
        m = (ys >= p0) & (ys <= p1)
        t = np.clip((ys - p0) / max(p1 - p0, 1e-6), 0, 1)
        for k in range(3):
            img[..., k] = np.where(m, lerp(c0[k], c1[k], t), img[..., k])
    return img


def add_glow(img, cx, cy, radius, color, strength):
    h, w, _ = img.shape
    yy, xx = np.mgrid[0:h, 0:w]
    d = np.sqrt((xx - cx) ** 2 + (yy - cy) ** 2) / radius
    g = np.exp(-d * d) * strength
    for k in range(3):
        img[..., k] += g * color[k]


def bokeh(img, rng, n, rmin, rmax, palette, ymin=0.0, ymax=1.0):
    h, w, _ = img.shape
    layer = np.zeros_like(img)
    yy, xx = np.mgrid[0:h, 0:w]
    for _ in range(n):
        r = rng.uniform(rmin, rmax)
        cx, cy = rng.uniform(0, w), rng.uniform(ymin * h, ymax * h)
        col = palette[rng.integers(len(palette))]
        a = rng.uniform(0.05, 0.22)
        x0, x1 = int(max(cx - r - 2, 0)), int(min(cx + r + 2, w))
        y0, y1 = int(max(cy - r - 2, 0)), int(min(cy + r + 2, h))
        if x1 <= x0 or y1 <= y0:
            continue
        d = np.sqrt((xx[y0:y1, x0:x1] - cx) ** 2 + (yy[y0:y1, x0:x1] - cy) ** 2)
        disc = np.clip(r - d, 0, 1.5) / 1.5  # soft edge
        rim = np.exp(-((d - r * 0.92) ** 2) / (r * 0.02 + 1)) * 0.6  # bright rim like real bokeh
        m = (disc * 0.7 + rim * disc) * a
        for k in range(3):
            layer[y0:y1, x0:x1, k] += m * col[k]
    pil = Image.fromarray(np.clip(layer * 255, 0, 255).astype(np.uint8)).filter(ImageFilter.GaussianBlur(1.2))
    img += np.asarray(pil, np.float32) / 255


def skyline(img, rng, base, color, jag, seed_off=0):
    """a soft ridge / rooftop silhouette band"""
    h, w, _ = img.shape
    xs = np.arange(w)
    ridge = np.zeros(w)
    for f, a in ((0.0021, 60), (0.0063, 24), (0.019, 8)):
        ridge += a * np.sin(xs * f + rng.uniform(0, 6.28) + seed_off)
    # a few blocky buildings
    x = 0
    while x < w:
        bw = int(rng.uniform(20, 70))
        bh = rng.uniform(0, jag)
        ridge[x:x + bw] -= bh
        x += bw + int(rng.uniform(0, 30))
    top = (base * h + ridge).astype(int)
    for xi in range(w):
        t0 = max(min(top[xi], h), 0)
        img[t0:, xi, :] = img[t0:, xi, :] * 0.25 + np.array(color) * 0.75


def grain(img, rng, amount):
    img += rng.normal(0, amount, img.shape[:2])[..., None].astype(np.float32)


def save(img, path, size=None):
    pil = Image.fromarray(np.clip(img * 255, 0, 255).astype(np.uint8))
    if size:
        pil = pil.resize(size, Image.LANCZOS)
    path.parent.mkdir(parents=True, exist_ok=True)
    pil.save(path, optimize=True)


def twilight(w=2560, h=1440, seed=7, hue=None):
    rng = np.random.default_rng(seed)
    stops = hue or [
        (0.00, (0.06, 0.05, 0.18)),
        (0.38, (0.24, 0.10, 0.36)),
        (0.62, (0.78, 0.30, 0.42)),
        (0.74, (0.98, 0.58, 0.40)),
        (1.00, (0.20, 0.08, 0.20)),
    ]
    img = gradient(h, w, stops)
    add_glow(img, w * 0.62, h * 0.70, h * 0.22, (1.0, 0.75, 0.45), 0.55)
    add_glow(img, w * 0.62, h * 0.70, h * 0.60, (0.9, 0.35, 0.55), 0.25)
    bokeh(img, rng, 70, h * 0.01, h * 0.06, [(1, .8, .9), (.7, .8, 1), (1, .9, .6)], 0.05, 0.7)
    skyline(img, rng, 0.80, (0.10, 0.04, 0.14), 80)
    skyline(img, rng, 0.88, (0.05, 0.02, 0.08), 40, 2.0)
    # stars in the upper sky
    for _ in range(260):
        x, y = int(rng.uniform(0, w)), int(rng.uniform(0, h * 0.35))
        img[y, x] += rng.uniform(0.2, 0.7)
    grain(img, rng, 0.012)
    return img


def main():
    OUT.mkdir(parents=True, exist_ok=True)
    save(twilight(), OUT / "bg-twilight.png")
    palettes = {
        "teal": [(0, (0.02, .12, .16)), (.6, (0.05, .45, .50)), (1, (0.70, .90, .80))],
        "ember": [(0, (0.10, .03, .02)), (.6, (0.70, .25, .08)), (1, (1.0, .80, .40))],
        "rose": [(0, (0.20, .04, .14)), (.6, (0.85, .35, .55)), (1, (1.0, .85, .90))],
        "moss": [(0, (0.03, .08, .04)), (.6, (0.30, .55, .25)), (1, (0.90, .95, .70))],
        "ocean": [(0, (0.02, .03, .12)), (.6, (0.15, .30, .75)), (1, (0.70, .85, 1.0))],
        "violet": [(0, (0.06, .02, .12)), (.6, (0.45, .20, .80)), (1, (0.95, .80, 1.0))],
    }
    for i, (name, stops) in enumerate(palettes.items()):
        rng = np.random.default_rng(100 + i)
        w, h = 640, 360
        img = gradient(h, w, stops)
        add_glow(img, w * rng.uniform(.3, .7), h * .7, h * .5, (1, .95, .85), .35)
        bokeh(img, rng, 30, 4, 26, [(1, 1, 1), (1, .9, .8)], 0.05, 0.8)
        skyline(img, rng, 0.82, tuple(c * .25 for c in stops[0][1]), 30)
        grain(img, rng, 0.01)
        save(img, OUT / f"thumb-{name}.png")
    print("art written to", OUT)


if __name__ == "__main__":
    main()
