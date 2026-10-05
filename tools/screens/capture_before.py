#!/usr/bin/env python3
"""Captures the game's current main menu and song select headless, with a small generated library that uses the
mockups' original artwork (no user content), for side-by-side comparison with the design mockups.

usage: capture_before.py --game build/dist/bin-x86_64/mikosu --out docs/renovation/screens/before [--res 1920x1080]
"""

import argparse
import math
import os
import shutil
import struct
import subprocess
import tempfile
import wave
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
ART = ROOT / "docs" / "renovation" / "mockups" / "src" / "art"

SETS = [
    ("Aurora Drift", "Lanternfall", "kazehana", "bg-twilight.png", [("Easy", 3, 2), ("Normal", 4, 4), ("Hard", 6, 6),
                                                                 ("Insane", 7, 8), ("Kaze's Extra", 8, 9)]),
    ("Lumen Kid", "Velvet Static", "ponytail", "thumb-rose.png", [("Normal", 4, 4), ("Hard", 6, 7)]),
    ("Mareen", "Glass Harbor", "Shiro", "thumb-ocean.png", [("Hard", 6, 7)]),
    ("The Quiet Engine", "Moss & Mirrors", "Oak", "thumb-moss.png", [("Insane", 7, 8)]),
    ("Kaori Nanase", "Paper Planes in June", "Shibuki", "thumb-teal.png", [("Normal", 4, 5)]),
    ("Juno Park", "Fireline Theory", "Mirelle", "thumb-ember.png", [("Hard", 6, 7)]),
]


def osu_file(artist, title, creator, version, od, ar, bg, seed):
    objs = []
    t = 1000
    for i in range(120 + seed * 15):
        ang = i * (0.3 + seed * 0.05)
        x = int(256 + 180 * math.cos(ang))
        y = int(192 + 130 * math.sin(ang * 1.3))
        if i % 8 == 0:
            objs.append(f"{x},{y},{t},6,0,B|{min(x + 80, 500)}:{y}|{min(x + 120, 500)}:{max(y - 60, 0)},1,140")
            t += 600
        else:
            objs.append(f"{x},{y},{t},{5 if i % 16 == 1 else 1},0,0:0:0:0:")
            t += 300 - seed * 20
    return f"""osu file format v14

[General]
AudioFilename: audio.wav
PreviewTime: 2000
Mode: 0

[Metadata]
Title:{title}
Artist:{artist}
Creator:{creator}
Version:{version}
BeatmapID:0
BeatmapSetID:-1

[Difficulty]
HPDrainRate:5
CircleSize:4
OverallDifficulty:{od}
ApproachRate:{ar}
SliderMultiplier:1.6
SliderTickRate:1

[Events]
0,0,"{bg}",0,0

[TimingPoints]
0,333.333,4,2,0,70,1,0

[HitObjects]
""" + "\n".join(objs) + "\n"


def write_wav(path, seconds):
    rate = 22050
    with wave.open(str(path), "wb") as w:
        w.setnchannels(1)
        w.setsampwidth(2)
        w.setframerate(rate)
        period = struct.pack("<50h", *(round(5000 * math.sin(2 * math.pi * i / 50)) for i in range(50)))
        w.writeframes(period * (rate * seconds // 50))


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--game", type=Path, required=True)
    ap.add_argument("--out", type=Path, required=True)
    ap.add_argument("--res", default="1920x1080")
    args = ap.parse_args()
    w, h = args.res.split("x")
    work = Path(tempfile.mkdtemp(prefix="mikosu-before-"))
    try:
        (work / "cfg").mkdir(parents=True)
        (work / "empty-osu").mkdir()
        (work / "cfg" / "osu.cfg").write_text(f"auto_update 0\nosu_folder {work}/empty-osu\nname taizaki\n")
        for si, (artist, title, creator, bg, diffs) in enumerate(SETS):
            d = work / "maps" / f"{artist} - {title}"
            d.mkdir(parents=True)
            shutil.copy(ART / bg, d / bg)
            write_wav(d / "audio.wav", 60)
            for di, (version, od, ar) in enumerate(diffs):
                (d / f"{artist} - {title} ({creator}) [{version}].osu").write_text(
                    osu_file(artist, title, creator, version, od, ar, bg, di + si))
        script = [
            "@wait_secs 3",
            "take_screenshot before-mainmenu-idle.png",
            f"mouse_to {int(w) // 2} {int(h) // 2}",
            "@wait_secs 0.5",
            "mouse_down left",
            "mouse_up left",
            "@wait_secs 1.5",
            "take_screenshot before-mainmenu.png",
            "set_active_ui_screen songbrowser",
            "@wait_secs 4",
            "sendkey End",
            "@wait_secs 1",
            "sendkey Up",
            "@wait_secs 2",
            "take_screenshot before-songselect.png",
            "@wait_secs 1",
            "exit",
        ]
        cmd = [str(args.game.resolve()), "-headless", "-multi", "-datadir", str(work), "-opengl", "-w", w, "-h", h]
        guard = ROOT / "tools" / "dev" / "guarded"
        if guard.exists():
            cmd = [str(guard), "--mem", "6G", "--tasks", "512", "--"] + cmd
        env = dict(os.environ, SDL_VIDEO_OFFSCREEN_DISPLAY_SIZE=args.res)
        subprocess.run(cmd, cwd=args.game.resolve().parent, env=env, input="\n".join(script) + "\n", text=True,
                       stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, timeout=180, check=True)
        args.out.mkdir(parents=True, exist_ok=True)
        for png in work.glob("before-*.png"):
            shutil.copy(png, args.out / png.name)
            print("captured", args.out / png.name)
    finally:
        shutil.rmtree(work, ignore_errors=True)


if __name__ == "__main__":
    main()
