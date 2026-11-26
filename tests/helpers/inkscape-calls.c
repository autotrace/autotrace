/* SPDX-FileCopyrightText: 2026 AutoTrace contributors
 *
 * SPDX-License-Identifier: CC0-1.0
 */

/* Use libautotrace the way Inkscape's Trace Bitmap dialog does
   (src/trace/autotrace/inkscape-autotrace.cpp in Inkscape), so that a
   change that breaks Inkscape's build or behaviour fails "make check"
   (#342).  It relies on what Inkscape relies on:
   - at_fitting_opts_new() before autotrace_init(), and a white
     background_color from at_color_new();
   - an at_bitmap filled in on the stack (height, width, bitmap, np = 3)
     rather than from at_bitmap_new();
   - at_splines_new_full() with progress and cancel callbacks and no
     message callback;
   - walking the result with SPLINE_LIST_ARRAY_LENGTH/_ELT,
     SPLINE_LIST_LENGTH/_ELT, SPLINE_DEGREE, START_POINT, CONTROL1,
     CONTROL2 and END_POINT, reading splines->height, splines->centerline,
     list.open and list.color, and comparing colours with at_color_equal();
   - the options Inkscape sets: color_count, centerline, preserve_width,
     filter_iterations and error_threshold.

   Each case traces a bitmap made in memory and checks the result as
   Inkscape would use it.  Usage: inkscape-calls   Exits 0 if every case
   passes.  */

#include <stdio.h>
#include <string.h>

#include "autotrace.h"
#include "output.h"
#include "spline.h"

typedef struct {
  gfloat last;
  int backwards;
} progress_state;

static void progress(gfloat frac, gpointer data)
{
  progress_state *state = data;

  if (frac < state->last)
    state->backwards++;
  state->last = frac;
}

static gboolean keep_going(gpointer data)
{
  return FALSE;
}

static gboolean cancel_now(gpointer data)
{
  return TRUE;
}

/* Walk SPLINES as Inkscape turns them into paths: return the number of
   paths, counting a new one whenever the colour changes, or -1 if a
   segment isn't a line or a cubic.  Count the segments in SEGMENTS, and in
   STROKES the spline lists Inkscape draws as strokes rather than fills.  */
static int walk(at_splines_type *splines, int *segments, int *strokes)
{
  at_spline_list_type list;
  at_color last_color = {0, 0, 0};
  int paths = 0, list_i;

  *segments = *strokes = 0;
  if (splines->height <= 0)
    return -1;
  for (list_i = 0; list_i < (int)SPLINE_LIST_ARRAY_LENGTH(*splines); list_i++) {
    at_spline_type first;
    int spline_i;

    list = SPLINE_LIST_ARRAY_ELT(*splines, list_i);
    if (list_i == 0 || !at_color_equal(&list.color, &last_color))
      paths++;
    /* Inkscape strokes these and fills the others.  */
    if (splines->centerline || list.open)
      (*strokes)++;
    first = SPLINE_LIST_ELT(list, 0);
    if (START_POINT(first).x < 0)
      return -1;
    for (spline_i = 0; spline_i < (int)SPLINE_LIST_LENGTH(list); spline_i++) {
      at_spline_type spline = SPLINE_LIST_ELT(list, spline_i);

      if (SPLINE_DEGREE(spline) != AT_LINEARTYPE && SPLINE_DEGREE(spline) != AT_CUBICTYPE)
        return -1;
      /* Inkscape reads these for its lineTo() and curveTo().  */
      (void)CONTROL1(spline);
      (void)CONTROL2(spline);
      (void)END_POINT(spline);
      (*segments)++;
      last_color = list.color;
    }
  }
  return paths;
}

static int failures;

/* Trace an RGB bitmap of SIZE x SIZE pixels, white with a black square in
   the middle unless BLANK, with OPTS.  EXPECT_PATHS is the least number
   of paths wanted (0: none).  */
static void run(const char *name, at_fitting_opts_type *opts, int size, gboolean blank,
                int expect_paths, gboolean cancel)
{
  unsigned char *pixels = g_malloc((gsize)size * size * 3);
  at_bitmap bitmap;
  at_splines_type *splines;
  progress_state state = {-1, 0};
  int paths = 0, segments = 0, strokes = 0, y;

  memset(pixels, 255, (gsize)size * size * 3);
  if (!blank)
    for (y = size / 4; y < 3 * size / 4; y++)
      memset(pixels + ((gsize)y * size + size / 4) * 3, 0, (gsize)(size / 2) * 3);

  /* As Inkscape: on the stack, not from at_bitmap_new().  */
  bitmap.height = size;
  bitmap.width = size;
  bitmap.bitmap = pixels;
  bitmap.np = 3;

  splines = at_splines_new_full(&bitmap, opts, NULL, NULL, progress, &state,
                                cancel ? cancel_now : keep_going, NULL);
  if (cancel) {
    if (splines != NULL) {
      printf("%s: splines returned after a cancel  <- wrong\n", name);
      failures++;
      at_splines_free(splines);
    } else
      printf("%s: NULL, as documented\n", name);
    g_free(pixels);
    return;
  }
  if (splines == NULL) {
    printf("%s: NULL  <- wrong\n", name);
    failures++;
    g_free(pixels);
    return;
  }
  paths = walk(splines, &segments, &strokes);
  printf("%s: %d paths, %d segments, %d strokes, progress ended at %.3f, %d steps backwards\n",
         name, paths, segments, strokes, state.last, state.backwards);
  /* A centerline trace is all strokes, an outline trace none.  */
  if ((opts->centerline && strokes != (int)SPLINE_LIST_ARRAY_LENGTH(*splines)) ||
      (!opts->centerline && strokes != 0)) {
    printf("%s: stroke count wrong\n", name);
    failures++;
  }
  if (paths < 0 || paths < expect_paths || (expect_paths == 0 && segments != 0) ||
      state.backwards != 0 || state.last != 1.0f) {
    printf("%s:  <- wrong\n", name);
    failures++;
  }
  at_splines_free(splines);
  g_free(pixels);
}

int main(void)
{
  at_fitting_opts_type *opts;

  /* Inkscape's order: the options first, then autotrace_init().  */
  opts = at_fitting_opts_new();
  opts->background_color = at_color_new(255, 255, 255);
  autotrace_init();

  run("outline", opts, 40, FALSE, 1, FALSE);

  opts->filter_iterations = 2;
  opts->error_threshold = 1.5f;
  run("filter_iterations, error_threshold", opts, 40, FALSE, 1, FALSE);

  opts->color_count = 8;
  run("color_count", opts, 40, FALSE, 1, FALSE);
  /* Only the background colour: an empty trace, not a crash (#404).  */
  run("color_count, blank image", opts, 40, TRUE, 0, FALSE);
  opts->color_count = 0;

  opts->centerline = TRUE;
  run("centerline", opts, 40, FALSE, 1, FALSE);
  opts->preserve_width = TRUE;
  run("centerline, preserve_width", opts, 40, FALSE, 1, FALSE);
  opts->centerline = FALSE;
  opts->preserve_width = FALSE;

  run("cancel", opts, 40, FALSE, 0, TRUE);

  at_fitting_opts_free(opts);
  return failures ? 1 : 0;
}
