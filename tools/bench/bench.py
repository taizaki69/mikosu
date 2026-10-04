#!/usr/bin/env python3
"""Performance baseline / regression benchmark for mikosu (see docs/renovation/BASELINE.md).

Runs the game headless (offscreen rendering on the real GPU, uncapped frame rate) with -benchout, drives it
with a stdin script, and collects the FrameStats summaries into one JSON file.

scenarios:
  gameplay    a generated dense map (220 BPM streams, slider spam, jumps) played on autoplay
  songselect  song select over a real osu!stable library (--library), idle and while scrolling
  startup     launch -> first frame -> ready for input, with a warm data dir

usage:
  bench.py --game build/dist/bin-x86_64/neomod [--library ~/.local/share/osu-stable] [--renderer gl|sdlgpu]
           [--res 2560x1440] [--runs 3] [--out results.json] [scenario ...]

The user's osu! folder is only ever read (the game keeps its own databases in the bench data dir).
Each run is wrapped in tools/dev/guarded so a misbehaving build can't take the machine down.
"""

import argparse
import json
import math
import os
import shutil
import statistics
import struct
import subprocess
import sys
import tempfile
import time
import wave
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
GUARDED = ROOT / "tools" / "dev" / "guarded"

# ---------------------------------------------------------------------------------------------------------------
# reference map: deterministic, ~62 s, built to be expensive to draw and judge
# ---------------------------------------------------------------------------------------------------------------

BPM = 220.0
BEAT = 60000.0 / BPM
LEAD_IN = 1500  # ms before the first object


def _clamp(v, lo, hi):
    return max(lo, min(hi, v))


def generate_dense_map():
    objects = []
    t = LEAD_IN
    new_combo_every = 16

    # A: 0-20 s, 1/4 streams sweeping across the playfield
    count = 0
    while t < LEAD_IN + 20000:
        ang = count * 0.17
        x = 256 + 190 * math.cos(ang) * math.cos(count * 0.031)
        y = 192 + 140 * math.sin(ang)
        nc = 4 if count % new_combo_every == 0 else 0
        objects.append(f"{int(_clamp(x, 0, 512))},{int(_clamp(y, 0, 384))},{int(t)},{1 | nc},0,0:0:0:0:")
        t += BEAT / 4
        count += 1

    # B: 20-40 s, 1/2 kick sliders mixed with long curved repeat sliders every bar
    bar = 0
    while t < LEAD_IN + 40000:
        for i in range(8):
            x = 80 + (i * 53 + bar * 97) % 352
            y = 60 + (i * 41 + bar * 71) % 264
            if i == 0:
                # long bezier with 2 repeats over the bar
                pts = f"B|{x + 120}:{y - 60}|{x + 200}:{y + 80}|{x + 60}:{y + 140}"
                objects.append(f"{x},{y},{int(t)},6,0,{pts},3,240")
                t += BEAT * 2
                break
            pts = f"L|{x + 50}:{y + 20}"
            objects.append(f"{x},{y},{int(t)},{2 | (4 if i == 1 else 0)},0,{pts},1,54")
            t += BEAT / 2
        else:
            t += 0
        bar += 1
        # fill the rest of the bar with perfect-circle sliders
        for i in range(4):
            x = 100 + (i * 89 + bar * 37) % 300
            y = 90 + (i * 59 + bar * 23) % 200
            objects.append(f"{x},{y},{int(t)},2,0,P|{x + 40}:{y + 60}|{x + 90}:{y},1,140")
            t += BEAT / 2

    # C: 40-58 s, 1/2 jumps with stacks, then a spinner to the end
    count = 0
    while t < LEAD_IN + 58000:
        x = 40 if count % 2 == 0 else 470
        y = 40 + (count * 67) % 300
        nc = 4 if count % 8 == 0 else 0
        objects.append(f"{x},{y},{int(t)},{1 | nc},0,0:0:0:0:")
        if count % 6 == 5:  # 3-note stack
            for k in range(1, 3):
                objects.append(f"{x},{y},{int(t + k * BEAT / 4)},1,0,0:0:0:0:")
        t += BEAT / 2
        count += 1
    objects.append(f"256,192,{int(t)},12,0,{int(t + 3000)},0:0:0:0:")
    end = int(t + 3000)

    osu = f"""osu file format v14

[General]
AudioFilename: audio.wav
AudioLeadIn: 0
PreviewTime: {LEAD_IN}
Mode: 0

[Metadata]
Title:Dense Reference
Artist:mikosu bench
Creator:mikosu
Version:Dense
BeatmapID:0
BeatmapSetID:-1

[Difficulty]
HPDrainRate:6
CircleSize:4.2
OverallDifficulty:9
ApproachRate:9.3
SliderMultiplier:1.8
SliderTickRate:2

[Events]
0,0,"bg.png",0,0
//Break Periods

[TimingPoints]
0,{BEAT:.6f},4,2,0,70,1,0
{LEAD_IN + 20000},{BEAT:.6f},4,2,0,70,1,1
{LEAD_IN + 40000},{BEAT:.6f},4,2,0,70,1,0

[HitObjects]
""" + "\n".join(objects) + "\n"
    return osu, end, len(objects)


def write_png(path, width=1920, height=1080):
    """a deterministic diagonal colour gradient, so the map has a background to draw (no image libraries needed)"""
    import zlib

    rows = []
    for y in range(height):
        row = bytearray([0])  # filter: none
        for x in range(width):
            t = (x / width + y / height) / 2
            row += bytes((int(40 + 120 * t), int(30 + 60 * (1 - t)), int(90 + 100 * (1 - t))))
        rows.append(bytes(row))
    raw = zlib.compress(b"".join(rows), 6)

    def chunk(tag, data):
        return struct.pack(">I", len(data)) + tag + data + struct.pack(">I", zlib.crc32(tag + data) & 0xFFFFFFFF)

    png = b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", struct.pack(">IIBBBBB", width, height, 8, 2, 0, 0, 0))
    png += chunk(b"IDAT", raw) + chunk(b"IEND", b"")
    Path(path).write_bytes(png)


def write_wav(path, seconds):
    rate = 22050
    with wave.open(str(path), "wb") as w:
        w.setnchannels(1)
        w.setsampwidth(2)
        w.setframerate(rate)
        period = struct.pack("<50h", *(round(6000 * math.sin(2 * math.pi * i / 50)) for i in range(50)))
        w.writeframes(period * (rate * seconds // 50))


# ---------------------------------------------------------------------------------------------------------------
# game runs
# ---------------------------------------------------------------------------------------------------------------


def make_datadir(base, osu_folder, res):
    (base / "cfg").mkdir(parents=True, exist_ok=True)
    w, h = res.split("x")
    cfg = [
        "auto_update 0",
        f"osu_folder {osu_folder}",
        f"windowed_resolution {w}x{h}",
        "fullscreen 0",
        "fps_max 0",
        "fps_max_menu 0",
    ]
    (base / "cfg" / "osu.cfg").write_text("\n".join(cfg) + "\n")


def run_game(game, datadir, script_lines, renderer, bench_out, timeout, res):
    w, h = res.split("x")
    # the headless window is created at its final size: SDL's offscreen GL surface can't be resized later,
    # and its fake display (default 1024x768) has to be big enough (build-aux/misc/SDL3-offscreen-display-size.patch)
    cmd = [str(game), "-headless", "-multi", "-datadir", str(datadir), "-benchout", str(bench_out), "-w", w, "-h", h]
    env = dict(os.environ, SDL_VIDEO_OFFSCREEN_DISPLAY_SIZE=res)
    if renderer == "gl":
        cmd.append("-opengl")
    elif renderer == "sdlgpu":
        cmd += ["-sdlgpu", "vulkan"]
    if GUARDED.exists():
        cmd = [str(GUARDED), "--bench", "--mem", "8G", "--tasks", "1024", "--"] + cmd
    t0 = time.monotonic()
    proc = subprocess.Popen(
        cmd,
        cwd=Path(game).parent,
        env=env,
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        encoding="utf-8",
        errors="replace",
    )
    proc.stdin.write("".join(ln + "\n" for ln in script_lines))
    proc.stdin.close()
    # read the log as it streams, to timestamp events (e.g. when the beatmap database finished loading)
    lines, events = [], {}
    for line in proc.stdout:
        lines.append(line)
        if "beatmapsets from database" in line and "db_loaded_s" not in events:
            events["db_loaded_s"] = time.monotonic() - t0
        if time.monotonic() - t0 > timeout:
            proc.kill()
            break
    rc = proc.wait()
    wall = time.monotonic() - t0
    output = "".join(lines)
    if rc != 0 or not Path(bench_out).exists():
        sys.stderr.write(output[-4000:])
        raise RuntimeError(f"game run failed (exit {rc})")
    with open(bench_out) as f:
        data = json.load(f)
    data["wall_s"] = wall
    data["events"] = events
    data["log_tail"] = output[-2000:]
    return data


def seg(data, label):
    for s in data["segments"]:
        if s["label"] == label:
            return s
    raise KeyError(label)


def scenario_gameplay(args, work):
    osu, end_ms, n = generate_dense_map()
    data = work / "data-gameplay"
    if data.exists():
        shutil.rmtree(data)
    empty_osu = work / "empty-osu"
    empty_osu.mkdir(exist_ok=True)
    make_datadir(data, empty_osu, args.res)
    songs = data / "maps" / "mikosu bench - Dense Reference"
    songs.mkdir(parents=True)
    (songs / "Dense.osu").write_text(osu)
    write_wav(songs / "audio.wav", end_ms // 1000 + 3)
    write_png(songs / "bg.png")
    play_secs = 45
    script = [
        "mod_autoplay 1",
        "set_active_ui_screen songbrowser",
        "@wait_secs 4",
        "sendkey Return",
        "@wait_secs 3",  # load + lead-in
        "bench_segment gameplay",
    ]
    # synthetic key presses during play to sample the input -> present pipeline
    for _ in range(play_secs * 4):
        script += ["sendkey z", "@wait_secs 0.25"]
    script += ["bench_segment after", "exit"]
    print(f"  gameplay: {n} objects, {play_secs} s measured", flush=True)
    return run_game(args.game, data, script, args.renderer, work / "gameplay.json", args.timeout, args.res)


def scenario_songselect(args, work, warm):
    if not args.library:
        print("  songselect: skipped (no --library)", flush=True)
        return None
    data = work / "data-songselect"
    if not warm and data.exists():
        shutil.rmtree(data)
    make_datadir(data, Path(args.library).expanduser(), args.res)
    script = [
        "bench_segment load",
        "set_active_ui_screen songbrowser",
        # the first (cold) visit imports osu!.db into the game's own database
        f"@wait_secs {args.db_wait if not warm else 15}",
        "bench_segment songselect_idle",
        "@wait_secs 8",
        "bench_segment songselect_scroll_keys",
    ]
    for _ in range(80):  # walk the carousel: each step reselects, loads background/preview, recalcs stars
        script += ["sendkey Down", "@wait_secs 0.1"]
    script += ["bench_segment songselect_scroll_wheel"]
    for i in range(60):
        script += [f"mouse_wheel 0 {-3 if (i // 20) % 2 == 0 else 3}", "@wait_secs 0.1"]
    script += ["bench_segment after", "exit"]
    label = "warm" if warm else "cold"
    print(f"  songselect ({label}): library {args.library}", flush=True)
    return run_game(args.game, data, script, args.renderer, work / f"songselect-{label}.json", args.timeout,
                    args.res)


def scenario_startup(args, work):
    data = work / "data-startup"
    if data.exists():
        shutil.rmtree(data)
    empty_osu = work / "empty-osu"
    empty_osu.mkdir(exist_ok=True)
    make_datadir(data, empty_osu, args.res)
    results = []
    for i in range(args.runs + 1):  # the first run creates the data dir and is discarded
        r = run_game(args.game, data, ["bench_segment ready", "@wait_secs 1", "exit"], args.renderer,
                     work / f"startup-{i}.json", args.timeout, args.res)
        if i > 0:
            results.append({"first_frame_ms": r["startup"]["first_frame_ms"],
                            "ready_ms": seg(r, "ready")["start_s"] * 1000.0,
                            "wall_s": r["wall_s"], "peak_rss_mb": r["memory"]["peak_rss_mb"]})
    return results


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--game", type=Path, required=True)
    ap.add_argument("--library", help="osu!stable folder for the song select scenario (read only)")
    ap.add_argument("--renderer", choices=["gl", "sdlgpu", "default"], default="gl")
    ap.add_argument("--res", default="2560x1440")
    ap.add_argument("--runs", type=int, default=3, help="repetitions per scenario (median reported)")
    ap.add_argument("--db-wait", type=int, default=120, help="seconds to wait for the cold library import")
    ap.add_argument("--timeout", type=int, default=900)
    ap.add_argument("--out", type=Path, default=Path("bench-results.json"))
    ap.add_argument("--work", type=Path, help="work dir (default: a temp dir, removed afterwards)")
    ap.add_argument("scenarios", nargs="*", default=["startup", "gameplay", "songselect"])
    args = ap.parse_args()
    args.game = args.game.resolve()

    work = args.work or Path(tempfile.mkdtemp(prefix="mikosu-bench-"))
    work.mkdir(parents=True, exist_ok=True)
    game = str(args.game.relative_to(ROOT)) if args.game.is_relative_to(ROOT) else args.game.name
    out = {"game": game, "renderer": args.renderer, "res": args.res, "runs": args.runs,
           "date": time.strftime("%Y-%m-%d %H:%M:%S"), "results": {}}
    try:
        if "startup" in args.scenarios:
            print("startup", flush=True)
            out["results"]["startup"] = scenario_startup(args, work)
        if "gameplay" in args.scenarios:
            print("gameplay", flush=True)
            out["results"]["gameplay"] = [scenario_gameplay(args, work) for _ in range(args.runs)]
        if "songselect" in args.scenarios and args.library:
            print("songselect", flush=True)
            cold = scenario_songselect(args, work, warm=False)
            warm = [scenario_songselect(args, work, warm=True) for _ in range(args.runs)]
            out["results"]["songselect_cold"] = cold
            out["results"]["songselect"] = warm
    finally:
        for runs in out["results"].values():
            for r in runs if isinstance(runs, list) else [runs]:
                if isinstance(r, dict):
                    r.pop("log_tail", None)
        args.out.write_text(json.dumps(out, indent=1))
        if not args.work:
            shutil.rmtree(work, ignore_errors=True)
    print(f"wrote {args.out}")


if __name__ == "__main__":
    main()
