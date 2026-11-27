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
  corpus.py score [--svg OUT] AUTOTRACE IMAGE [OPTION...]
                                         trace one image of any kind, print
                                         its metrics as a line of JSON

Needs Python 3 and ImageMagick 7 ("magick") with an SVG renderer, ideally
librsvg.  See README.md for what the metrics mean.
"""

import argparse
import json
import math
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
            # Stored images (images/NAME.png) are relative to this directory.
            args = [os.path.join(HERE, a) if a.startswith("images/") else a
                    for a in shlex.split(args)]
            images.append((name, category, sets, args))
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


# Ground truth: the shapes that the -draw arguments describe.
#
# Coordinates are autotrace's: pixel (x, y) covers the square from (x, y) to
# (x + 1, y + 1), and y points down.  ImageMagick puts pixel centres on whole
# numbers, so its point (x, y) is (x + 0.5, y + 0.5) here.  It also fills a
# shape with a margin of half a pixel: "rectangle 32,32 95,95" covers 64x64
# pixels, and a circle of radius 44 an area of about pi * 44.5^2.  So the
# edges of a filled shape lie MARGIN outside its outline, rounded at corners
# (a sharp tip doesn't grow a mitre).  The pixels can't tell a rounded corner
# from a square one, so an exact trace of a right angle is up to 0.21 pixels
# away from it.  A stroke's centre line is its path itself.

MARGIN = 0.5
# Spacing of the points sampled on the shapes and on the trace, in pixels.
STEP = 0.1


class Circle:
    def __init__(self, cx, cy, r):
        self.cx, self.cy, self.r = cx, cy, r

    def signed(self, x, y):
        """Distance to the outline, negative inside."""
        return math.hypot(x - self.cx, y - self.cy) - self.r

    def nearest(self, x, y, offset):
        """Distance to the outline moved OFFSET outwards, and the point on it
        nearest to (x, y)."""
        d = math.hypot(x - self.cx, y - self.cy)
        ux, uy = ((x - self.cx) / d, (y - self.cy) / d) if d > 0 else (1.0, 0.0)
        r = self.r + offset
        return abs(d - r), (self.cx + r * ux, self.cy + r * uy)

    def samples(self, offset):
        r = self.r + offset
        n = max(8, int(2 * math.pi * r / STEP))
        return [(self.cx + r * math.cos(2 * math.pi * k / n),
                 self.cy + r * math.sin(2 * math.pi * k / n)) for k in range(n)]


class Polyline:
    def __init__(self, points, closed):
        if closed and points[0] != points[-1]:
            points = points + [points[0]]
        self.points, self.closed = points, closed
        # (-side * dy, side * dx) is the outward normal of a segment (dx, dy):
        # outlines with a positive shoelace sum turn clockwise, y pointing down.
        area = sum(x0 * y1 - x1 * y0 for (x0, y0), (x1, y1) in zip(points, points[1:]))
        self.side = -1 if area > 0 else 1

    def signed(self, x, y):
        d = min(segment_distance(x, y, a, b) for a, b in zip(self.points, self.points[1:]))
        return -d if self.closed and self.inside(x, y) else d

    def nearest(self, x, y, offset):
        """Distance to the outline moved OFFSET outwards, and the point on it
        nearest to (x, y)."""
        # Start from the first point, for an outline of repeated points only.
        x0, y0 = self.points[0]
        best = (math.hypot(x - x0, y - y0), x0, y0, 0.0, 0.0)
        for (x0, y0), (x1, y1) in zip(self.points, self.points[1:]):
            dx, dy = x1 - x0, y1 - y0
            norm = dx * dx + dy * dy
            if norm == 0:
                continue  # a repeated point: its neighbours cover it
            t = max(0.0, min(1.0, ((x - x0) * dx + (y - y0) * dy) / norm))
            qx, qy = x0 + t * dx, y0 + t * dy
            d = math.hypot(x - qx, y - qy)
            # On a tie, prefer a segment, which has a direction.
            if d <= best[0]:
                best = (d, qx, qy, dx, dy)
        d, qx, qy, dx, dy = best
        inside = self.closed and self.inside(x, y)
        length = math.hypot(dx, dy)
        if d > 1e-9:
            # Outwards is away from (x, y) outside the shape, towards it inside.
            sign = -1 if inside else 1
            nx, ny = sign * (x - qx) / d, sign * (y - qy) / d
        elif length > 0:
            nx, ny = -self.side * dy / length, self.side * dx / length
        else:
            nx, ny = 1.0, 0.0  # on a lone point, any direction will do
        return abs((-d if inside else d) - offset), (qx + offset * nx, qy + offset * ny)

    def inside(self, x, y):
        result = False
        for (x0, y0), (x1, y1) in zip(self.points, self.points[1:]):
            if (y0 > y) != (y1 > y) and x < x0 + (y - y0) * (x1 - x0) / (y1 - y0):
                result = not result
        return result

    def samples(self, offset):
        """Points along each segment, moved OFFSET outwards."""
        out = []
        for (x0, y0), (x1, y1) in zip(self.points, self.points[1:]):
            length = math.hypot(x1 - x0, y1 - y0)
            if length == 0:
                continue
            nx, ny = -self.side * (y1 - y0) / length, self.side * (x1 - x0) / length
            n = max(1, int(length / STEP))
            for k in range(n + (0 if self.closed else 1)):
                t = k / n
                out.append((x0 + t * (x1 - x0) + offset * nx, y0 + t * (y1 - y0) + offset * ny))
        return out


def segment_distance(x, y, a, b):
    (x0, y0), (x1, y1) = a, b
    dx, dy = x1 - x0, y1 - y0
    norm = dx * dx + dy * dy
    t = 0 if norm == 0 else max(0.0, min(1.0, ((x - x0) * dx + (y - y0) * dy) / norm))
    return math.hypot(x - x0 - t * dx, y - y0 - t * dy)


# Chords of a flattened Bézier curve stay this close to it, in pixels.
FLATNESS = 0.01


def midpoint(a, b):
    return ((a[0] + b[0]) / 2, (a[1] + b[1]) / 2)


def bezier(p0, p1, p2, p3, depth=0):
    """Points on a cubic Bézier curve, without its start point, such that
    the chords between them are within FLATNESS of the curve."""
    # The curve stays within 3/4 of the control points' distance from
    # the chord.
    if depth >= 16 or max(segment_distance(p[0], p[1], p0, p3) for p in (p1, p2)) <= FLATNESS:
        return [p3]
    # Split at t = 1/2 (de Casteljau).
    a, b, c = midpoint(p0, p1), midpoint(p1, p2), midpoint(p2, p3)
    d, e = midpoint(a, b), midpoint(b, c)
    m = midpoint(d, e)
    return bezier(p0, a, d, m, depth + 1) + bezier(m, e, c, p3, depth + 1)


def draw_shape(primitive):
    """The shape of an ImageMagick -draw primitive, or None if unsupported."""
    words = primitive.replace(",", " ").split()
    kind = words[0]
    if kind == "path":
        tokens = TOKEN.findall(primitive[len("path"):])
        if not set(t for t in tokens if t.isalpha()) <= {"M", "L", "C", "Z", "z"}:
            return None
        nums = [float(t) for t in tokens if not t.isalpha()]
        cmds = [t for t in tokens if t.isalpha()]
        points, closed, i = [], False, 0
        for cmd in cmds:
            if cmd == "M":
                if points:
                    return None  # one subpath only
                points.append((nums[i], nums[i + 1]))
                i += 2
            elif cmd == "L":
                points.append((nums[i], nums[i + 1]))
                i += 2
            elif cmd == "C":
                p = [(nums[i + 2 * k], nums[i + 2 * k + 1]) for k in range(3)]
                points += bezier(points[-1], p[0], p[1], p[2])
                i += 6
            else:
                closed = True
        shape_points = points
    else:
        try:
            v = [float(w) for w in words[1:]]
        except ValueError:
            return None
        if kind == "circle" and len(v) == 4:
            return Circle(v[0] + 0.5, v[1] + 0.5, math.hypot(v[2] - v[0], v[3] - v[1]))
        if kind == "ellipse" and len(v) == 6 and (v[4], v[5]) == (0, 360):
            n = 180  # within 0.01 pixels of the ellipse
            shape_points = [(v[0] + v[2] * math.cos(2 * math.pi * k / n),
                             v[1] + v[3] * math.sin(2 * math.pi * k / n)) for k in range(n)]
            closed = True
        elif kind == "rectangle" and len(v) == 4:
            shape_points = [(v[0], v[1]), (v[2], v[1]), (v[2], v[3]), (v[0], v[3])]
            closed = True
        elif kind == "polygon" and len(v) >= 6 and len(v) % 2 == 0:
            shape_points = list(zip(v[0::2], v[1::2]))
            closed = True
        elif kind == "line" and len(v) == 4:
            shape_points = [(v[0], v[1]), (v[2], v[3])]
            closed = False
        else:
            return None
    return Polyline([(x + 0.5, y + 0.5) for x, y in shape_points], closed)


def drawn_shapes(args):
    """The shapes drawn by magick ARGS, as ("fill" or "stroke", [shapes]).

    Returns None for an image with no -draw primitive, with one that
    draw_shape() doesn't know, or with both filled and stroked ones.  Noise
    added after drawing doesn't matter: the shapes are still the truth."""
    fill, stroke = "black", "none"
    kinds, shapes = set(), []
    for option, value in zip(args, args[1:]):
        if option == "-fill":
            fill = value
        elif option == "-stroke":
            stroke = value
        elif option == "-draw":
            shape = draw_shape(value)
            if shape is None:
                return None
            if fill != "none":
                kinds.add("fill")
            if stroke != "none":
                kinds.add("stroke")
            shapes.append(shape)
    if len(kinds) != 1:
        return None
    return kinds.pop(), shapes


def trace_points(svg):
    """Points along the paths of an autotrace SVG, at most STEP apart."""
    points = []
    for d in re.findall(r'\bd="([^"]*)"', svg):
        tokens = TOKEN.findall(d)
        i, pos, start = 0, None, None
        while i < len(tokens):
            cmd = tokens[i]
            i += 1
            if cmd == "M":
                pos = start = (float(tokens[i]), float(tokens[i + 1]))
                i += 2
                points.append(pos)
                continue
            if cmd == "L":
                ends = [(float(tokens[i]), float(tokens[i + 1]))]
                i += 2
            elif cmd == "C":
                c = [(float(tokens[i + 2 * k]), float(tokens[i + 2 * k + 1])) for k in range(3)]
                ends = bezier(pos, c[0], c[1], c[2])
                i += 6
            else:
                ends = [start]
            for end in ends:
                n = max(1, int(math.hypot(end[0] - pos[0], end[1] - pos[1]) / STEP))
                points += [(pos[0] + k / n * (end[0] - pos[0]), pos[1] + k / n * (end[1] - pos[1]))
                           for k in range(1, n + 1)]
                pos = end
    return points


class PointGrid:
    """Nearest-point queries over a set of points."""

    CELL = 4.0

    def __init__(self, points):
        self.cells = {}
        for p in points:
            self.cells.setdefault((int(p[0] // self.CELL), int(p[1] // self.CELL)), []).append(p)

    def nearest(self, x, y):
        cx, cy = int(x // self.CELL), int(y // self.CELL)
        best, ring = math.inf, 0
        # Search rings of cells until no closer point can be found.
        while ring * self.CELL <= best + self.CELL and ring < 4096:
            for gx in range(cx - ring, cx + ring + 1):
                for gy in range(cy - ring, cy + ring + 1):
                    if max(abs(gx - cx), abs(gy - cy)) != ring:
                        continue
                    for px, py in self.cells.get((gx, gy), ()):
                        best = min(best, math.hypot(px - x, py - y))
            ring += 1
        return best


def shape_distances(kind, shapes, svg, width, height):
    """Mean and largest distance between the trace and the drawn shapes."""
    margin = MARGIN if kind == "fill" else 0.0

    def interior(x, y):
        # The trace of the background runs along the image border, and so
        # do the edges of shapes that reach it: leave both out.
        return min(x, y, width - x, height - y) > 1e-6

    def covered(k, x, y):
        # Is this point of shape K's edge under a later filled shape?
        return kind == "fill" and any(s.signed(x, y) < margin - 1e-6 for s in shapes[k + 1:])

    trace = [p for p in trace_points(svg) if kind != "fill" or interior(*p)]
    truth = [(x, y) for k, shape in enumerate(shapes) for x, y in shape.samples(margin)
             if interior(x, y) and not covered(k, x, y)]
    if not trace or not truth:
        return None, None
    truth_grid = PointGrid(truth)

    def to_visible_edge(x, y):
        # The exact distance to the nearest edge that isn't covered.  Where
        # the nearest point of an outline is covered, its visible part may
        # still be close: fall back to the samples of the visible edges.
        best, fallback = math.inf, False
        for k, shape in enumerate(shapes):
            d, point = shape.nearest(x, y, margin)
            if d >= best:
                continue
            if covered(k, *point):
                fallback = True
            else:
                best = d
        return min(best, truth_grid.nearest(x, y)) if fallback else best

    to_shape = [to_visible_edge(x, y) for x, y in trace]
    grid = PointGrid(trace)
    to_trace = [grid.nearest(x, y) for x, y in truth]
    mean = (sum(to_shape) / len(to_shape) + sum(to_trace) / len(to_trace)) / 2
    return round(mean, 3), round(max(max(to_shape), max(to_trace)), 3)


def background_option(options):
    """The -background-color of OPTIONS, in any spelling getopt accepts:
    one or two dashes, any abbreviation (only this option starts with b),
    and the value after = or as the next word.  The last one counts."""
    bg = None
    for i, word in enumerate(options):
        if not word.startswith("-") or word in ("-", "--"):
            continue
        name, eq, value = word.lstrip("-").partition("=")
        if name and "background-color".startswith(name.lower()):
            if not eq:
                value = options[i + 1] if i + 1 < len(options) else ""
            bg = "#" + value
    return bg


def is_centerline_svg(svg):
    """Whether SVG is a centerline trace: every path a stroke, unfilled.
    Works whichever option or preset asked for it."""
    styles = re.findall(r'<path style="([^"]*)"', svg)
    return bool(styles) and all("fill:none" in s.replace(" ", "") for s in styles)


def round_trip(png, svg_file, options, shapes=None, centerline=None):
    """Compare the rendered SVG with the input image, and with SHAPES, the
    drawn shapes from drawn_shapes(), if given.  CENTERLINE says whether
    it is a centerline trace; None takes it from OPTIONS."""
    if centerline is None:
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
    # Filled shapes are compared with an area trace, strokes with a
    # centre-line one.
    if shapes and shapes[0] == ("stroke" if centerline else "fill"):
        result["shape_mean"], result["shape_max"] = shape_distances(shapes[0], shapes[1], svg, w, h)
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


def trace(autotrace, image, options, svg_file, entry, shapes=None, repeat=1, compare=True,
          detect_centerline=False):
    """Trace IMAGE with OPTIONS into SVG_FILE, and add the status, the
    time, the SVG's statistics and, if COMPARE, the round trip to ENTRY.
    With DETECT_CENTERLINE, the SVG says whether it is a centerline trace;
    otherwise OPTIONS do."""
    best = None
    for _ in range(repeat):
        if os.path.exists(svg_file):
            os.unlink(svg_file)
        start = time.perf_counter()
        proc = subprocess.run([autotrace] + options +
                              ["-output-format", "svg", "-output-file", svg_file, image],
                              stdout=subprocess.DEVNULL, stderr=subprocess.PIPE)
        elapsed = time.perf_counter() - start
        best = elapsed if best is None else min(best, elapsed)
    entry["status"] = proc.returncode
    entry["time"] = round(best, 4)
    if proc.stderr:
        entry["stderr"] = proc.stderr.decode(errors="replace")[:500]
    if proc.returncode == 0 and not os.path.exists(svg_file):
        # For example -help or -list-presets among the options.
        entry["error"] = "autotrace wrote no SVG"
    elif proc.returncode == 0:
        with open(svg_file, encoding="utf-8") as f:
            svg = f.read()
        entry["size"] = len(svg.encode())
        entry.update(svg_stats(svg))
        if compare:
            centerline = is_centerline_svg(svg) if detect_centerline else None
            if centerline:
                entry["centerline"] = True
            entry.update(round_trip(image, svg_file, options, shapes, centerline))
    return entry


def measure(autotrace, directory, output, repeat=1):
    if not os.path.isdir(directory):
        generate(directory)
    results = []
    workdir = tempfile.mkdtemp(prefix="corpus-")
    try:
        for name, category, sets, args in read_manifest():
            png = os.path.join(directory, name + ".png")
            shapes = drawn_shapes(args)
            for options in sets:
                case = case_name(name, options)
                svg_file = os.path.join(workdir, "out.svg")
                entry = {"case": case, "category": category}
                trace(autotrace, png, options, svg_file, entry, shapes, repeat,
                      compare=category != "timing")
                results.append(entry)
                print("%-40s %s" % (case, summary(entry)), file=sys.stderr)
    finally:
        shutil.rmtree(workdir)
    with open(output, "w", encoding="utf-8") as f:
        json.dump({"autotrace": autotrace, "results": results}, f, indent=1)


def program(name):
    """NAME as a path that works from any directory: a bare name, such as
    an installed "autotrace", is looked up in PATH."""
    if os.sep in name or (os.altsep and os.altsep in name):
        return os.path.abspath(name)
    found = shutil.which(name)
    if found is None:
        sys.exit("%s: not found in PATH" % name)
    return found


def score(autotrace, image, options, keep=None):
    """Trace one IMAGE of any kind with OPTIONS and print its metrics as
    one line of JSON.  Exit with autotrace's status if it failed."""
    workdir = tempfile.mkdtemp(prefix="corpus-")
    try:
        svg_file = keep or os.path.join(workdir, "out.svg")
        entry = trace(autotrace, image, options, svg_file, {"options": options},
                      detect_centerline=True)
    finally:
        shutil.rmtree(workdir)
    print(json.dumps(entry))
    if entry["status"] != 0:
        sys.exit(entry["status"])
    if "error" in entry:
        sys.exit(1)


def summary(entry):
    keys = ["time", "paths", "nodes", "wrong", "edge_mean", "shape_mean", "shape_max",
            "stray_max", "ink_max"]
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
    ("shape_mean", "shape mean"),
    ("shape_max", "shape max"),
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
    p = sub.add_parser("score")
    p.add_argument("--svg", help="keep the trace in this file")
    p.add_argument("autotrace")
    p.add_argument("image")
    p.add_argument("options", nargs=argparse.REMAINDER,
                   help="options for autotrace, after the image")
    args = parser.parse_args()

    if args.command == "generate":
        generate(args.dir)
    elif args.command == "score":
        score(program(args.autotrace), args.image, args.options, args.svg)
    elif args.command == "measure":
        measure(program(args.autotrace), args.dir, args.output, args.repeat)
    elif args.command == "compare":
        compare(args.old, args.new, args.all)
    else:
        images = os.path.join(args.workdir, "images")
        old_json = os.path.join(args.workdir, "old.json")
        new_json = os.path.join(args.workdir, "new.json")
        # Both builds trace the same files, drawn once.
        generate(images)
        measure(program(args.old), images, old_json, args.repeat)
        measure(program(args.new), images, new_json, args.repeat)
        compare(old_json, new_json, args.all)


if __name__ == "__main__":
    main()
