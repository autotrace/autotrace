---
name: choosing-options
description: Choose autotrace options for a particular image by measuring, not guessing. Look at the image, start from the defaults and the matching -preset, then change one option at a time and score each trace with tests/corpus/corpus.py score, which renders the SVG back and counts wrong pixels, nodes and size. Use it whenever someone wants to trace an image with autotrace or libautotrace and asks which options to use, why a trace looks wrong (too many shapes, lost detail, jagged or blobby edges, specks, doubled lines), or how to make the output smaller or more faithful.
---

# Choosing autotrace options for an image

AutoTrace has about 20 fitting options, and their interactions are hard to
predict (issue #50). Options that look sensible can make a trace worse. For
example, despeckling before `-centerline` turns the anti-aliased pixels
around a line into dozens of extra grey strokes. **So measure every
candidate on the user's own image, and recommend only options you measured.**

## What you need

- An `autotrace` binary: a build in this tree (`./autotrace`, which runs the
  right library through libtool) or an installed one.
- Python 3, and ImageMagick 7 (`magick`) with an SVG renderer, ideally
  librsvg. The score renders each SVG back to pixels.
- `tests/corpus/corpus.py` from this repository.

## The score

```sh
tests/corpus/corpus.py score [--svg OUT.svg] AUTOTRACE IMAGE [OPTION...]
```

This traces IMAGE with the options that follow it, and prints one line of
JSON. AUTOTRACE is a path or a program name in `PATH`. IMAGE can be any
format that autotrace reads: everything ImageMagick reads if autotrace was
built with it (`autotrace -list-input-formats`), otherwise PNG, BMP, TGA,
PNM and a few others; convert anything else to PNG first. The fields that
matter:

| field | meaning | better |
|---|---|---|
| `status` | autotrace's exit status; anything but 0 is a failure, with `stderr` | 0 |
| `wrong` | pixels whose colour differs by more than 32 in a channel between the image and the rendered trace | lower |
| `edge_mean`, `edge_max` | distance in pixels between the image's colour edges and the trace's | lower |
| `nodes` | segments in all paths: what a person editing the SVG deals with | lower |
| `paths`, `subpaths` | `<path>` elements, and the separate shapes or strokes inside them; many more than the image visibly has means fragments | close to the visible count |
| `specks` | shapes of under 6 square pixels: noise, or slivers of anti-aliasing | lower, unless the image has dots that small |
| `lines`, `upright` | % of the outline that is straight, and level or upright | depends on the art. Pixel staircases score near 100, so read them with `specks`: a high share with few specks means straight edges kept straight |
| `outside` | pixels the trace reaches beyond the image | 0 |
| `size` | SVG bytes | lower |
| `time` | seconds | lower |

A centerline trace (`-centerline`, or `-preset lineart`) is strokes, not
filled areas, and the JSON says `"centerline": true`. It has no `wrong`.
Instead:
- `stray_max` is how far a stroke strays from the ink;
- `ink_max` is how far any ink is from the nearest stroke.

Never compare `wrong` between an outline trace and a centerline one.

All strokes of one colour form one path, so count strokes with `subpaths`,
not `paths`. `ink_*` counts every dark pixel as ink, noise specks
included. On a noisy scan it therefore rewards tracing the specks, and
gets worse exactly when despeckling works. There, judge by `subpaths`,
`nodes` and `stray_max`, and look at the result.

Fidelity (`wrong`, `edge_*`) and simplicity (`nodes`, `size`) pull against
each other. Ask the user which matters more if it isn't clear: a logo for
a cutting plotter wants few nodes, while a map wants every edge in place.
Without an answer, prefer a large cut in nodes for a small rise in
`wrong`, of a few percent.

## Procedure

1. **Look at the image.** Run
   `magick identify -format '%wx%h, %k colours, alpha %A\n' IMAGE`, and
   view the image if you can. "alpha Undefined" means no alpha channel;
   "Blend" or "True" means one. Classify the image:
   - **flat colours** (logo, icon, text, diagram), with or without
     anti-aliased edges: hundreds of colours on an image that looks like
     a handful means anti-aliasing;
   - **line art** (drawn lines, handwriting, a scanned sketch) that should
     become strokes rather than outlines;
   - **scan**: flat colours or lines with specks of noise;
   - **photo or gradient**: thousands of colours with no flat regions;
   - **transparent background** (an alpha channel), or a background the
     user wants left out.
2. **Score the baseline:** the defaults, then the preset that matches
   (`autotrace -list-presets`):

   | preset | for | options |
   |---|---|---|
   | `logo` | flat colours, anti-aliased or not | `-snap-antialiasing -error-threshold 1 -keep-knees` |
   | `lineart` | drawn lines | `-centerline -snap-antialiasing -despeckle-level 2 -despeckle-tightness 0 -noise-removal 1` |
   | `scan` | scanned drawings and text | `logo`'s, and `-despeckle-level 4 -despeckle-tightness 0 -noise-removal 1` |
   | `photo` | photographs, gradients | `-color-count 32 -despeckle-level 2` |

   If two classes fit, score both presets. A preset applies before every
   other option, wherever it is on the command line, so the others change
   it. A flag that a preset turns on, such as `-keep-knees` or
   `-centerline`, can't be turned off again. To try a preset without one,
   write out its other options instead of `-preset`.
3. **Change one option at a time** from the best result so far, using the
   table below. Keep a change only if the score improves on the measure
   the user cares about. Stop after two rounds without an improvement, or
   after about 20 traces.
4. **Look at the result.** Render the best SVG
   (`magick -background white OUT.svg out.png`) and view it next to the
   image. The score can't see some failures: strokes merged that should be
   separate, a corner rounded off, a thin gap closed.
5. **Report** the final command line, its score next to the defaults' and
   the preset's, and what you tried that didn't help. Then the user can
   change the trade-off without starting again.

**A large image** (several megapixels, or a trace that takes more than a
few seconds) can be searched on a crop:
`magick IMAGE -crop 512x512+X+Y +repage crop.png`, at a place with typical
content. Don't downscale: despeckling and the thresholds count pixels, so
their effect changes with the scale. Confirm the final options on the whole
image.

## What each option does, and what it costs

| symptom | try | cost |
|---|---|---|
| an anti-aliased edge becomes many thin outlines or slivers | `-snap-antialiasing` (does nothing unless the image has 2–24 flat colours) | a few wrong pixels along the edges |
| too many colour regions in a photo or gradient | `-color-count N` (works on grey and colour images), N from 4 to 64 | banding; fewer colours, more `wrong` |
| specks of noise become tiny shapes | `-despeckle-level 2`–`4` (merges regions under 2, 4 or 8 pixels) | specks that are real detail disappear |
| black specks on white survive despeckling | add `-despeckle-tightness 0 -noise-removal 1`; by default only similar colours are merged | merges across any colour difference |
| curves are lumpy or off the edge | `-error-threshold 1` (default 2); lower fits closer | more nodes |
| too many nodes | `-error-threshold 3`–`4` | curves drift from the edges |
| circles and text are faceted, staircases smoothed wrongly | `-keep-knees` | more nodes: about 10% on a logo, and on the corpus twice as many cubic segments; sharp corners can round off |
| sharp corners are rounded | raise `-corner-threshold` (default 100°: a smaller angle is a corner); for corners right next to each other, also raise `-corner-always-threshold` (default 60°) | spurious corners on curves |
| curves get corners | lower `-corner-threshold` | real corners round off |
| nearly straight curves | `-line-threshold` higher (default 1) makes more of them straight lines | gentle curves become lines |
| lines should be single strokes, not outlined shapes | `-centerline` (assumes a white background unless `-background-color` says otherwise) | stroke width is lost; add `-preserve-width` to keep it |
| a background colour should not be traced, or the output should be transparent | `-background-color RRGGBB` | regions of exactly that colour vanish, including inside shapes |
| shapes are jaggy from noise along edges | raise `-filter-iterations` (default 4) | small features blur |

Run `autotrace -help` or read `autotrace.1` for the full list. Only some
options are ever worth searching: the ones above.

**Known interactions:**
- Despeckling an anti-aliased image without `-snap-antialiasing` merges
  the edge shades into regions of their own. With `-centerline`, each
  becomes a stroke.
- The passes run in this order: `-snap-antialiasing`, despeckling,
  `-color-count`, then tracing. So despeckling sees the full colours: on a
  photo it merges specks into similar neighbours before the colours are
  reduced.
- `-background-color` also affects `-color-count`: the background counts
  as one more palette colour.
- With `-centerline`, the defaults suit thin lines. Thick filled shapes
  thin to a skeleton that may have spurs.

## Background

Parts of the fitter come from the `limn` program in GNU fontutils
(README.md). Its comments cite:
- P. J. Schneider, *Phoenix: An Interactive Curve Design System Based on
  the Automatic Fitting of Hand-Sketched Curves*, master's thesis,
  University of Washington, 1988. It covers the least-squares fit that
  `-error-threshold` controls, with subdivision of the curve where the fit
  fails, and the corner finding that the corner options tune.
- M. Plass and M. Stone, "Curve-fitting with piecewise parametric cubics",
  SIGGRAPH 1983: chord-length parameterisation.
- J. M. Cychosz, "Efficient Binary Image Thinning using Neighborhood
  Maps", *Graphics Gems IV*, 1994: the thinning behind `-centerline`.

`tests/corpus/README.md` explains the metrics in more detail. The pull
requests that added the presets (#426) and `-keep-knees` (#425) have the
corpus measurements behind the advice above.
