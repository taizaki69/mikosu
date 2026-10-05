#!/usr/bin/env python3
"""Headless replay runner: plays .osr replays through mikosu's gameplay code and compares the result with what the
replay file says (judgement counts, max combo, total score), plus star rating and pp. Drives the game's
`replay_run` console command (src/App/Neomod/Tools/ReplayRunner.cpp) in one headless process.

usage:
  replay_runner.py --game build/dist/bin-x86_64/mikosu --songs ~/.local/share/osu-stable/Songs \\
                   --replays ~/.local/share/osu-stable/Data/r [--profile stable] [--out results.jsonl]
  replay_runner.py --game ... --map some.osu --replays one.osr        (a single pair, no Songs index needed)

Every .osr is matched to its beatmap by the MD5 stored in the replay. Inputs are only read; the game runs with a
throwaway data dir. Prints a summary (exact-match rates) and writes one JSON line per replay to --out.
"""

import argparse
import hashlib
import json
import os
import struct
import subprocess
import sys
import tempfile
import time
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
GUARDED = ROOT / "tools" / "dev" / "guarded"


def read_uleb(f):
    result = shift = 0
    while True:
        b = f.read(1)[0]
        result |= (b & 0x7F) << shift
        shift += 7
        if not b & 0x80:
            return result


def read_osu_string(f):
    tag = f.read(1)[0]
    if tag == 0:
        return ""
    return f.read(read_uleb(f)).decode("utf-8", "replace")


def replay_header(path):
    """(ruleset, osu_version, beatmap md5) from the start of an .osr"""
    with open(path, "rb") as f:
        mode = f.read(1)[0]
        version = struct.unpack("<i", f.read(4))[0]
        md5 = read_osu_string(f)
    return mode, version, md5


def index_songs(songs, cache_file):
    """md5 -> path for every .osu under the songs folder (cached by path, size and mtime)"""
    cache = {}
    if cache_file.exists():
        try:
            cache = json.loads(cache_file.read_text())
        except ValueError:
            cache = {}
    index, fresh = {}, {}
    for dirpath, _, files in os.walk(songs):
        for name in files:
            if not name.lower().endswith(".osu"):
                continue
            p = os.path.join(dirpath, name)
            st = os.stat(p)
            key = f"{p}|{st.st_size}|{int(st.st_mtime)}"
            md5 = cache.get(key)
            if md5 is None:
                with open(p, "rb") as fh:
                    md5 = hashlib.md5(fh.read()).hexdigest()
            fresh[key] = md5
            index[md5] = p
    cache_file.write_text(json.dumps(fresh))
    return index


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--game", type=Path, required=True)
    ap.add_argument("--songs", type=Path, help="Songs folder to find each replay's beatmap by MD5")
    ap.add_argument("--map", type=Path, help="use this beatmap for every replay instead of searching --songs")
    ap.add_argument("--replays", type=Path, nargs="+", required=True, help=".osr files or folders of them")
    ap.add_argument("--profile", default="stable")
    ap.add_argument("--out", type=Path, default=Path("replay-results.jsonl"))
    ap.add_argument("--timeout", type=int, default=1800)
    args = ap.parse_args()

    replays = []
    for r in args.replays:
        replays += sorted(r.glob("*.osr")) if r.is_dir() else [r]
    if not replays:
        sys.exit("no replays found")

    work = Path(tempfile.mkdtemp(prefix="mikosu-replays-"))
    if args.map:
        index = None
    elif args.songs:
        print(f"indexing {args.songs} ...", flush=True)
        index = index_songs(args.songs, Path(tempfile.gettempdir()) / "mikosu-songs-md5-cache.json")
        print(f"  {len(index)} beatmaps", flush=True)
    else:
        sys.exit("need --songs or --map")

    commands, skipped = [], []
    for osr in replays:
        try:
            mode, version, md5 = replay_header(osr)
        except (OSError, IndexError, struct.error) as e:
            skipped.append({"replay_path": str(osr), "error": f"unreadable replay header: {e}"})
            continue
        if mode != 0:
            skipped.append({"replay_path": str(osr), "error": f"not an osu!standard replay (ruleset {mode})"})
            continue
        beatmap = str(args.map) if args.map else index.get(md5)
        if not beatmap:
            skipped.append({"replay_path": str(osr), "error": f"beatmap {md5} not found in the Songs folder"})
            continue
        commands.append(f"replay_run {beatmap}|{osr}|{args.profile}")

    (work / "cfg").mkdir()
    (work / "cfg" / "osu.cfg").write_text(f"auto_update 0\nosu_folder {work}/no-osu\n")
    cmd = [str(args.game.resolve()), "-headless", "-multi", "-datadir", str(work)]
    if GUARDED.exists():
        cmd = [str(GUARDED), "--mem", "8G", "--tasks", "1024", "--"] + cmd
    script = ["@wait_secs 1"] + commands + ["@wait_secs 1", "exit"]
    print(f"running {len(commands)} replays ({len(skipped)} skipped) ...", flush=True)
    t0 = time.monotonic()
    proc = subprocess.run(cmd, cwd=args.game.resolve().parent, input="\n".join(script) + "\n", text=True,
                          stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=args.timeout)
    results = [json.loads(line[len("REPLAYRUN "):]) for line in proc.stdout.splitlines() if line.startswith("REPLAYRUN ")]
    results += skipped
    with open(args.out, "w") as f:
        for r in results:
            f.write(json.dumps(r) + "\n")

    ran = [r for r in results if "match" in r]
    errors = [r for r in results if "error" in r]

    def rate(key):
        n = sum(1 for r in ran if r["match"][key])
        return f"{n}/{len(ran)} ({100.0 * n / max(len(ran), 1):.1f}%)"

    print(f"done in {time.monotonic() - t0:.1f} s: {len(ran)} simulated, {len(errors)} errors -> {args.out}")
    print(f"  judgement counts exact: {rate('counts')}")
    print(f"  max combo exact:        {rate('max_combo')}")
    print(f"  total score exact:      {rate('score')}")
    for r in errors[:10]:
        print(f"  error: {Path(r.get('replay_path', '?')).name}: {r['error']}")
    if proc.returncode != 0:
        print(f"game exited with {proc.returncode}", file=sys.stderr)
        sys.stderr.write(proc.stdout[-3000:])
        sys.exit(1)


if __name__ == "__main__":
    main()
