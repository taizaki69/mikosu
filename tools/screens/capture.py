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
ALL_SCREENS = ["mainmenu", "mainmenu-open", "options", "songselect", "modselect", "gameplay", "pause", "ranking"]

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


def make_library(work):
    maps = work / "maps"
    for si, (artist, title, creator, bg, diffs) in enumerate(lib.SETS):
        d = maps / f"{artist} - {title}"
        d.mkdir(parents=True)
        shutil.copy(ART / bg, d / bg)
        lib.write_wav(d / "audio.wav", 60)
        for di, (version, od, ar) in enumerate(diffs):
            (d / f"{artist} - {title} ({creator}) [{version}].osu").write_text(
                lib.osu_file(artist, title, creator, version, od, ar, bg, di + si))
    d = maps / f"Paper Lanterns - {QUICK_TITLE}"
    d.mkdir(parents=True)
    shutil.copy(ART / "bg-twilight.png", d / "bg-twilight.png")
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
    if "mainmenu-open" in want:
        s += [f"mouse_to {w // 2} {h // 2}", "@wait_secs 0.3", "mouse_down left", "mouse_up left", "@wait_secs 1.5"]
        shot("mainmenu-open")
        s += ["mouse_to 5 5", "@wait_secs 0.2"]
    if "options" in want:
        s += ["set_active_ui_screen optionsoverlay", "@wait_secs 1.5"]
        shot("options")
        s += ["sendkey Escape", "@wait_secs 1"]
    if want & {"songselect", "modselect", "gameplay", "pause", "ranking"}:
        s += ["set_active_ui_screen songbrowser", "@wait_secs 4", "sendkey End", "@wait_secs 1", "sendkey Up",
              "@wait_secs 2"]
        shot("songselect")
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
    args = ap.parse_args()
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
