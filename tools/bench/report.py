#!/usr/bin/env python3
"""Summarise bench.py result files as a Markdown table (median over runs), and optionally compare against a
baseline with the regression rule from docs/renovation/BASELINE.md.

usage:
  report.py results.json [more.json ...]
  report.py --baseline docs/renovation/bench/baseline-neomod-0fedcafb-gl.json new-gl.json
"""

import argparse
import json
import statistics
import sys

SCENES = [
    ("gameplay", "gameplay", "Gameplay, dense map"),
    ("songselect", "songselect_idle", "Song select, idle"),
    ("songselect", "songselect_scroll_keys", "Song select, key scrolling"),
    ("songselect", "songselect_scroll_wheel", "Song select, wheel scrolling"),
]


def seg(run, label):
    return next(s for s in run["segments"] if s["label"] == label)


def med(values):
    values = [v for v in values if v is not None]
    return statistics.median(values) if values else None


def metric(runs, label, key, stat):
    return med([(seg(r, label).get(key) or {}).get(stat) for r in runs])


def summarize(path):
    d = json.load(open(path))
    out = {"renderer": d["renderer"], "res": d["res"], "date": d["date"], "rows": {}}
    res = d["results"]
    for key, label, name in SCENES:
        if key not in res:
            continue
        runs = res[key]
        out["rows"][label] = {
            "name": name,
            "fps": med([seg(r, label)["frames"] / max(seg(r, label)["duration_s"], 1e-9) for r in runs]),
            "cpu_p50": metric(runs, label, "cpu_frame_ms", "p50"),
            "cpu_p99": metric(runs, label, "cpu_frame_ms", "p99"),
            "cpu_max": metric(runs, label, "cpu_frame_ms", "max"),
            "gpu_p50": metric(runs, label, "gpu_frame_ms", "p50"),
            "gpu_p99": metric(runs, label, "gpu_frame_ms", "p99"),
            "lat_p50": metric(runs, label, "input_latency_ms", "p50"),
            "lat_p99": metric(runs, label, "input_latency_ms", "p99"),
            "p99_range": (min(seg(r, label)["cpu_frame_ms"]["p99"] for r in runs),
                          max(seg(r, label)["cpu_frame_ms"]["p99"] for r in runs)),
            "rss_peak": med([r["memory"]["peak_rss_mb"] for r in runs]),
        }
    if "startup" in res:
        st = res["startup"]
        out["startup"] = {
            "first_frame_ms": med([s["first_frame_ms"] for s in st]),
            "ready_ms": med([s["ready_ms"] for s in st]),
            "peak_rss_mb": med([s["peak_rss_mb"] for s in st]),
        }
    cold = res.get("songselect_cold")
    if cold:
        out["cold_import_s"] = (cold.get("events") or {}).get("db_loaded_s")
    return out


def fmt(v, digits=3):
    return "–" if v is None else f"{v:.{digits}f}"


def table(s):
    lines = [f"**{s['renderer']} at {s['res']}** (measured {s['date']}, median of runs)", "",
             "| Scene | FPS | CPU frame p50 / p99 / max (ms) | p99 spread | GPU p50 / p99 (ms) | Input→present p50 / p99 (ms) | Peak RSS (MB) |",
             "|---|---:|---:|---:|---:|---:|---:|"]
    for row in s["rows"].values():
        lo, hi = row["p99_range"]
        lines.append(
            f"| {row['name']} | {row['fps']:.0f} | {fmt(row['cpu_p50'])} / {fmt(row['cpu_p99'])} / {fmt(row['cpu_max'], 2)} "
            f"| {lo:.3f}–{hi:.3f} | {fmt(row['gpu_p50'])} / {fmt(row['gpu_p99'])} | {fmt(row['lat_p50'])} / {fmt(row['lat_p99'])} "
            f"| {fmt(row['rss_peak'], 0)} |")
    if "startup" in s:
        st = s["startup"]
        lines += ["", f"Startup (warm data dir): first frame {st['first_frame_ms']:.0f} ms, ready for input "
                      f"{st['ready_ms']:.0f} ms, peak RSS {st['peak_rss_mb']:.0f} MB."]
    if s.get("cold_import_s"):
        lines += [f"First import of the library: {s['cold_import_s']:.1f} s from launch."]
    return "\n".join(lines)


def compare(base, new):
    """the BASELINE.md rule: gameplay CPU p99 (median of runs) may not rise more than 5% above the baseline
    median, unless it stays inside the baseline's own run-to-run spread"""
    fails = []
    for label, b in base["rows"].items():
        n = new["rows"].get(label)
        if not n:
            continue
        limit = max(b["cpu_p99"] * 1.05, b["p99_range"][1])
        status = "ok" if n["cpu_p99"] <= limit else "REGRESSION"
        print(f"{b['name']:32s} p99 {b['cpu_p99']:.3f} -> {n['cpu_p99']:.3f} ms (limit {limit:.3f}) {status}")
        if status != "ok" and label == "gameplay":
            fails.append(label)
    return fails


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("files", nargs="+")
    ap.add_argument("--baseline")
    args = ap.parse_args()
    if args.baseline:
        base = summarize(args.baseline)
        bad = []
        for f in args.files:
            bad += compare(base, summarize(f))
        sys.exit(1 if bad else 0)
    for f in args.files:
        print(table(summarize(f)))
        print()


if __name__ == "__main__":
    main()
