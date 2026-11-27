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
        if "-corpus-jpeg" in args:
            # Not a magick option: save the image as a JPEG of this
            # quality and read it back, compression artefacts and all.
            i = args.index("-corpus-jpeg")
            jpeg = magick(args[:i] + args[i + 2:] + ["-quality", args[i + 1], "jpeg:-"])
            magick(["jpeg:-", "PNG24:" + out], input=jpeg)
        else:
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


# The outline itself: its turns, straight parts and specks (#443, after
# the metrics of VectorCraft's tracer)

# An anchor that turns by this much is a corner; by KINK, a kink unless a
# true corner is near.
CORNER_TURN = 20.0
KINK_TURN = 10.0
# How near (px) a traced corner must be to a true one to keep it, and how
# far a kink must be from any true corner.
CORNER_NEAR = 1.5
KINK_FAR = 2.0
# Closed subpaths smaller than this (px of area) are specks.
SPECK_AREA = 6.0


def subpaths(svg):
    """The subpaths of an autotrace SVG, as (segments, closed); one that
    ends where it starts is closed, with or without a "z".  Each
    segment is a cubic (p0, p1, p2, p3); a line is one with its control
    points at its ends.  Segments of no length are left out."""
    result = []
    for d in re.findall(r'\bd="([^"]*)"', svg):
        tokens = TOKEN.findall(d)
        i, pos, start, segs = 0, None, None, None
        while i < len(tokens):
            cmd = tokens[i]
            i += 1
            if cmd == "M":
                if segs:
                    result.append((segs, segs[0][0] == segs[-1][3]))
                pos = start = (float(tokens[i]), float(tokens[i + 1]))
                i += 2
                segs = []
                continue
            if segs is None:
                # A drawing command right after "z" starts a new subpath
                # where the last one started.
                start, segs = pos, []
            if cmd == "L":
                end = (float(tokens[i]), float(tokens[i + 1]))
                i += 2
                seg = (pos, pos, end, end)
            elif cmd == "C":
                c = [(float(tokens[i + 2 * k]), float(tokens[i + 2 * k + 1])) for k in range(3)]
                i += 6
                seg = (pos, c[0], c[1], c[2])
            else:
                seg = (pos, pos, start, start)
            if len(set(seg)) > 1:
                segs.append(seg)
            pos = seg[3]
            if cmd in "Zz":
                if segs:
                    result.append((segs, True))
                segs = None
        if segs:
            result.append((segs, segs[0][0] == segs[-1][3]))
    return result


def turn_angle(a, b):
    """The angle in degrees between directions A and B."""
    return abs(math.degrees(math.atan2(a[0] * b[1] - a[1] * b[0], a[0] * b[0] + a[1] * b[1])))


def first_direction(points):
    """The direction from the first of POINTS to the next one apart from it."""
    x0, y0 = points[0]
    for x, y in points[1:]:
        if math.hypot(x - x0, y - y0) > 1e-9:
            return (x - x0, y - y0)
    return None


def anchor_turns(paths):
    """(anchor, degrees it turns by) for each anchor of PATHS, from
    subpaths(), that has a segment on both sides."""
    turns = []
    for segs, closed in paths:
        for k in range(0 if closed else 1, len(segs)):
            before, after = segs[k - 1], segs[k]
            a = first_direction(before[::-1])
            b = first_direction(after)
            if a is not None and b is not None:
                turns.append((after[0], turn_angle((-a[0], -a[1]), b)))
    return turns


def flatten(seg):
    """Points along a cubic segment, its start included."""
    p0, p1, p2, p3 = seg
    if p1 == p0 and p2 == p3:
        return [p0, p3]
    return [p0] + bezier(p0, p1, p2, p3)


def is_straight(seg):
    p0, p1, p2, p3 = seg
    return segment_distance(*p1, p0, p3) < 0.02 and segment_distance(*p2, p0, p3) < 0.02


def outline_stats(svg, width, height):
    """The share of the outline that is straight, and level or upright;
    the specks; how far the trace reaches outside the image.  Edges along
    the image border, which every background has, don't count."""
    length = straight = upright = 0.0
    specks, outside = 0, 0.0
    for segs, closed in subpaths(svg):
        points = []
        for seg in segs:
            flat = flatten(seg)
            points += flat[:-1]
            for x, y in flat:
                outside = max(outside, -x, -y, x - width, y - height)
            seg_length = sum(math.hypot(b[0] - a[0], b[1] - a[1]) for a, b in zip(flat, flat[1:]))
            (x0, y0), (x1, y1) = seg[0], seg[3]
            if is_straight(seg) and ((x0 == x1 and x0 in (0, width)) or
                                     (y0 == y1 and y0 in (0, height))):
                continue
            length += seg_length
            if is_straight(seg):
                straight += seg_length
                if x0 == x1 or y0 == y1:
                    upright += seg_length
        if closed:
            ring = zip(points, points[1:] + points[:1])
            area = sum(xa * yb - xb * ya for (xa, ya), (xb, yb) in ring)
            specks += abs(area) / 2 < SPECK_AREA
    result = {"specks": specks, "outside": round(outside, 3)}
    if length > 0:
        result["lines"] = round(100 * straight / length, 1)
        result["upright"] = round(100 * upright / length, 1)
    return result


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
        if not out:
            # A lone point, such as a one-pixel rectangle: moved OFFSET
            # outwards, it is a circle, as nearest() treats it.
            x0, y0 = self.points[0]
            out = Circle(x0, y0, 0.0).samples(offset) if offset > 0 else [(x0, y0)]
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


def true_corners(kind, shapes, width, height):
    """The sharp corners of the drawn SHAPES that show: vertices where an
    outline turns by CORNER_TURN or more, inside the image and not under
    a later filled shape."""
    margin = MARGIN if kind == "fill" else 0.0
    corners = []
    for k, shape in enumerate(shapes):
        if not isinstance(shape, Polyline):
            continue
        points = [p for i, p in enumerate(shape.points) if i == 0 or p != shape.points[i - 1]]
        if shape.closed and len(points) > 1 and points[0] == points[-1]:
            points.pop()
        n = len(points)
        for i in range(n) if shape.closed else range(1, n - 1):
            (xa, ya), (x, y), (xc, yc) = points[i - 1], points[i], points[(i + 1) % n]
            if turn_angle((x - xa, y - ya), (xc - x, yc - y)) < CORNER_TURN:
                continue
            # A filled shape's corner within the margin of the border is
            # traced on the border, where anchors don't count.
            if min(x, y, width - x, height - y) <= margin + 1e-6:
                continue
            if kind == "fill" and any(s.signed(x, y) < margin - 1e-6 for s in shapes[k + 1:]):
                continue
            corners.append((x, y))
    return corners


def corner_outline(shape):
    """SHAPE's outline as a list of points; a circle as a polygon with
    sides of about a pixel, close enough to find where it meets others."""
    if isinstance(shape, Circle):
        n = max(16, int(2 * math.pi * shape.r))
        points = [(shape.cx + shape.r * math.cos(2 * math.pi * k / n),
                   shape.cy + shape.r * math.sin(2 * math.pi * k / n)) for k in range(n)]
        return points + points[:1]
    return shape.points


def crossing(a, b, c, d):
    """Where segments AB and CD meet, ends included, or None."""
    rx, ry = b[0] - a[0], b[1] - a[1]
    sx, sy = d[0] - c[0], d[1] - c[1]
    den = rx * sy - ry * sx
    if abs(den) < 1e-12:
        return None  # parallel: no single point, and no turn
    qx, qy = c[0] - a[0], c[1] - a[1]
    t = (qx * sy - qy * sx) / den
    u = (qx * ry - qy * rx) / den
    if -1e-9 <= t <= 1 + 1e-9 and -1e-9 <= u <= 1 + 1e-9:
        return (a[0] + t * rx, a[1] + t * ry)
    return None


def junctions(kind, shapes):
    """The points where the outlines of two drawn SHAPES meet or cross
    and show: where one filled shape cuts another, or where strokes meet.
    A trace turns there, but no primitive has a vertex there."""
    outlines = [corner_outline(s) for s in shapes]
    boxes = [(min(x for x, _ in o), min(y for _, y in o), max(x for x, _ in o),
              max(y for _, y in o)) for o in outlines]
    found = []
    for k in range(len(shapes)):
        for j in range(k + 1, len(shapes)):
            a, b = boxes[k], boxes[j]
            if a[0] > b[2] + 1 or b[0] > a[2] + 1 or a[1] > b[3] + 1 or b[1] > a[3] + 1:
                continue
            for p, q in zip(outlines[k], outlines[k][1:]):
                for r, s in zip(outlines[j], outlines[j][1:]):
                    point = crossing(p, q, r, s)
                    if point is None:
                        continue
                    if kind == "fill" and any(t.signed(*point) < -1e-6 for t in shapes[j + 1:]):
                        continue
                    found.append(point)
    return found


def corner_stats(kind, shapes, svg, width, height):
    """The share of the true corners that have a traced corner near them,
    and the kinks: anchors that turn by KINK_TURN or more far from any
    true corner or junction.  Anchors on the image border don't count."""
    corners = true_corners(kind, shapes, width, height)
    expected = corners + junctions(kind, shapes)
    turns = [(p, t) for p, t in anchor_turns(subpaths(svg))
             if min(p[0], p[1], width - p[0], height - p[1]) > 1e-6]
    result = {"kinks": sum(1 for (x, y), t in turns if t >= KINK_TURN and
                           all(math.hypot(x - cx, y - cy) > KINK_FAR for cx, cy in expected))}
    if corners:
        kept = sum(1 for cx, cy in corners
                   if any(t >= CORNER_TURN and math.hypot(x - cx, y - cy) <= CORNER_NEAR
                          for (x, y), t in turns))
        result["corners"] = round(100 * kept / len(corners), 1)
    return result


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


# Colour (#443, after the metrics of VectorCraft's tracer)

# A path whose colour is farther than this (Oklab, x100) from every drawn
# colour has an odd colour.
ODD_DELTA = 10.0


def oklab(rgb):
    """An 8-bit sRGB colour in Oklab (Björn Ottosson, 2020), with the
    lightness L_r of Ottosson's Okhsl (2021).  Oklab's own L rises
    steeply from black, like a cube root: #060606 would be 12 from black,
    and every dark grey of a JPEG an odd colour."""
    lin = [v / 255 / 12.92 if v / 255 <= 0.04045 else ((v / 255 + 0.055) / 1.055) ** 2.4
           for v in rgb]
    l = (0.4122214708 * lin[0] + 0.5363325363 * lin[1] + 0.0514459929 * lin[2]) ** (1 / 3)
    m = (0.2119034982 * lin[0] + 0.6806995451 * lin[1] + 0.1073969566 * lin[2]) ** (1 / 3)
    s = (0.0883024619 * lin[0] + 0.2817188376 * lin[1] + 0.6299787005 * lin[2]) ** (1 / 3)
    lightness = 0.2104542553 * l + 0.7936177850 * m - 0.0040720468 * s
    k1, k2 = 0.206, 0.03
    k3 = (1 + k1) / (1 + k2)
    toe = (k3 * lightness - k1 + math.sqrt((k3 * lightness - k1) ** 2 +
                                           4 * k2 * k3 * lightness)) / 2
    return (toe,
            1.9779984951 * l - 2.4285922050 * m + 0.4505937099 * s,
            0.0259040371 * l + 0.7827717662 * m - 0.8086757660 * s)


def delta_e(a, b):
    """The Euclidean distance of two Oklab colours, times 100: black to
    white is 100."""
    return 100 * math.dist(a, b)


def colour_fidelity(width, height, src, out):
    """The mean colour difference between the pixels SRC and OUT, and the
    same weighted by how strong an edge SRC has at each pixel (the
    gradient of its lightness)."""
    cache = {}

    def lab(c):
        if c not in cache:
            cache[c] = oklab(c)
        return cache[c]

    light = [lab(c)[0] for c in src]
    total = edge_total = edge_weight = 0.0
    for y in range(height):
        row, up, down = y * width, max(y - 1, 0) * width, min(y + 1, height - 1) * width
        for x in range(width):
            i = row + x
            d = delta_e(lab(src[i]), lab(out[i]))
            g = math.hypot(light[row + min(x + 1, width - 1)] - light[row + max(x - 1, 0)],
                           light[down + x] - light[up + x])
            total += d
            edge_total += d * g
            edge_weight += g
    return (round(total / (width * height), 3),
            round(edge_total / edge_weight, 3) if edge_weight > 0 else 0.0)


def colour_values(names):
    """ImageMagick colour NAMES as 8-bit RGB."""
    raw = magick(["xc:" + n for n in names] + ["+append", "-depth", "8", "rgb:-"])
    return [tuple(raw[i:i + 3]) for i in range(0, 3 * len(names), 3)]


def drawn_palette(args):
    """The colours that magick ARGS draw with: the canvas, every fill and
    every stroke, as RGB.  None for an image that isn't drawn, or that is
    composited with another image, such as the scanned ones, whose
    colours change."""
    if "-compose" in args:
        return None
    names, fill, stroke, drew = [], "black", "none", False
    for arg in args:
        if arg.startswith("xc:") and arg != "xc:none":
            names.append(arg[3:])
    for option, value in zip(args, args[1:]):
        if option == "-fill":
            fill = value
        elif option == "-stroke":
            stroke = value
        elif option == "-draw":
            drew = True
            names += [c for c in (fill, stroke) if c != "none"]
    if not drew or not names:
        return None
    return colour_values(sorted(set(names)))


def odd_colours(svg, pixels, palette):
    """The paths of SVG whose fill is farther than ODD_DELTA from every
    colour of PALETTE, and the PIXELS of the rendered trace in such a
    colour.  Paths are stacked, so a path's own area would count what
    later paths cover."""
    labs = [oklab(c) for c in palette]
    odd = {}

    def is_odd(rgb):
        if rgb not in odd:
            colour = oklab(rgb)
            odd[rgb] = all(delta_e(colour, t) > ODD_DELTA for t in labs)
        return odd[rgb]

    fills = re.findall(r'<path style="[^"]*\bfill:#([0-9a-fA-F]{6})', svg)
    return (sum(1 for f in fills if is_odd(hex_colour(f))),
            sum(1 for p in pixels if is_odd(p)))


# The gap is measured on a render this many times the image's size.
GAP_SCALE = 4


def uncovered(svg):
    """The area of the image, in pixels, that no path of SVG covers.

    Rendered onto transparency without anti-aliasing at GAP_SCALE times
    the size, each sub-pixel is either covered or not.  With
    anti-aliasing, two paths that meet exactly would each cover part of
    the pixels along their border, and leave a seam that isn't in the
    trace (the conflation of renderers)."""
    if 'shape-rendering="crispEdges"' not in svg:
        svg = svg.replace("<svg ", '<svg shape-rendering="crispEdges" ', 1)
    with tempfile.NamedTemporaryFile("w", suffix=".svg", delete=False) as f:
        f.write(svg)
    try:
        raw = magick(["-density", str(96 * GAP_SCALE), "-background", "none", "svg:" + f.name,
                      "-format", "%w %h\n", "-write", "info:-",
                      "-alpha", "extract", "-depth", "8", "gray:-"])
    finally:
        os.unlink(f.name)
    header, alpha = raw.split(b"\n", 1)
    size = re.search(r'<svg [^>]*\bwidth="([0-9.]+)"[^>]*\bheight="([0-9.]+)"', svg)
    if size and tuple(map(int, header.split())) != tuple(
            round(float(v) * GAP_SCALE) for v in size.groups()):
        return None  # a renderer that ignored -density
    return round(alpha.count(0) / GAP_SCALE ** 2, 2)


def round_trip(png, svg_file, options, shapes=None, centerline=None, palette=None):
    """Compare the rendered SVG with the input image, and with SHAPES, the
    drawn shapes from drawn_shapes(), if given.  CENTERLINE says whether
    it is a centerline trace; None takes it from OPTIONS.  PALETTE is the
    drawn colours from drawn_palette(), if known."""
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
        result.update(corner_stats(shapes[0], shapes[1], svg, w, h))
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
    # Drawn again with anti-aliasing, as a viewer would draw it.
    _, _, smooth = load_pixels("svg:" + svg_file, bg or "white")
    result["delta"], result["edge_delta"] = colour_fidelity(w, h, src, smooth)
    if not bg:
        # With -background-color, the background is left out on purpose.
        result["gap"] = uncovered(svg)
    canvas = hex_colour(bg) if bg else (255, 255, 255)
    if palette is not None:
        result["odd_paths"], result["odd_px"] = odd_colours(svg, out, palette + [canvas])
    return result


def rounded(pair):
    return tuple(None if v is None else round(v, 3) for v in pair)


def hex_colour(spec):
    spec = spec.lstrip("#")
    return tuple(int(spec[i:i + 2], 16) for i in (0, 2, 4))


def trace(autotrace, image, options, svg_file, entry, shapes=None, repeat=1, compare=True,
          detect_centerline=False, palette=None):
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
        dims = re.search(r'<svg [^>]*\bwidth="([0-9.]+)"[^>]*\bheight="([0-9.]+)"', svg)
        if dims:
            entry.update(outline_stats(svg, float(dims.group(1)), float(dims.group(2))))
        entry.update(svg_stats(svg))
        if compare:
            centerline = is_centerline_svg(svg) if detect_centerline else None
            if centerline:
                entry["centerline"] = True
            entry.update(round_trip(image, svg_file, options, shapes, centerline, palette))
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
            palette = drawn_palette(args)
            for options in sets:
                case = case_name(name, options)
                svg_file = os.path.join(workdir, "out.svg")
                entry = {"case": case, "category": category}
                trace(autotrace, png, options, svg_file, entry, shapes, repeat,
                      compare=category != "timing", palette=palette)
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
    keys = ["time", "paths", "nodes", "wrong", "edge_mean", "shape_mean", "shape_max", "corners",
            "kinks", "stray_max", "ink_max"]
    return " ".join("%s=%s" % (k, entry[k]) for k in keys if k in entry)


# The columns of the comparison: (key, heading)
COLUMNS = [
    ("time", "time s"),
    ("size", "bytes"),
    ("paths", "paths"),
    ("nodes", "nodes"),
    ("L_diagonal", "diag. L"),
    ("C", "C"),
    ("lines", "lines %"),
    ("upright", "upright %"),
    ("specks", "specks"),
    ("outside", "outside px"),
    ("wrong", "wrong px"),
    ("wrong_exact", "exact px"),
    ("extra", "extra px"),
    ("missing", "missing px"),
    ("edge_mean", "edge mean"),
    ("edge_max", "edge max"),
    ("delta", "ΔE"),
    ("edge_delta", "edge ΔE"),
    ("odd_paths", "odd paths"),
    ("odd_px", "odd px"),
    ("gap", "gap px"),
    ("shape_mean", "shape mean"),
    ("shape_max", "shape max"),
    ("corners", "corners %"),
    ("kinks", "kinks"),
    ("stray_max", "stray max"),
    ("ink_max", "ink max"),
]


# Columns that are shares of something, not counts.
PERCENT = {"lines", "upright", "corners"}


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
    # A sum of percentages means nothing.
    print("| **total** | %s |" % " | ".join(
        "" if k in PERCENT else cell(round(totals[k][0], 3), round(totals[k][1], 3), k)
        for k, _ in cols))
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
