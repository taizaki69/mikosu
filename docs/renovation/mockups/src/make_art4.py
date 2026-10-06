#!/usr/bin/env python3
"""Round-4 art: a bright, anime-style sky over the sea (cumulus clouds lit by the sun, light rays, a sun path on the
water, sparkles) as the 'beatmap background', and set thumbnails at other times of day. Original and generated with
fixed seeds, so no beatmap content is used. Needs numpy + Pillow. Writes into art/ (git-ignored)."""

import numpy as np
from PIL import Image, ImageFilter

from make_art import OUT, save


def smoothstep(a, b, x):
    t = np.clip((x - a) / (b - a), 0, 1)
    return t * t * (3 - 2 * t)


def value_noise(h, w, cells_y, rng, stretch=1.0):
    """smooth noise: a random grid upsampled bicubically; stretch > 1 makes features wider than tall"""
    cells_x = max(2, int(round(cells_y * w / h / stretch)))
    grid = rng.random((cells_y + 1, cells_x + 1)).astype(np.float32)
    return np.asarray(Image.fromarray(grid, mode="F").resize((w, h), Image.BICUBIC), np.float32)


def fbm(h, w, rng, base=3, octaves=6, stretch=1.0, gain=0.5):
    out = np.zeros((h, w), np.float32)
    amp, total = 1.0, 0.0
    for o in range(octaves):
        out += value_noise(h, w, base * 2 ** o, rng, stretch) * amp
        total += amp
        amp *= gain
    return out / total


def blur(a, r):
    """gaussian blur (sigma r) of a float array, through the FFT, with edge padding"""
    if r <= 0:
        return a.astype(np.float32)
    h, w = a.shape
    p = int(3 * r) + 1
    ap = np.pad(a.astype(np.float64), p, mode="edge")
    fy = np.fft.fftfreq(ap.shape[0])[:, None]
    fx = np.fft.rfftfreq(ap.shape[1])[None, :]
    g = np.exp(-2 * np.pi ** 2 * r ** 2 * (fx ** 2 + fy ** 2))
    out = np.fft.irfft2(np.fft.rfft2(ap) * g, s=ap.shape)
    return out[p:p + h, p:p + w].astype(np.float32)


def vgrad(h, w, stops):
    """vertical gradient through (position, rgb) stops, clamped at both ends"""
    ys = np.linspace(0, 1, h, dtype=np.float32)
    pos = [p for p, _ in stops]
    cols = np.stack([np.interp(ys, pos, [c[k] for _, c in stops]) for k in range(3)], -1).astype(np.float32)
    return np.repeat(cols[:, None, :], w, axis=1)


def glow(img, cx, cy, r, colour, strength, power=2.0):
    h, w, _ = img.shape
    yy, xx = np.mgrid[0:h, 0:w].astype(np.float32)
    d = np.sqrt((xx - cx) ** 2 + (yy - cy) ** 2) / r
    g = np.exp(-(d ** power)) * strength
    img += g[..., None] * np.array(colour, np.float32)


def scene(w, h, seed, pal, sun=(0.74, 0.2), horizon=0.7, cloudiness=0.0, night=False):
    rng = np.random.default_rng(seed)
    hz = int(h * horizon)
    img = vgrad(h, w, pal["sky"])
    sx, sy = sun[0] * w, sun[1] * h

    # sun (or moon) and its haze
    glow(img, sx, sy, h * 0.6, pal["haze"], 0.22, 1.2)
    glow(img, sx, sy, h * 0.07, pal["sunglow"], 0.45)
    glow(img, sx, sy, h * 0.022, (1, 1, 1), 1.4, 3)

    if night:
        stars = rng.random((h, w)) > 0.9985
        tw = rng.random((h, w)).astype(np.float32)
        img += (stars * tw * (1 - smoothstep(0.1, horizon, np.linspace(0, 1, h)[:, None])))[..., None] * 0.9

    # cumulus: density from stretched fbm, bigger banks towards the horizon, thin wisps up high
    dens = fbm(h, w, rng, base=2, octaves=7, stretch=1.7)
    dens = (dens - dens.mean()) / (dens.std() + 1e-6)
    ys = np.linspace(0, 1, h, dtype=np.float32)[:, None]
    bank = smoothstep(0.25, horizon - 0.08, ys) * 0.9 - smoothstep(horizon - 0.04, horizon, ys) * 3.0
    d = dens + bank - 1.05 + cloudiness * 4
    cover = smoothstep(0.0, 0.55, d)
    # light: density falls towards the sun on the lit side
    off = int(h * 0.012)
    dx, dy = (off if sx >= w / 2 else -off), -off
    shifted = np.roll(np.roll(d, -dy, 0), -dx, 1)
    lit = np.clip(0.6 + (d - shifted) * 1.6, 0, 1)
    lit = blur(lit, 1.2)
    # cloud bottoms are darker, tops brighter
    under = blur(np.clip((d - np.roll(d, -off * 2, 0)) * 0.8, 0, 1), 2)
    ccol = pal["cloud_shadow"][None, None, :] * (1 - lit[..., None]) + pal["cloud_lit"][None, None, :] * lit[..., None]
    ccol = ccol * (1 - 0.18 * np.clip(under, 0, 1))[..., None]
    # rim light on edges facing the sun
    rim = np.clip(smoothstep(0.0, 0.12, d) - smoothstep(0.12, 0.4, d), 0, 1) * lit
    ccol += rim[..., None] * pal["rim"][None, None, :] * 0.35
    a = blur(cover, h * 0.0006)[..., None] * (ys < horizon)[..., None]
    img = img * (1 - a) + ccol * a

    # god rays from the sun
    yy, xx = np.mgrid[0:h, 0:w].astype(np.float32)
    ang = np.arctan2(yy - sy, xx - sx)
    segs = rng.random(73).astype(np.float32)
    k = np.interp((ang + np.pi) / (2 * np.pi) * 72, np.arange(73), segs)
    dist = np.sqrt((xx - sx) ** 2 + (yy - sy) ** 2) / h
    rays = k ** 4 * np.exp(-dist * 2.4) * (1 - a[..., 0] * 0.7) * (yy < hz)
    img += blur(rays, h * 0.006)[..., None] * np.array(pal["sunglow"], np.float32) * 0.12

    # sea: gradient, horizontal ripples, the sun path, glints
    sea = vgrad(h - hz, w, pal["sea"])
    rip = fbm(h - hz, w, rng, base=10, octaves=5, stretch=8.0)
    persp = np.linspace(0.25, 1.0, h - hz, dtype=np.float32)[:, None]
    sea *= (0.86 + 0.28 * (rip - 0.5) * persp)[..., None]
    pathw = (0.03 + 0.18 * np.linspace(0, 1, h - hz, dtype=np.float32)[:, None]) * w
    path = np.exp(-((np.arange(w, dtype=np.float32)[None, :] - sx) ** 2) / (2 * pathw ** 2))
    fine = fbm(h - hz, w, rng, base=28, octaves=3, stretch=5.0)
    sparkle = smoothstep(0.62, 0.74, fine) * path * (0.4 + 0.6 * persp)
    sea += (path * 0.22)[..., None] * np.array(pal["sunglow"], np.float32)
    sea += (sparkle * 1.2)[..., None]
    img[hz:] = sea
    # horizon haze
    glow_band = np.exp(-((np.arange(h, dtype=np.float32) - hz) / (h * 0.012)) ** 2)[:, None, None]
    img += glow_band * np.array(pal["haze"], np.float32) * 0.1

    # floating glints
    for _ in range(int(w * h / 110000)):
        gx, gy = rng.uniform(0, w), rng.uniform(0, h)
        r = rng.uniform(1.5, 5) * h / 1080
        glow(img, gx, gy, r, (1, 1, 1), rng.uniform(0.15, 0.5), 2)

    # bloom
    bright = np.clip(img.mean(-1) - 0.95, 0, 1)
    img += blur(bright, h * 0.02)[..., None] * 0.35
    img += rng.normal(0, 0.008, (h, w))[..., None].astype(np.float32)
    return img


PALETTES = {
    "sky": dict(sky=[(0, (0.28, 0.52, 0.92)), (0.45, (0.56, 0.77, 1.0)), (0.68, (0.84, 0.92, 1.0)), (0.7, (1.0, 0.92, 0.94))],
                sea=[(0, (0.55, 0.78, 0.98)), (1, (0.13, 0.40, 0.78))],
                haze=(1.0, 0.93, 0.95), sunglow=(1.0, 0.95, 0.85),
                cloud_lit=np.array((1.0, 0.99, 0.97)), cloud_shadow=np.array((0.70, 0.76, 0.92)), rim=np.array((1.0, 0.95, 0.85))),
    "dusk": dict(sky=[(0, (0.22, 0.24, 0.55)), (0.4, (0.70, 0.45, 0.70)), (0.62, (1.0, 0.62, 0.62)), (0.7, (1.0, 0.80, 0.60))],
                 sea=[(0, (0.85, 0.55, 0.60)), (1, (0.25, 0.18, 0.40))],
                 haze=(1.0, 0.75, 0.6), sunglow=(1.0, 0.75, 0.5),
                 cloud_lit=np.array((1.0, 0.82, 0.75)), cloud_shadow=np.array((0.55, 0.42, 0.65)), rim=np.array((1.0, 0.8, 0.5))),
    "night": dict(sky=[(0, (0.03, 0.04, 0.14)), (0.5, (0.12, 0.12, 0.34)), (0.7, (0.32, 0.26, 0.52))],
                  sea=[(0, (0.18, 0.18, 0.38)), (1, (0.03, 0.04, 0.12))],
                  haze=(0.6, 0.55, 0.9), sunglow=(0.85, 0.88, 1.0),
                  cloud_lit=np.array((0.55, 0.56, 0.78)), cloud_shadow=np.array((0.16, 0.16, 0.34)), rim=np.array((0.8, 0.85, 1.0))),
    "sakura": dict(sky=[(0, (0.98, 0.70, 0.84)), (0.5, (1.0, 0.86, 0.92)), (0.7, (1.0, 0.95, 0.96))],
                   sea=[(0, (0.95, 0.78, 0.88)), (1, (0.62, 0.48, 0.78))],
                   haze=(1.0, 0.9, 0.95), sunglow=(1.0, 0.95, 0.95),
                   cloud_lit=np.array((1.0, 0.97, 0.98)), cloud_shadow=np.array((0.90, 0.72, 0.84)), rim=np.array((1.0, 0.9, 0.95))),
    "mint": dict(sky=[(0, (0.30, 0.75, 0.78)), (0.5, (0.62, 0.92, 0.88)), (0.7, (0.92, 1.0, 0.96))],
                 sea=[(0, (0.55, 0.88, 0.84)), (1, (0.08, 0.45, 0.50))],
                 haze=(0.92, 1.0, 0.96), sunglow=(1.0, 1.0, 0.92),
                 cloud_lit=np.array((1.0, 1.0, 0.98)), cloud_shadow=np.array((0.65, 0.84, 0.84)), rim=np.array((1.0, 1.0, 0.9))),
    "storm": dict(sky=[(0, (0.18, 0.20, 0.36)), (0.5, (0.42, 0.42, 0.62)), (0.7, (0.78, 0.72, 0.86))],
                  sea=[(0, (0.50, 0.50, 0.68)), (1, (0.12, 0.14, 0.28))],
                  haze=(0.85, 0.8, 0.95), sunglow=(1.0, 0.92, 0.9),
                  cloud_lit=np.array((0.92, 0.90, 0.98)), cloud_shadow=np.array((0.36, 0.36, 0.54)), rim=np.array((1.0, 0.9, 0.9))),
}


def main():
    OUT.mkdir(parents=True, exist_ok=True)
    bg = scene(1920, 1080, 41, PALETTES["sky"], sun=(0.72, 0.16))
    save(bg, OUT / "bg-sky.png")
    # the frosted-glass layer: the background blurred once (as the game would cache it), slightly brightened
    blurred = np.stack([blur(bg[..., k], 26) for k in range(3)], -1)
    save(blurred * 0.96 + 0.04, OUT / "bg-sky-blur.png")
    suns = {"sky": (0.6, 0.2), "dusk": (0.35, 0.62), "night": (0.7, 0.18), "sakura": (0.3, 0.25), "mint": (0.75, 0.3), "storm": (0.4, 0.3)}
    for i, (name, pal) in enumerate(PALETTES.items()):
        save(scene(640, 360, 200 + i, pal, sun=suns[name], night=(name == "night"), cloudiness=0.04 if name == "storm" else 0.0),
             OUT / f"thumb4-{name}.png")
    print("round-4 art written to", OUT)


if __name__ == "__main__":
    main()
