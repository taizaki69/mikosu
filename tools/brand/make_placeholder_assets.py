#!/usr/bin/env python3
"""Generates mikosu's placeholder brand assets (until the real logo is chosen):
  assets/materials/mikosu.png  main-menu wordmark (white "mikosu" with a dark outline, 1100x500, LA)
  assets/icon.ico              Windows app icon (flat ring logo: approach ring with a gap, hit dot), 16-256 px
Original artwork, drawn with Outfit (SIL OFL 1.1). Needs Pillow; fonts from docs/renovation/mockups/src/get-fonts.sh."""
import math
from pathlib import Path

from PIL import Image, ImageDraw, ImageFont

ROOT = Path(__file__).resolve().parents[2]
FONT = ROOT / "docs/renovation/mockups/src/fonts/Outfit-wght.ttf"
ACCENT = (255, 125, 133)  # the chosen design's default coral accent
DARK = (18, 19, 26)


def wordmark():
    w, h = 1100, 500
    img = Image.new("LA", (w, h), (0, 0))
    font = ImageFont.truetype(str(FONT), 300)
    font.set_variation_by_axes([700])
    d = ImageDraw.Draw(img)
    box = d.textbbox((0, 0), "mikosu", font=font, stroke_width=14)
    x = (w - (box[2] - box[0])) // 2 - box[0]
    y = (h - (box[3] - box[1])) // 2 - box[1]
    d.text((x, y), "mikosu", font=font, fill=(245, 255), stroke_width=14, stroke_fill=(50, 255))
    img.save(ROOT / "assets/materials/mikosu.png", optimize=True)


def ring(size):
    s = size * 4  # draw large, downsample for smooth edges
    img = Image.new("RGBA", (s, s), (0, 0, 0, 0))
    d = ImageDraw.Draw(img)
    c = s / 2
    r_disc = s * 0.40
    d.ellipse([c - r_disc, c - r_disc, c + r_disc, c + r_disc], fill=DARK + (255,))
    r = s * 0.40
    width = max(4, int(s * 0.09))
    # arc from the top clockwise through 300 degrees, leaving a gap at the upper left (PIL angles: 0 = 3 o'clock)
    d.arc([c - r, c - r, c + r, c + r], start=-90, end=210, fill=ACCENT + (255,), width=width)
    a = math.radians(210)
    dot = s * 0.07
    px, py = c + math.cos(a) * (r - width / 2), c + math.sin(a) * (r - width / 2)
    d.ellipse([px - dot, py - dot, px + dot, py + dot], fill=(255, 255, 255, 255))
    return img.resize((size, size), Image.LANCZOS)


def icon():
    sizes = [16, 24, 32, 48, 64, 128, 256]
    big = ring(256)
    big.save(ROOT / "assets/icon.ico", sizes=[(n, n) for n in sizes])


if __name__ == "__main__":
    wordmark()
    icon()
    print("wrote assets/materials/mikosu.png and assets/icon.ico")
