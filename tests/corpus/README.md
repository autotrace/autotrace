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
ready to paste into a pull request. A run takes about two and a half
minutes.

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
One argument isn't ImageMagick's: `-corpus-jpeg QUALITY` saves the image
as a JPEG of that quality and reads it back, so that the PNG has JPEG's
artefacts.

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
- **centerline**: line art traced with `-centerline` (#112): outlines,
  curves, lines at 24 angles shifted by quarter pixels, junctions of
  degree 3 to 5, dashes, and lines of different widths.
- **degraded**: a star and a three-colour logo, blurred, saved as a JPEG
  at quality 30, and "scanned" onto uneven cream paper with grain.
- **askew**: rectangles turned by 1, 3 and 7 degrees, whose nearly level
  edges should still come out as straight lines.
- **gradient**: linear gradients of two and three colours.
- **size**: the same star, and the same specks and dots, at several
  sizes, because despeckling and some thresholds count pixels.
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

The degraded, askew, gradient and size images, and the centerline ones
from `cl-angles` on, follow the test scenes of
[VectorCraft](https://github.com/storytold/vectorcraft)'s tracer (#442).
The scanned and JPEG copies of the logo, and the scanned star, are traced
only with a preset: with the default options they give thousands of paths
of grain, and take most of a run's time.

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
| lines % | share of the outline's length that is straight: lines, and curves whose control points lie within 0.02 pixels of the chord |
| upright % | share of the outline's length that is straight and exactly level or upright |
| specks | closed subpaths of under 6 square pixels, including those with no area at all (#448) |
| outside px | how far the farthest point of the trace lies outside the image |

Lines and curves along the image border, which every background has, are
left out of the lines and upright shares. Edges shared by two regions
are traced once in each, so they count twice.

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

Colour metrics compare colours in Oklab, where a difference of 100 is
black against white. To match how a viewer sees the trace, ΔE and edge
ΔE use a render with anti-aliasing. Odd pixels are counted on the
`crispEdges` render.

| column | meaning |
|---|---|
| ΔE | mean colour difference between the input and the trace |
| edge ΔE | the same, weighted by how strong an edge the input has at each pixel (the gradient of its lightness) |
| odd paths | paths whose colour is more than 10 from every colour drawn, the canvas included |
| odd px | pixels of the render in such a colour: slivers of a colour the drawing doesn't have |

The odd colours need the drawn colours: the canvas and every fill and
stroke colour. So they are given only for images drawn by `-draw`, and
not for those composited with another image, such as the scanned ones,
whose paper changes every colour.
- **Noise, blur and JPEG:** these images keep their drawn colours as the
  palette, so most odd paths there are traced artefacts: noise, blurred
  ramps, compression blocks.
- **Edge ΔE** is 0 for an image without edges, such as a gradient,
  however far its colours are off. Read ΔE there.

These follow the
metrics of VectorCraft's tracer, with two differences:
- **Lightness:** this uses the lightness L_r of Ottosson's Okhsl instead
  of Oklab's own L. Oklab's L rises from black like a cube root, so
  `#060606` would be 12 from black, and every dark grey of a JPEG would
  count as an odd colour. With L_r it is 4.
- **Odd area:** this counts the odd pixels that show, not the area of
  the odd paths. autotrace stacks its paths, so a path's own area
  includes what later paths cover.

| column | meaning |
|---|---|
| gap px | area that no path covers, in pixels: holes through which the page shows |

The gap is measured on a `crispEdges` render onto transparency, at four
times the image's size. So it counts sub-pixels a quarter pixel wide,
each 1/16 of a pixel's area. A sliver thinner than a quarter pixel is
missed or counted whole, depending on where it falls, and a small
diagonal hole comes out about a quarter too large.
- **Why not anti-aliased:** with anti-aliasing, two paths that meet
  exactly would each cover part of the pixels along their border and
  leave a faint seam. librsvg draws that seam (the conflation of
  renderers), but it isn't in the trace, which shares each border between
  its neighbours (#338). On the plasma images it would add hundreds of
  pixels.
- **Renderer:** the numbers depend on the renderer. ImageMagick's own
  MSVG, used where librsvg isn't, gives somewhat different gaps, as it
  does for `wrong px`.
- **Where it occurs:** on master, the holes come from regions of a few
  pixels in a corner of the image that lose their corners, sometimes
  down to no area (#448). Their neighbours share the cut border, so the
  corner is left uncovered.
- **`-background-color`:** with it, the background is left out on
  purpose, so the gap isn't given.
- **Overlap:** VectorCraft also measures the area covered twice. autotrace
  stacks its paths by design, so that would only measure the stacking,
  and it isn't given.

For `-centerline`, the trace is a one-pixel line rather than an area:

| column | meaning |
|---|---|
| stray max | how far the traced line strays from the ink, in pixels |
| ink max | how far the farthest ink pixel is from the traced line, in pixels |

For line art drawn by `-draw` and traced with `-centerline`, the corpus
also compares the topology of the traced strokes with that of the drawn
ones. Each set of strokes makes a graph:
- **Nodes:** the ends of open strokes, and the points where two strokes
  cross. Points within 0.05 pixels of each other are one node.
- **Degree:** the number of arms that leave a node. A stroke ending there
  gives one arm, and a stroke passing through gives two.
- **Junctions and ends:** a node of degree 3 or more is a junction, and
  one of degree 1 is a free end.

A traced junction matches a drawn one within the stroke's width plus 3
pixels. A traced end matches a drawn one within half the width plus 4
pixels. Each traced node matches once.

| column | meaning |
|---|---|
| junct. ok | drawn junctions with a traced junction of the same degree |
| junct. wrong | drawn junctions with a traced junction of another degree |
| junct. missed | drawn junctions with no traced junction near |
| junct. extra | traced junctions that match no drawn one |
| junct. px | mean distance from a matched junction to the drawn one |
| ends missed | drawn free ends with no traced end near |
| spurs | traced free ends that match no drawn one: stray pieces, and strokes that stop short of the stroke they should meet |
| end px | mean distance from a matched end to the drawn one |

The 0.05-pixel rule is strict on purpose. A stroke that stops a pixel
short of another, as the stem of the T in `cl-junctions` does, looks
joined at one pixel's width, but in a vector editor it isn't: that counts
as a missed junction and a spur. These follow VectorCraft's centre-line
metrics (`crates/trace/tests/quality/centre_lines.rs`).
- **All strokes count:** every traced stroke counts, whatever its colour,
  so the grey fragments that anti-aliasing leaves add spurs. A drawn end
  can also match a fragment's end, not the main stroke's.
- **No width yet:** VectorCraft's width and coverage columns are left
  out. Only `-centerline -preserve-width` writes stroke widths, and the
  corpus doesn't trace with it yet.

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

Two more columns look at the corners. A drawn shape's corners are the
vertices where its outline turns by 20° or more, inside the image and not
covered by a later shape. A traced anchor turns by the angle between the
segments that meet there.

| column | meaning |
|---|---|
| corners % | share of the drawn corners that have a traced anchor turning by 20° or more within 1.5 pixels |
| kinks | traced anchors that turn by 10° or more, farther than 2 pixels from any drawn corner: bumps in straight edges and curves |

Anchors on the image border don't count, and neither do drawn corners
within half a pixel of it, whose trace lies on the border. Where one
filled shape cuts another, or strokes meet or cross, the trace turns at a
point that no primitive has as a vertex: an anchor near such a junction
isn't a kink, but the junction isn't a corner either. A kink on an edge
shared by two regions counts twice.

These two columns, and the lines, upright, specks and outside ones,
follow the metrics of
[VectorCraft](https://github.com/storytold/vectorcraft)'s tracer
(`crates/trace/tests/quality/measure.rs`, #443), with the same thresholds,
and two differences:
- The distances to a corner are measured from the drawn vertex, while a
  filled shape's traced corner lies about 0.7 pixels outside it. So the
  1.5 pixels leave less room here.
- Specks are counted per closed subpath, so a small hole counts too.
  VectorCraft counts whole paths.

Lower is better for all of them except `C`, `lines %` and `upright %`,
which depend on the shape, and `corners %`, where higher is better. A
change usually trades one for another: more nodes for fewer wrong pixels,
say. The table shows both, and the pull request should say which matters.

The edge, stray and ink distances are approximate (3–4 chamfer weights,
within about 8% of the Euclidean distance). Run times vary between runs;
the table only reports a change of more than 20% or 50 ms.

## Not yet

- Ground truth for the images from outside, such as the SVG sources of
  the ANSI gate symbols, for the strokes in `thin` traced as areas, and
  for the text, whose glyph outlines are in the fonts.
