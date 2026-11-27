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
ready to paste into a pull request. A run takes about a minute and a half.

- `--repeat 3` reports the best of three run times. Use it when time
  matters, on an otherwise idle machine.
- `--all` lists the unchanged cases too.
- The steps can also run one at a time: `corpus.py generate DIR`,
  `corpus.py measure AUTOTRACE DIR OUT.json` and
  `corpus.py compare OLD.json NEW.json`.

### Scoring one trace

`corpus.py score [--svg OUT.svg] AUTOTRACE IMAGE [OPTION...]` traces a
single image with the options after it, and prints the same metrics as one
line of JSON. The image can be any file ImageMagick reads, not only a corpus
image. The [choosing-options](../../.agents/skills/choosing-options/SKILL.md)
skill uses it to compare option sets on a user's own image. It exits with
autotrace's status when the trace fails. `--svg` keeps the trace.

## The images

[`images.txt`](images.txt) lists the images, one per line, with the
`magick` command that draws each of them, or for the images in `images/`,
the file. Each line also gives the option sets to trace the image with.

Prefer generated images: they need no image files and no licences beyond
the scripts' own. An image from elsewhere goes into `images/` as a PNG on
white, with its author, licence and source recorded in `REUSE.toml` (see
the existing entries). A public-domain image takes the licence
`LicenseRef-PublicDomain`, and its entry says why it is in the public
domain.

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
- **outside**: line art, diagrams and symbols from Wikimedia Commons,
  either CC0 or in the public domain. Among them are JPEGs with their
  compression artefacts (`hiemalora`, `syndicalism`), ANSI logic gate
  symbols, and a scanned book illustration (`sunbonnet`).
- **text**: Liberation Sans and Serif at 12, 32 and 40 pixels, with and
  without anti-aliasing, and in colour. How text renders depends on the
  fonts and on FreeType, so two machines would draw different images:
  these are stored in `images/` too, and [`text.sh`](text.sh) records how
  they were made. The fonts are under the SIL Open Font License, which
  doesn't cover documents made with them, so the images are CC0.
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

Ground-truth metrics compare the trace with the shape that was drawn,
not with its pixels. That separates the fitting error from the
rasterisation error, which matters for changes aimed at the shape itself,
such as recognising circles and lines (#349) or refining curves against
the image (#354). They are given for images drawn by `-draw` primitives
alone (`rectangle`, `polygon`, `circle`, `ellipse`, `line`, and a `path`
of `M`, `L`, `C` and `Z`): filled shapes traced as areas, and stroked
ones traced with `-centerline`, where the shape is the stroke's path. The
noise images count too, since the noise is added after drawing: there,
the metrics show how much of the noise the trace keeps.

| column | meaning |
|---|---|
| shape mean | mean distance between the traced curves and the drawn shape, in pixels, averaged over both directions |
| shape max | the largest such distance, either way |

These distances are exact: points 0.1 pixels apart on the traced curves
are measured against the shape, and points on the shape against the
trace, with Bézier curves on both sides flattened to within 0.01 pixels.
ImageMagick fills a shape with a margin of half a pixel, so the
edges of a filled shape are taken to lie half a pixel outside it, rounded
at the corners. Pixels can't tell a rounded corner from a square one, so
an exact trace of a right angle still scores up to 0.21 at the corner.
Edges along the image border, and those covered by a later shape, are left
out. `square-aa` is drawn on half pixels, where ImageMagick's
anti-aliasing doesn't follow that model: its edges are about a quarter of
a pixel off.

Lower is better for all of them except `C`, which depends on the shape. A
change usually trades one for another: more nodes for fewer wrong pixels,
say. The table shows both, and the pull request should say which matters.

The edge, stray and ink distances are approximate (3–4 chamfer weights,
within about 8% of the Euclidean distance). Run times vary between runs;
the table only reports a change of more than 20% or 50 ms.

## Not yet

- Ground truth for the images from outside, such as the SVG sources of
  the ANSI gate symbols, for the strokes in `thin` traced as areas, and
  for the text, whose glyph outlines are in the fonts.
