#!/usr/bin/env python3
"""compare.py RUN_A RUN_B [--scene NAME] [--steps N]: the status traces of two scripted runs
(run_scenario.sh output folders), aligned at the first logic step of a scene.

The scene is --scene or the ALIGN environment variable (Title, Map, CourseTask, ...).
Each run is aligned on its log line "[test] align at logic step N scene=NAME", or on the first
status.txt line of that scene. Step numbers are rebased to that step (0 = the align step), then
the status lines are compared step by step: scene, map, course, pack and player position.
Exit status: 0 identical, 1 differs (the first differing step is printed), 2 no align step.
"""
import argparse
import os
import re
import sys

FIELDS = ("scene", "map", "course", "pack", "x", "y", "z")
MARKER = re.compile(r"\[test\] align at logic step (\d+) scene=(\S+)")


def read_lines(path):
    if not os.path.exists(path):
        return []
    with open(path, errors="replace") as f:
        return f.read().splitlines()


def parse_status(lines):
    rows = []
    for line in lines:
        parts = line.split()
        if len(parts) != 1 + len(FIELDS):
            continue
        try:
            step = int(parts[0])
        except ValueError:
            continue
        rows.append((step, tuple(parts[1:])))
    return rows


def align_step(run, scene):
    wanted = scene.lower()
    for line in read_lines(os.path.join(run, "log")):
        m = MARKER.search(line)
        if m and m.group(2).lower() == wanted:
            return int(m.group(1))
    for step, row in parse_status(read_lines(os.path.join(run, "status.txt"))):
        if row[0].lower() == wanted:
            return step
    return None


def rebased(run, origin, steps=None):
    out = {}
    for step, row in parse_status(read_lines(os.path.join(run, "status.txt"))):
        rel = step - origin
        if rel < 0 or (steps is not None and rel > steps):
            continue
        out[rel] = row
    return out


def describe(row):
    if row is None:
        return "(no line)"
    return " ".join("%s=%s" % kv for kv in zip(FIELDS, row))


def compare(run_a, run_b, scene, steps=None):
    origin_a, origin_b = align_step(run_a, scene), align_step(run_b, scene)
    report = ["align %s: %s step %s, %s step %s" % (scene, run_a, origin_a, run_b, origin_b)]
    if origin_a is None or origin_b is None:
        report.append("no align step (the run never reached scene %s)" % scene)
        return 2, report
    a, b = rebased(run_a, origin_a, steps), rebased(run_b, origin_b, steps)
    if not a or not b:
        report.append("no status lines at or after the align step")
        return 2, report
    last = min(max(a), max(b))
    keys = sorted(k for k in set(a) | set(b) if k <= last)
    bad = [k for k in keys if a.get(k) != b.get(k)]
    if not bad:
        report.append("status  %d steps, identical" % len(keys))
        return 0, report
    first = bad[0]
    report.append("status  %d steps, %d differ, first at step %d" % (len(keys), len(bad), first))
    report.append("   A: %s" % describe(a.get(first)))
    report.append("   B: %s" % describe(b.get(first)))
    names = [n for n, x, y in zip(FIELDS, a.get(first, ()), b.get(first, ())) if x != y]
    if names:
        report.append("   differs in: %s" % ", ".join(names))
    return 1, report


def main(argv=None):
    p = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    p.add_argument("run_a")
    p.add_argument("run_b")
    p.add_argument("--scene", default=os.environ.get("ALIGN", "Title"))
    p.add_argument("--steps", type=int, default=None)
    args = p.parse_args(argv)
    code, report = compare(args.run_a, args.run_b, args.scene, args.steps)
    print("\n".join(report))
    return code


if __name__ == "__main__":
    sys.exit(main())
