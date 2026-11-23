<!--
SPDX-FileCopyrightText: © 2026 AutoTrace contributors

SPDX-License-Identifier: CC0-1.0
-->

# Trace-quality corpus

The tests in `tests/` tell us that the output changed. This corpus tells
us whether it got better or worse (issue #341). It traces a set of images
with two `autotrace` builds and prints a table of the differences, for a
pull request that changes traced output to show its effect in numbers.

It isn't part of `make check`, and nothing here fails: it only measures.

## Running it

You need Python 3 and ImageMagick 7 (`magick`) built with librsvg, which
renders the SVG back to pixels. Build the old and the new version, for
example master and your branch in two worktrees, then:

```sh
tests/corpus/corpus.py run ../master/autotrace ./autotrace /tmp/corpus
```

It draws the images into `/tmp/corpus/images`, writes `old.json` and
`new.json` there, and prints a Markdown table of the cases that changed,
ready to paste into a pull request. A run takes under a minute.

- `--repeat 3` reports the best of three run times. Use it when time
  matters, on an otherwise idle machine.
- `--all` lists the unchanged cases too.
- The steps can also run one at a time: `corpus.py generate DIR`,
  `corpus.py measure AUTOTRACE DIR OUT.json` and
  `corpus.py compare OLD.json NEW.json`.

## The images

[`images.txt`](images.txt) lists the images, one per line, with the
`magick` command that draws each of them, or for the images in `images/`,
the file. Each line also gives the option sets to trace the image with.

Prefer generated images: they need no image files and no licences beyond
the scripts' own. An image from elsewhere goes into `images/` as a PNG on
white, with its author, licence and source recorded in `REUSE.toml` (see
the existing entries).

The categories:

- **geometry**: squares (#50), rotated squares, circles, ellipses, triangles,
  stars and a Bézier blob. Most come in pairs, `NAME` drawn without
  anti-aliasing and `NAME-aa` with it (#348).
- **topology**: holes, nested regions, regions touching only at a corner,
  a checkerboard, islands of the same colour.
- **thin**: lines one and two pixels wide.
- **colour**: the same plasma image with 2, 8 and 64 colours, a smooth one
  reduced by `-color-count`, and regions 1–10 levels apart.
- **noise**: impulse noise, traced with and without despeckling (#376).
- **alpha**: a shape on a transparent background.
- **centerline**: line art traced with `-centerline` (#112).
- **outside**: CC0 line art, diagrams and symbols from Wikimedia Commons,
  among them a JPEG with its compression artefacts (`hiemalora`).
- **timing**: a 2000×2000 image for run time and threads (#155). It gets
  output metrics only.

To add an image, add a line. Prefer a drawing that shows one problem
clearly over a large mixed one, and name the issue it comes from in a
comment.

## The metrics

Output metrics, from the SVG alone:

| column | meaning |
|---|---|
| time s | wall time of the `autotrace` run, in seconds |
| bytes | size of the SVG |
| paths | `<path>` elements |
| nodes | `L` and `C` commands |
| diag. L | `L` commands that are neither horizontal nor vertical |
| C | `C` commands (Bézier curves) |

Round-trip metrics render the SVG at the input's size, with anti-aliasing
off (`shape-rendering="crispEdges"`), and compare it with the input. Both
are flattened onto white, or onto the `-background-color` if one is given.
Two colours count as the same when every channel is within 32 levels, so
that the shades of anti-aliased input don't count as errors.

| column | meaning |
|---|---|
| wrong px | pixels whose colour differs |
| exact px | pixels whose colour differs at all, for colours closer than 32 levels |
| extra px | with `-background-color`: background pixels the trace covers |
| missing px | with `-background-color`: foreground pixels the trace leaves out |
| edge mean | mean distance between the input's and the trace's colour edges, in pixels (Chamfer distance) |
| edge max | the largest such distance (Hausdorff distance) |

For `-centerline`, the trace is a one-pixel line rather than an area:

| column | meaning |
|---|---|
| stray max | how far the traced line strays from the ink, in pixels |
| ink max | how far the farthest ink pixel is from the traced line, in pixels |

Lower is better for all of them except `C`, which depends on the shape. A
change usually trades one for another: more nodes for fewer wrong pixels,
say. The table shows both, and the pull request should say which matters.

Distances are approximate (3–4 chamfer weights, within about 8% of the
Euclidean distance). Run times vary between runs; the table only reports a
change of more than 20% or 50 ms.

## Not yet

- Distances to the drawn shape itself (ground truth) rather than to its
  pixels.
- The public-domain images suggested in #341. REUSE has no SPDX
  identifier for public domain, so they need a `LicenseRef-` licence text
  first.
- Text, which depends on the fonts installed.
