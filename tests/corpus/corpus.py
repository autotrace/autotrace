#!/usr/bin/env python3
# SPDX-FileCopyrightText: © 2026 AutoTrace contributors
#
# SPDX-License-Identifier: CC0-1.0

"""Measure trace quality over a corpus of generated images (issue #341).

  corpus.py generate DIR                 draw the images of images.txt
  corpus.py measure AUTOTRACE DIR OUT    trace them, write metrics as JSON
  corpus.py compare OLD.json NEW.json    print the differences
  corpus.py run OLD_AUTOTRACE NEW_AUTOTRACE [WORKDIR]
                                         all of the above, for two builds

Needs Python 3 and ImageMagick 7 ("magick") with an SVG renderer, ideally
librsvg.  See README.md for what the metrics mean.
"""

import argparse
import json
import os
import re
import shlex
import shutil
import subprocess
import sys
import tempfile
import time

HERE = os.path.dirname(os.path.abspath(__file__))
MANIFEST = os.path.join(HERE, "images.txt")

# Two colours closer than this in every channel count as the same.  Inputs
# that are anti-aliased have shades that a trace can't reproduce exactly.
TOLERANCE = 32

# Chamfer distance weights: 3 per step along an axis, 4 diagonally.
INF = 1 << 30


def read_manifest(path=MANIFEST):
    """Return a list of (name, category, [option sets], magick args)."""
    images = []
    with open(path, encoding="utf-8") as f:
        for lineno, line in enumerate(f, 1):
            line = line.strip()
            if not line or line.startswith("#"):
                continue
            fields = [x.strip() for x in line.split("|", 3)]
            if len(fields) != 4:
                sys.exit("%s:%d: expected 4 fields separated by |" % (path, lineno))
            name, category, options, args = fields
            sets = [shlex.split(s) for s in options.split(";")]
            images.append((name, category, sets, shlex.split(args)))
    return images


def case_name(name, options):
    return name + ("[" + " ".join(options) + "]" if options else "")


def magick(args, **kw):
    return subprocess.run(["magick"] + args, check=True, stdout=subprocess.PIPE, **kw).stdout


def generate(directory):
    os.makedirs(directory, exist_ok=True)
    for name, _, _, args in read_manifest():
        out = os.path.join(directory, name + ".png")
        magick(args + [out])


# Images as (width, height, list of (r, g, b))


def load_pixels(source, background="white"):
    """Read SOURCE (a file name, svg:NAME for SVG) flattened onto BACKGROUND."""
    raw = magick(["-background", background, source, "-alpha", "remove", "-alpha", "off",
                  "-depth", "8", "-format", "%w %h\n", "-write", "info:-", "rgb:-"])
    header, data = raw.split(b"\n", 1)
    width, height = map(int, header.split())
    pixels = [tuple(data[i:i + 3]) for i in range(0, 3 * width * height, 3)]
    return width, height, pixels


def same(a, b):
    return all(abs(x - y) <= TOLERANCE for x, y in zip(a, b))


def boundary(width, height, pixels):
    """Pixels with a 4-neighbour of another colour."""
    edge = [False] * (width * height)
    for y in range(height):
        for x in range(width):
            i = y * width + x
            for j in ([i + 1] if x + 1 < width else []) + ([i + width] if y + 1 < height else []):
                if not same(pixels[i], pixels[j]):
                    edge[i] = edge[j] = True
    return edge


def distance_map(width, height, mask):
    """Approximate Euclidean distance, in pixels, to the nearest set pixel."""
    d = [0 if m else INF for m in mask]
    for y in range(height):
        for x in range(width):
            i = y * width + x
            v = d[i]
            if x > 0:
                v = min(v, d[i - 1] + 3)
            if y > 0:
                v = min(v, d[i - width] + 3)
                if x > 0:
                    v = min(v, d[i - width - 1] + 4)
                if x + 1 < width:
                    v = min(v, d[i - width + 1] + 4)
            d[i] = v
    for y in range(height - 1, -1, -1):
        for x in range(width - 1, -1, -1):
            i = y * width + x
            v = d[i]
            if x + 1 < width:
                v = min(v, d[i + 1] + 3)
            if y + 1 < height:
                v = min(v, d[i + width] + 3)
                if x + 1 < width:
                    v = min(v, d[i + width + 1] + 4)
                if x > 0:
                    v = min(v, d[i + width - 1] + 4)
            d[i] = v
    return [v / 3.0 for v in d]


def directed(dist, mask):
    """Mean and maximum of DIST over the pixels in MASK."""
    values = [dist[i] for i, m in enumerate(mask) if m]
    if not values:
        return None, None
    return sum(values) / len(values), max(values)


def symmetric(width, height, a, b):
    """Mean (Chamfer) and maximum (Hausdorff) distance between masks A, B."""
    if not any(a) or not any(b):
        return None, None
    mean_ab, max_ab = directed(distance_map(width, height, b), a)
    mean_ba, max_ba = directed(distance_map(width, height, a), b)
    return round((mean_ab + mean_ba) / 2, 3), round(max(max_ab, max_ba), 3)


NUMBER = r"-?(?:\d+\.?\d*|\.\d+)(?:[eE][-+]?\d+)?"
TOKEN = re.compile(r"[MLCZz]|" + NUMBER)


def svg_stats(svg):
    """Counts of the paths and drawing commands in an autotrace SVG."""
    stats = {"paths": 0, "subpaths": 0, "L": 0, "L_diagonal": 0, "C": 0}
    for d in re.findall(r'\bd="([^"]*)"', svg):
        stats["paths"] += 1
        tokens = TOKEN.findall(d)
        i, x, y = 0, 0.0, 0.0
        while i < len(tokens):
            cmd = tokens[i]
            i += 1
            if cmd == "M":
                x, y = float(tokens[i]), float(tokens[i + 1])
                i += 2
                stats["subpaths"] += 1
            elif cmd == "L":
                nx, ny = float(tokens[i]), float(tokens[i + 1])
                i += 2
                stats["L"] += 1
                if nx != x and ny != y:
                    stats["L_diagonal"] += 1
                x, y = nx, ny
            elif cmd == "C":
                x, y = float(tokens[i + 4]), float(tokens[i + 5])
                i += 6
                stats["C"] += 1
    stats["nodes"] = stats["L"] + stats["C"]
    return stats


def background_option(options):
    if "-background-color" in options:
        return "#" + options[options.index("-background-color") + 1]
    return None


def round_trip(png, svg_file, options):
    """Compare the rendered SVG with the input image."""
    centerline = "-centerline" in options
    bg = background_option(options) or ("#FFFFFF" if centerline else None)
    # Flatten both onto the same colour, or transparent input would differ.
    w, h, src = load_pixels(png, bg or "white")
    # crispEdges switches anti-aliasing off, or every edge pixel would differ.
    with open(svg_file, encoding="utf-8") as f:
        svg = f.read().replace("<svg ", '<svg shape-rendering="crispEdges" ', 1)
    with tempfile.NamedTemporaryFile("w", suffix=".svg", delete=False) as f:
        f.write(svg)
    try:
        rw, rh, out = load_pixels("svg:" + f.name, bg or "white")
    finally:
        os.unlink(f.name)
    if (rw, rh) != (w, h):
        return {"error": "rendered %dx%d, input %dx%d" % (rw, rh, w, h)}

    result = {}
    if centerline:
        # Strokes are one pixel wide: how far do they stray from the ink, and
        # how far is the ink from the nearest stroke?
        bgc = hex_colour(bg)
        ink = [not same(p, bgc) for p in src]
        stroke = [not same(p, bgc) for p in out]
        result["stray_mean"], result["stray_max"] = rounded(
            directed(distance_map(w, h, ink), stroke))
        result["ink_mean"], result["ink_max"] = rounded(
            directed(distance_map(w, h, stroke), ink))
        return result

    result["wrong"] = sum(1 for a, b in zip(src, out) if not same(a, b))
    # Without tolerance, for colours closer than TOLERANCE (close-colours).
    result["wrong_exact"] = sum(1 for a, b in zip(src, out) if a != b)
    if bg:
        bgc = hex_colour(bg)
        result["extra"] = sum(1 for a, b in zip(src, out) if same(a, bgc) and not same(b, bgc))
        result["missing"] = sum(1 for a, b in zip(src, out) if not same(a, bgc) and same(b, bgc))
    result["edge_mean"], result["edge_max"] = symmetric(w, h, boundary(w, h, src),
                                                        boundary(w, h, out))
    return result


def rounded(pair):
    return tuple(None if v is None else round(v, 3) for v in pair)


def hex_colour(spec):
    spec = spec.lstrip("#")
    return tuple(int(spec[i:i + 2], 16) for i in (0, 2, 4))


def measure(autotrace, directory, output, repeat=1):
    if not os.path.isdir(directory):
        generate(directory)
    results = []
    workdir = tempfile.mkdtemp(prefix="corpus-")
    try:
        for name, category, sets, _ in read_manifest():
            png = os.path.join(directory, name + ".png")
            for options in sets:
                case = case_name(name, options)
                svg_file = os.path.join(workdir, "out.svg")
                entry = {"case": case, "category": category}
                best = None
                for _ in range(repeat):
                    start = time.perf_counter()
                    proc = subprocess.run([autotrace] + options +
                                          ["-output-format", "svg", "-output-file", svg_file, png],
                                          stdout=subprocess.DEVNULL, stderr=subprocess.PIPE)
                    elapsed = time.perf_counter() - start
                    best = elapsed if best is None else min(best, elapsed)
                entry["status"] = proc.returncode
                entry["time"] = round(best, 4)
                if proc.stderr:
                    entry["stderr"] = proc.stderr.decode(errors="replace")[:500]
                if proc.returncode == 0:
                    with open(svg_file, encoding="utf-8") as f:
                        svg = f.read()
                    entry["size"] = len(svg.encode())
                    entry.update(svg_stats(svg))
                    if category != "timing":
                        entry.update(round_trip(png, svg_file, options))
                results.append(entry)
                print("%-40s %s" % (case, summary(entry)), file=sys.stderr)
    finally:
        shutil.rmtree(workdir)
    with open(output, "w", encoding="utf-8") as f:
        json.dump({"autotrace": autotrace, "results": results}, f, indent=1)


def summary(entry):
    keys = ["time", "paths", "nodes", "wrong", "edge_mean", "stray_max", "ink_max"]
    return " ".join("%s=%s" % (k, entry[k]) for k in keys if k in entry)


# The columns of the comparison: (key, heading)
COLUMNS = [
    ("time", "time s"),
    ("size", "bytes"),
    ("paths", "paths"),
    ("nodes", "nodes"),
    ("L_diagonal", "diag. L"),
    ("C", "C"),
    ("wrong", "wrong px"),
    ("wrong_exact", "exact px"),
    ("extra", "extra px"),
    ("missing", "missing px"),
    ("edge_mean", "edge mean"),
    ("edge_max", "edge max"),
    ("stray_max", "stray max"),
    ("ink_max", "ink max"),
]


def cell(old, new, key):
    if old is None and new is None:
        return ""
    if old == new:
        return str(new)
    if old is None or new is None:
        return "%s → %s" % (old, new)
    if key == "time":
        # Run time varies from run to run: report only big changes.
        if abs(new - old) < max(0.05, 0.2 * old):
            return str(new)
    return "%s → %s" % (old, new)


def compare(old_file, new_file, show_all=False):
    with open(old_file, encoding="utf-8") as f:
        old = {r["case"]: r for r in json.load(f)["results"]}
    with open(new_file, encoding="utf-8") as f:
        new_results = json.load(f)["results"]
    cols = [c for c in COLUMNS if any(c[0] in r for r in new_results)]
    print("| case | " + " | ".join(c[1] for c in cols) + " |")
    print("|---|" + "---|" * len(cols))
    changed = 0
    totals = {c[0]: [0, 0] for c in cols}
    for r in new_results:
        o = old.get(r["case"], {})
        if o.get("status", 0) != r.get("status", 0):
            print("| %s | exit status %s → %s |" % (r["case"], o.get("status"), r.get("status")))
            changed += 1
            continue
        cells = [cell(o.get(k), r.get(k), k) for k, _ in cols]
        for k, _ in cols:
            if isinstance(o.get(k), (int, float)) and isinstance(r.get(k), (int, float)):
                totals[k][0] += o[k]
                totals[k][1] += r[k]
        if show_all or any("→" in c for c in cells):
            print("| %s | %s |" % (r["case"], " | ".join(cells)))
            changed += "→" in "".join(cells)
    print("| **total** | %s |" % " | ".join(
        cell(round(totals[k][0], 3), round(totals[k][1], 3), k) for k, _ in cols))
    print("\n%d of %d cases changed." % (changed, len(new_results)))


def main():
    parser = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    sub = parser.add_subparsers(dest="command", required=True)
    p = sub.add_parser("generate")
    p.add_argument("dir")
    p = sub.add_parser("measure")
    p.add_argument("autotrace")
    p.add_argument("dir")
    p.add_argument("output")
    p.add_argument("--repeat", type=int, default=1, help="report the best of N runs")
    p = sub.add_parser("compare")
    p.add_argument("old")
    p.add_argument("new")
    p.add_argument("--all", action="store_true", help="show unchanged cases too")
    p = sub.add_parser("run")
    p.add_argument("old")
    p.add_argument("new")
    p.add_argument("workdir", nargs="?", default="corpus-work")
    p.add_argument("--repeat", type=int, default=1)
    p.add_argument("--all", action="store_true")
    args = parser.parse_args()

    if args.command == "generate":
        generate(args.dir)
    elif args.command == "measure":
        measure(os.path.abspath(args.autotrace), args.dir, args.output, args.repeat)
    elif args.command == "compare":
        compare(args.old, args.new, args.all)
    else:
        images = os.path.join(args.workdir, "images")
        old_json = os.path.join(args.workdir, "old.json")
        new_json = os.path.join(args.workdir, "new.json")
        # Both builds trace the same files, drawn once.
        generate(images)
        measure(os.path.abspath(args.old), images, old_json, args.repeat)
        measure(os.path.abspath(args.new), images, new_json, args.repeat)
        compare(old_json, new_json, args.all)


if __name__ == "__main__":
    main()
