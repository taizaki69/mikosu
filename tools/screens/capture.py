#!/usr/bin/env python3
"""Screenshots of mikosu's screens at several resolutions, headless, for checking the redesign (workstream F).

Each resolution is one headless launch that walks through the screens with a generated library (the mockups'
original artwork, never user content) and saves <out>/<screen>@<WxH>[-ui2x].png:

  mainmenu, mainmenu-open, options, songselect, modselect, gameplay, pause, ranking

usage:
  capture.py --game build/dist/bin-x86_64/mikosu --out /tmp/shots
             [--res 1280x720,1920x1080,2560x1440,3440x1440] [--ui2x 2560x1440] [--screens mainmenu,songselect]
             [--fps 60] [--renderer gl|sdlgpu] [--set 'ui_theme moon']

--fps caps the frame rate (headless_fps_max; 0 = uncapped) so captures stay light on a PC that's busy with
something else. The artwork comes from docs/renovation/mockups/src/art (python3 make_art.py there creates it).
"""

import argparse
import os
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import capture_before as lib  # the generated library: SETS, osu_file(), write_wav()

ROOT = Path(__file__).resolve().parents[2]
ART = ROOT / "docs" / "renovation" / "mockups" / "src" / "art"
ALL_SCREENS = ["mainmenu", "mainmenu-open", "mainmenu-hover", "options", "songselect", "songselect-hover", "modselect",
               "gameplay", "pause", "ranking"]

# a short map for the gameplay, pause and ranking screens (found by its title through song select's search)
QUICK_TITLE = "Quickstep Test"
QUICK_MAP = """osu file format v14

[General]
AudioFilename: audio.wav
AudioLeadIn: 0
Mode: 0

[Metadata]
Title:{title}
Artist:Paper Lanterns
Creator:kazehana
Version:Normal
BeatmapID:0
BeatmapSetID:-1

[Difficulty]
HPDrainRate:3
CircleSize:4
OverallDifficulty:5
ApproachRate:7
SliderMultiplier:1.4
SliderTickRate:1

[Events]
0,0,"bg-twilight.png",0,0

[TimingPoints]
0,400,4,2,0,70,1,0

[HitObjects]
{objects}
"""


def quick_map():
    objs = []
    t = 800
    for i in range(14):
        x, y = 120 + (i * 53) % 280, 110 + (i * 71) % 170
        if i % 5 == 4:
            objs.append(f"{x},{y},{t},2,0,B|{x + 90}:{y}|{x + 120}:{y + 40},1,140")
            t += 800
        else:
            objs.append(f"{x},{y},{t},{5 if i == 0 else 1},0,0:0:0:0:")
            t += 400
    return QUICK_MAP.format(title=QUICK_TITLE, objects="\n".join(objs))


# the round-4/6 mockups' art (make_art4.py) under the names the generated maps use, so in-game captures compare
# directly with the mockups; ROUND1_ART keeps the first rounds' bokeh and skylines
ART4 = {"bg-twilight.png": "bg-sky.png", "thumb-rose.png": "thumb4-sakura.png", "thumb-ocean.png": "thumb4-mint.png",
        "thumb-moss.png": "thumb4-storm.png", "thumb-teal.png": "thumb4-sky.png", "thumb-ember.png": "thumb4-dusk.png"}
ROUND1_ART = False


def art_for(name):
    if not ROUND1_ART and (ART / ART4.get(name, name)).exists():
        return ART / ART4.get(name, name)
    return ART / name


# made-up players for the leaderboards (the first is the capture's own name, so its rows show as the player's)
PLAYERS = [("taizaki", 8 | 16), ("mochi_m", 8), ("ren_k", 0), ("solstice", 16), ("Velvet", 0), ("kitsu", 64),
           ("hoshino", 0)]


def osu_string(text):
    raw = text.encode()
    out, n = bytearray([0x0B]), len(raw)
    while True:
        byte, n = n & 0x7F, n >> 7
        out.append(byte | (0x80 if n else 0))
        if not n:
            return bytes(out) + raw


def write_scores_db(path, maps):
    """an osu!stable scores.db with a few made-up scores on each generated map (md5, object count)"""
    import struct
    import time
    now_ticks = (int(time.time()) + 62135596800) * 10_000_000
    out = bytearray(struct.pack("<II", 20240101, len(maps)))
    for mi, (md5, objects) in enumerate(maps):
        out += osu_string(md5) + struct.pack("<I", len(PLAYERS))
        for pi, (name, mods) in enumerate(PLAYERS):
            n100, n50, miss = pi * 3 + mi % 3, pi // 2, pi // 3
            n300 = max(objects - n100 - n50 - miss, 0)
            combo = max(objects * 2 - pi * 37, 10)
            score = max(12_948_210 - pi * 1_100_000 - mi * 31_337, 10_000)
            days = [2, 7, 21, 33, 60, 150, 240][pi]
            out += struct.pack("<BI", 0, 20240101) + osu_string(md5) + osu_string(name) + b"\x00"
            out += struct.pack("<HHHHHHiHBI", n300, n100, n50, 0, 0, miss, score, combo, 0, mods)
            out += b"\x00" + struct.pack("<qiq", now_ticks - days * 86400 * 10_000_000, -1, 0)
    path.write_bytes(bytes(out))


def make_library(work):
    import hashlib
    maps = work / "maps"
    scored = []
    for si, (artist, title, creator, bg, diffs) in enumerate(lib.SETS):
        d = maps / f"{artist} - {title}"
        d.mkdir(parents=True)
        shutil.copy(art_for(bg), d / bg)
        lib.write_wav(d / "audio.wav", 60)
        for di, (version, od, ar) in enumerate(diffs):
            text = lib.osu_file(artist, title, creator, version, od, ar, bg, di + si)
            (d / f"{artist} - {title} ({creator}) [{version}].osu").write_text(text)
            objects = len(text.split("[HitObjects]")[1].strip().splitlines())
            scored.append((hashlib.md5(text.encode()).hexdigest(), objects))
    write_scores_db(work / "empty-osu" / "scores.db", scored)
    d = maps / f"Paper Lanterns - {QUICK_TITLE}"
    d.mkdir(parents=True)
    shutil.copy(art_for("bg-twilight.png"), d / "bg-twilight.png")
    lib.write_wav(d / "audio.wav", 15)
    (d / f"Paper Lanterns - {QUICK_TITLE} (kazehana) [Normal].osu").write_text(quick_map())


def script(w, h, screens, fps, ui2x, settings=()):
    """the stdin script for one launch: every wanted screen in a fixed order, each step setting up the next"""
    want = set(screens)
    # no fps counter or build stamp in the corner: they aren't part of the design being checked
    s = [f"headless_fps_max {fps}", "mod_autoplay 0", "draw_fps 0", "draw_runtime_info 0", *settings]
    if ui2x:
        s.append("ui_scale 2")
    s.append("@wait_secs 3")

    def shot(name):
        if name in want:
            s.extend([f"take_screenshot shots/{name}.png", "@wait_secs 0.5"])

    shot("mainmenu")
    if want & {"mainmenu-open", "mainmenu-hover"}:
        s += [f"mouse_to {w // 2} {h // 2}", "@wait_secs 0.3", "mouse_down left", "mouse_up left", "@wait_secs 1.5"]
        shot("mainmenu-open")
        # the first button, hovered (where it sits once the menu is open, right of the logo)
        s += [f"mouse_to {int(w * 0.66)} {int(h * 0.35)}", "@wait_secs 0.6"]
        shot("mainmenu-hover")
        s += ["mouse_to 5 5", "@wait_secs 0.2"]
    if "options" in want:
        s += ["set_active_ui_screen optionsoverlay", "@wait_secs 1.5"]
        shot("options")
        s += ["sendkey Escape", "@wait_secs 1"]
    if want & {"songselect", "modselect", "gameplay", "pause", "ranking"}:
        # the generated set with the most difficulties (and made-up scores), found by typing its title; Escape
        # clears the search and keeps the selection, as in stable
        s += ["set_active_ui_screen songbrowser", "@wait_secs 4", "sendtext Lanternfall", "@wait_secs 2",
              "sendkey Escape", "@wait_secs 2"]
        shot("songselect")
        if "songselect-hover" in want:
            # a card below the selection, hovered
            s += [f"mouse_to {int(w * 0.8)} {int(h * 0.62)}", "@wait_secs 0.8"]
            shot("songselect-hover")
            s += ["mouse_to 5 5", "@wait_secs 0.3"]
        if "modselect" in want:
            s += ["sendkey F1", "@wait_secs 1.5"]
            shot("modselect")
            s += ["sendkey F1", "@wait_secs 1"]
        if want & {"gameplay", "pause", "ranking"}:
            s += ["mod_autoplay 1", f"sendtext {QUICK_TITLE}", "@wait_secs 2", "sendkey Return", "@wait_secs 2.5"]
            shot("gameplay")
            if "pause" in want:
                s += ["sendkey Escape", "@wait_secs 1"]
                shot("pause")
                s += ["sendkey Escape", "@wait_secs 1"]
            s += ["@wait_secs 9"]
            shot("ranking")
    s += ["@wait_secs 0.5", "exit"]
    return "\n".join(s) + "\n"


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--game", type=Path, required=True)
    ap.add_argument("--out", type=Path, required=True)
    ap.add_argument("--res", default="1280x720,1920x1080,2560x1440,3440x1440")
    ap.add_argument("--ui2x", default="2560x1440", help="resolutions to also capture at 2x UI scale ('' = none)")
    ap.add_argument("--screens", default=",".join(ALL_SCREENS))
    ap.add_argument("--fps", type=int, default=60, help="frame cap while capturing (0 = uncapped)")
    ap.add_argument("--renderer", choices=["gl", "sdlgpu"], default="gl")
    ap.add_argument("--set", action="append", default=[], metavar="COMMAND",
                    help="a console command run before capturing, e.g. 'ui_theme moon' (repeatable)")
    ap.add_argument("--round1-art", action="store_true", help="the first mockup rounds' artwork instead of round 4's")
    args = ap.parse_args()
    global ROUND1_ART
    ROUND1_ART = args.round1_art
    if not ART.is_dir():
        sys.exit(f"{ART} is missing: run python3 make_art.py in {ART.parent} first")
    screens = [s for s in args.screens.split(",") if s]
    unknown = set(screens) - set(ALL_SCREENS)
    if unknown:
        sys.exit(f"unknown screens: {', '.join(sorted(unknown))} (known: {', '.join(ALL_SCREENS)})")

    runs = [(r, False) for r in args.res.split(",") if r] + [(r, True) for r in args.ui2x.split(",") if r]
    args.out.mkdir(parents=True, exist_ok=True)
    guard = ROOT / "tools" / "dev" / "guarded"
    failed = False
    for res, ui2x in runs:
        w, h = (int(v) for v in res.split("x"))
        work = Path(tempfile.mkdtemp(prefix="mikosu-shots-"))
        try:
            (work / "cfg").mkdir()
            (work / "empty-osu").mkdir()
            (work / "cfg" / "osu.cfg").write_text(f"auto_update 0\nosu_folder {work}/empty-osu\nname taizaki\n")
            make_library(work)
            cmd = [str(args.game.resolve()), "-headless", "-multi", "-datadir", str(work), "-w", str(w), "-h", str(h)]
            if args.renderer == "gl":
                cmd.append("-opengl")
            if guard.exists():
                cmd = [str(guard), "--mem", "6G", "--tasks", "512", "--"] + cmd
            env = dict(os.environ, SDL_VIDEO_OFFSCREEN_DISPLAY_SIZE=res)
            proc = subprocess.run(cmd, cwd=args.game.resolve().parent, env=env, input=script(w, h, screens, args.fps, ui2x, args.set),
                                  text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=300)
            suffix = f"@{res}" + ("-ui2x" if ui2x else "")
            got = 0
            for name in screens:
                src = work / "shots" / f"{name}.png"
                if src.exists():
                    shutil.copy(src, args.out / f"{name}{suffix}.png")
                    got += 1
            print(f"{res}{' ui2x' if ui2x else ''}: {got}/{len(screens)} screens", flush=True)
            if got != len(screens) or proc.returncode != 0:
                failed = True
                print(proc.stdout[-2000:], file=sys.stderr)
        finally:
            shutil.rmtree(work, ignore_errors=True)
    sys.exit(1 if failed else 0)


if __name__ == "__main__":
    main()
