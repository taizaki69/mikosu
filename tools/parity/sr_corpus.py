#!/usr/bin/env python3
"""Star rating parity against the official calculator (osu-tools PerformanceCalculator, the oracle) over a sample of a
local osu! library. Read only: the sampled .osu files are symlinked into a scratch folder, never copied or changed.

usage:
  sr_corpus.py --songs ~/.local/share/osu-stable/Songs --work /tmp/sr-corpus [--count 300] [--mods NM,HD,HR,DT,EZ]

needs: tools/diffcalc built (tools/diffcalc/build/diffcalc) and the oracle built (see CLAUDE.md: ref/osu-tools).
Writes <work>/report.json and prints a summary: per mod combination, how many maps are within 1e-4 / 1e-3 / 1e-2
stars of the oracle, the worst ones, and which skill (aim, speed, reading, flashlight) differs most.
"""

import argparse
import json
import os
import random
import statistics
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
DIFFCALC = ROOT / "tools" / "diffcalc" / "build" / "diffcalc"
ORACLE = ROOT / "ref" / "osu-tools" / "PerformanceCalculator" / "bin" / "Release" / "net10.0" / "PerformanceCalculator"
GUARD = ROOT / "tools" / "dev" / "guarded"
# (name, oracle attribute, mikosu diffcalc attribute)
SKILLS = [("aim", "aim_difficulty", "aimDifficulty"), ("speed", "speed_difficulty", "speedDifficulty"),
          ("reading", "reading_difficulty", "readingDifficulty"),
          ("flashlight", "flashlight_difficulty", "flashlightDifficulty")]


def metadata(path):
    """(mode, artist, title, creator, version) of an .osu file"""
    meta, mode = {}, "0"
    with open(path, encoding="utf-8-sig", errors="replace") as f:
        for line in f:
            line = line.strip()
            if line.startswith("[HitObjects]"):
                break
            if ":" in line:
                k, v = line.split(":", 1)
                if k in ("Artist", "Title", "Creator", "Version"):
                    meta[k] = v.strip()
                elif k == "Mode":
                    mode = v.strip()
    return mode, meta.get("Artist", ""), meta.get("Title", ""), meta.get("Creator", ""), meta.get("Version", "")


def guarded(cmd):
    return ([str(GUARD), "--mem", "8G", "--tasks", "1024", "--"] if GUARD.exists() else []) + cmd


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--songs", type=Path, required=True)
    ap.add_argument("--work", type=Path, required=True)
    ap.add_argument("--count", type=int, default=300)
    ap.add_argument("--mods", default="NM,HD,HR,DT,EZ")
    ap.add_argument("--seed", type=int, default=1)
    args = ap.parse_args()
    for tool in (DIFFCALC, ORACLE):
        if not tool.exists():
            sys.exit(f"missing {tool}")

    # sample osu!standard difficulties whose display name is unique (the oracle reports maps by name)
    files = sorted(p for p in args.songs.rglob("*.osu"))
    by_name = {}
    for p in files:
        mode, artist, title, creator, version = metadata(p)
        if mode != "0":
            continue
        name = f"{artist} - {title} ({creator}) [{version}]"
        by_name.setdefault(name, []).append(p)
    unique = sorted((n, ps[0]) for n, ps in by_name.items() if len(ps) == 1)
    random.Random(args.seed).shuffle(unique)
    sample = sorted(unique[: args.count])
    corpus = args.work / "corpus"
    corpus.mkdir(parents=True, exist_ok=True)
    for old in corpus.iterdir():
        old.unlink()
    for i, (name, p) in enumerate(sample):
        (corpus / f"{i:04d}.osu").symlink_to(p)
    print(f"{len(sample)} of {len(unique)} uniquely named osu!standard difficulties", flush=True)

    mods = [m.strip().upper() for m in args.mods.split(",") if m.strip()]
    results = {}
    for m in mods:
        # mikosu: rates aren't mods for diffcalc (DT = 1.5x)
        dc_mod, speed = {"DT": ("NM", "1.5"), "HT": ("NM", "0.75")}.get(m, (m, "1.0"))
        listing = args.work / "list.txt"
        listing.write_text("\n".join(str(corpus / f"{i:04d}.osu") for i in range(len(sample))) + "\n")
        out = args.work / f"diffcalc-{m}.jsonl"
        subprocess.run(guarded([str(DIFFCALC), "batch", "--list", str(listing), "-o", str(out), "--mods", dc_mod,
                                "--speeds", speed]), check=True, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        ours = {}
        for line in out.read_text().splitlines():
            r = json.loads(line)
            if "error" in r:
                continue
            idx = int(Path(r["map"]).stem)
            attrs = r.get("attrs", {})
            ours[idx] = {"stars": r["stars"]["total"] if isinstance(r["stars"], dict) else r["stars"],
                         **{k: attrs.get(a) for k, _, a in SKILLS}}

        oracle_args = [str(ORACLE), "difficulty", str(corpus), "-j"] + (["-m", m.lower()] if m != "NM" else [])
        proc = subprocess.run(guarded(oracle_args), capture_output=True, text=True)
        oracle_json = json.loads(proc.stdout[proc.stdout.index("{"):])
        theirs = {}
        for r in oracle_json["results"]:
            name = r["beatmap"]
            match = [i for i, (n, _) in enumerate(sample) if n == name]
            if len(match) == 1:
                a = r["attributes"]
                theirs[match[0]] = {"stars": a["star_rating"], **{k: a.get(key) for k, key, _ in SKILLS}}
        results[m] = {"ours": ours, "theirs": theirs}
        print(f"{m}: mikosu {len(ours)} maps, oracle {len(theirs)} maps", flush=True)

    report = {"count": len(sample), "mods": {}}
    for m, r in results.items():
        diffs = []
        for i, t in r["theirs"].items():
            o = r["ours"].get(i)
            if o is None:
                continue
            diffs.append((abs(o["stars"] - t["stars"]), i, o, t))
        if not diffs:
            continue
        diffs.sort(reverse=True)
        d = [x[0] for x in diffs]
        skill_med = {}
        for k, _, _ in SKILLS:
            vals = [abs((o[k] or 0) - (t[k] or 0)) for _, _, o, t in diffs if o.get(k) is not None and t.get(k) is not None]
            if vals:
                skill_med[k] = statistics.median(vals)
        report["mods"][m] = {
            "n": len(d), "within_1e-4": sum(x < 1e-4 for x in d), "within_1e-3": sum(x < 1e-3 for x in d),
            "within_1e-2": sum(x < 1e-2 for x in d), "median": statistics.median(d), "max": d[0],
            "skill_median_abs_diff": skill_med,
            "worst": [{"map": sample[i][0], "ours": o["stars"], "oracle": t["stars"]} for _, i, o, t in diffs[:5]],
        }
        s = report["mods"][m]
        print(f"{m:>3}: {s['within_1e-4']}/{s['n']} within 1e-4, {s['within_1e-3']} within 1e-3, "
              f"{s['within_1e-2']} within 1e-2; median {s['median']:.2e}, max {s['max']:.4f}; "
              f"skill medians {', '.join(f'{k} {v:.1e}' for k, v in skill_med.items())}")
    (args.work / "report.json").write_text(json.dumps(report, indent=1))


if __name__ == "__main__":
    main()
