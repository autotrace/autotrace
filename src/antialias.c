/*
 * SPDX-FileCopyrightText: © 2026 AutoTrace contributors
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

/* antialias.c: snap anti-aliased edge pixels to the flat colours (issue
   #348).

   Anti-aliased artwork has a few flat colours and, along every edge, a
   band of pixels blended from the two colours on either side.  Traced as
   they are, the blends become hundreds of thin regions.  This pass gives
   each blended pixel the nearer of the two colours, which is what the
   pixel would be without anti-aliasing at a 50% coverage threshold:

   1. The palette: the colours of flat pixels, those whose eight
      neighbours all lie within FLAT_TOLERANCE in every channel, or if
      that finds fewer than two colours (strokes too thin for a 3x3 block
      of ink), at least LOOSE_NEIGHBOURS of them.  Colours
      within MERGE_TOLERANCE of a more frequent one join it; a colour
      needs MIN_PIXELS pixels to count.
   2. An image with fewer than two such colours, or more than
      MAX_PALETTE (a photograph, a gradient), is left alone.
   3. Every other pixel that lies within SEGMENT_TOLERANCE of the segment
      between two palette colours gets the nearer end; one near a single
      palette colour gets that colour; the rest are left alone.  A flat
      pixel of a rare colour, with under a tenth of its cluster, gets
      the cluster's colour.

   Distances are Euclidean in RGB.  Grey pixels count as three equal
   channels, so that the tolerances mean the same for both.  */

#include "antialias.h"
#include <glib.h>
#include <math.h>
#include <stdlib.h>

#define FLAT_TOLERANCE 6
#define LOOSE_NEIGHBOURS 5
#define MERGE_TOLERANCE 12
#define MIN_PIXELS 16
#define MAX_PALETTE 24
#define SEGMENT_TOLERANCE 20.0

typedef struct {
  guint32 colour; /* packed channels, 8 bits each */
  gulong count;
} colour_count;

static guint32 pack(const unsigned char *p, unsigned np)
{
  return np == 1 ? p[0] : ((guint32)p[0] << 16) | ((guint32)p[1] << 8) | p[2];
}

static void unpack(guint32 colour, unsigned np, int *c)
{
  if (np == 1)
    c[0] = (int)colour;
  else {
    c[0] = (int)(colour >> 16) & 0xff;
    c[1] = (int)(colour >> 8) & 0xff;
    c[2] = (int)colour & 0xff;
  }
}

static int max_difference(const unsigned char *a, const unsigned char *b, unsigned np)
{
  int diff = 0;
  unsigned i;
  for (i = 0; i < np; i++)
    diff = MAX(diff, abs((int)a[i] - (int)b[i]));
  return diff;
}

/* Is the pixel at (X, Y) flat: do at least NEEDED of its eight neighbours,
   or all of them on the image border, lie within FLAT_TOLERANCE of it?
   Only neighbours inside the image count: repeating the border pixels
   instead would make every pixel of a border row look like its row.  */
static gboolean is_flat(const at_bitmap *bitmap, int x, int y, int needed)
{
  int width = bitmap->width, height = bitmap->height, dx, dy, misses = 0, allowed;
  int columns = 1 + (x > 0) + (x < width - 1), rows = 1 + (y > 0) + (y < height - 1);
  int total = columns * rows - 1;
  unsigned np = bitmap->np;
  const unsigned char *p = bitmap->bitmap + ((size_t)y * width + x) * np;

  if (total == 0)
    return FALSE;
  allowed = total - MIN(needed, total);
  for (dy = -1; dy <= 1; dy++)
    for (dx = -1; dx <= 1; dx++) {
      int nx = x + dx, ny = y + dy;
      if ((!dx && !dy) || nx < 0 || ny < 0 || nx >= width || ny >= height)
        continue;
      if (max_difference(p, bitmap->bitmap + ((size_t)ny * width + nx) * np, np) > FLAT_TOLERANCE &&
          ++misses > allowed)
        return FALSE;
    }
  return TRUE;
}

static gint by_count(gconstpointer a, gconstpointer b)
{
  const colour_count *x = a, *y = b;
  if (x->count != y->count)
    return x->count > y->count ? -1 : 1;
  return x->colour < y->colour ? -1 : x->colour > y->colour;
}

/* The client's callbacks, and the part of the pass's progress, FROM to
   TO, that the current scan of the image covers.  */
typedef struct {
  at_progress_func notify_progress;
  gpointer progress_data;
  at_testcancel_func test_cancel;
  gpointer testcancel_data;
  gfloat from, to;
} row_hooks;

/* At the start of each row, as find_outline_pixels() does: report the
   progress of pixel K of N, then ask whether the client has cancelled.  */
static gboolean cancel_requested(const row_hooks *hooks)
{
  return hooks->test_cancel && hooks->test_cancel(hooks->testcancel_data);
}

static gboolean row_cancelled(const row_hooks *hooks, size_t k, size_t n, unsigned width)
{
  if (k % width != 0)
    return FALSE;
  if (hooks->notify_progress)
    hooks->notify_progress(hooks->from + (hooks->to - hooks->from) * (gfloat)((double)k / n),
                           hooks->progress_data);
  return cancel_requested(hooks);
}

/* How many colours the palette search merges between two checks of
   test_cancel: a few milliseconds' work at most.  */
#define COLOURS_PER_CANCEL_CHECK 1024

/* The palette of flat colours, or an empty array if the image doesn't
   look like flat artwork or the client cancelled (*CANCELLED is then
   TRUE).  FLAT gets the flatness of every pixel, and MINOR maps each rare
   flat colour (key colour + 1) to the palette colour of its cluster (value
   colour + 1).  */
static GArray *find_palette(const at_bitmap *bitmap, gboolean *flat, int needed, GHashTable *minor,
                            const row_hooks *hooks, gboolean *cancelled)
{
  unsigned np = bitmap->np, i, j;
  size_t n = (size_t)bitmap->width * bitmap->height, k;
  GHashTable *counts = g_hash_table_new(g_direct_hash, g_direct_equal);
  GArray *colours = g_array_new(FALSE, FALSE, sizeof(colour_count));
  GArray *clusters = g_array_new(FALSE, FALSE, sizeof(colour_count));
  GArray *palette = g_array_new(FALSE, FALSE, sizeof(guint32));
  GArray *member = g_array_new(FALSE, FALSE, sizeof(unsigned));
  /* The pixels of each cluster's own colour, its most frequent one.  */
  GArray *own = g_array_new(FALSE, FALSE, sizeof(gulong));
  GHashTableIter iter;
  gpointer key, value;
  unsigned big = 0;
  guint32 run_colour = 0;
  gulong run = 0;

  /* Count the flat colours a run at a time: flat artwork has long runs of
     one colour.  Keys can't be 0 in a direct hash table: offset them.  */
  for (k = 0; k <= n; k++) {
    guint32 colour = 0;
    gboolean counted = FALSE;
    if (k < n) {
      if (row_cancelled(hooks, k, n, bitmap->width)) {
        *cancelled = TRUE;
        break;
      }
      flat[k] = is_flat(bitmap, (int)(k % bitmap->width), (int)(k / bitmap->width), needed);
      if (flat[k]) {
        colour = pack(bitmap->bitmap + k * np, np) + 1;
        counted = TRUE;
      }
    }
    if (run && (!counted || colour != run_colour)) {
      gpointer key_colour = GUINT_TO_POINTER(run_colour);
      gulong c = GPOINTER_TO_SIZE(g_hash_table_lookup(counts, key_colour));
      g_hash_table_insert(counts, key_colour, GSIZE_TO_POINTER(c + run));
      run = 0;
    }
    if (counted) {
      run_colour = colour;
      run++;
    }
  }
  g_hash_table_iter_init(&iter, counts);
  while (g_hash_table_iter_next(&iter, &key, &value)) {
    colour_count c = {GPOINTER_TO_UINT(key) - 1, GPOINTER_TO_SIZE(value)};
    g_array_append_val(colours, c);
  }
  g_hash_table_destroy(counts);
  if (!*cancelled && cancel_requested(hooks))
    *cancelled = TRUE;
  if (!*cancelled)
    g_array_sort(colours, by_count);

  /* Merge each colour into the first more frequent one close to it.  An
     image can have millions of flat colours, so keep asking test_cancel.  */
  for (i = 0; i < colours->len && !*cancelled; i++) {
    colour_count c = g_array_index(colours, colour_count, i);
    int a[3], b[3];
    if (i % COLOURS_PER_CANCEL_CHECK == COLOURS_PER_CANCEL_CHECK - 1 && cancel_requested(hooks)) {
      *cancelled = TRUE;
      break;
    }
    unpack(c.colour, np, a);
    for (j = 0; j < clusters->len; j++) {
      colour_count *cluster = &g_array_index(clusters, colour_count, j);
      unsigned ch;
      int diff = 0;
      unpack(cluster->colour, np, b);
      for (ch = 0; ch < np; ch++)
        diff = MAX(diff, abs(a[ch] - b[ch]));
      if (diff <= MERGE_TOLERANCE) {
        if (cluster->count < MIN_PIXELS && cluster->count + c.count >= MIN_PIXELS)
          big++;
        cluster->count += c.count;
        break;
      }
    }
    if (j == clusters->len) {
      g_array_append_val(clusters, c);
      g_array_append_val(own, c.count);
      if (c.count >= MIN_PIXELS)
        big++;
    }
    g_array_append_val(member, j);
    /* Counts only grow: too many colours now means too many at the end.  */
    if (big > MAX_PALETTE)
      break;
  }
  if (!*cancelled && big >= 2 && big <= MAX_PALETTE) {
    for (j = 0; j < clusters->len; j++)
      if (g_array_index(clusters, colour_count, j).count >= MIN_PIXELS)
        g_array_append_val(palette, g_array_index(clusters, colour_count, j).colour);
    /* A flat colour with under a tenth of its cluster's pixels, and under
       half as many as the cluster's own colour, is a stray variant of it,
       such as 251 beside 255, and would trace as regions of its own.  A
       gradient's shades are each about as frequent as the next, so they
       aren't banded however many of them a cluster holds.  */
    for (i = 0; i < member->len; i++) {
      colour_count c = g_array_index(colours, colour_count, i);
      j = g_array_index(member, unsigned, i);
      colour_count cluster = g_array_index(clusters, colour_count, j);
      if (cluster.count >= MIN_PIXELS && c.colour != cluster.colour &&
          10 * (guint64)c.count < cluster.count &&
          2 * (guint64)c.count < g_array_index(own, gulong, j))
        g_hash_table_insert(minor, GUINT_TO_POINTER(c.colour + 1),
                            GUINT_TO_POINTER(cluster.colour + 1));
    }
  }
  g_array_free(colours, TRUE);
  g_array_free(clusters, TRUE);
  g_array_free(member, TRUE);
  g_array_free(own, TRUE);
  return palette;
}

/* The squared distance from colour P to the segment from colour A to
   colour B, with T set to where along it the nearest point lies.  */
static double to_segment(const int *p, const int *a, const int *b, unsigned np, double *t)
{
  double ab = 0, ap = 0, d = 0;
  unsigned ch;
  for (ch = 0; ch < np; ch++) {
    double e = b[ch] - a[ch];
    ab += e * e;
    ap += e * (p[ch] - a[ch]);
  }
  *t = CLAMP(ap / ab, 0.0, 1.0);
  for (ch = 0; ch < np; ch++) {
    double e = p[ch] - (a[ch] + *t * (b[ch] - a[ch]));
    d += e * e;
  }
  return d;
}

gboolean snap_antialiased(at_bitmap *bitmap, at_progress_func notify_progress,
                          gpointer progress_data, at_testcancel_func test_cancel,
                          gpointer testcancel_data, unsigned long *changed_pixels)
{
  /* The strict scan, the loose one if needed, then the snapping: a third
     of the progress each.  */
  row_hooks hooks = {notify_progress, progress_data, test_cancel, testcancel_data, 0, 1 / 3.0f};
  unsigned np = bitmap->np, i, j, ch;
  size_t n = (size_t)bitmap->width * bitmap->height, k;
  /* Squared distances in grey count three channels.  */
  double scale = np == 1 ? 3.0 : 1.0;
  unsigned long changed = 0;
  gboolean *flat, *on_segment, cancelled = FALSE;
  GArray *palette;
  int (*colours)[3], variant[3];
  GHashTable *minor;

  if (changed_pixels)
    *changed_pixels = 0;
  if ((np != 1 && np != 3) || n == 0)
    return TRUE;
  flat = g_new(gboolean, n);
  minor = g_hash_table_new(g_direct_hash, g_direct_equal);
  palette = find_palette(bitmap, flat, 8, minor, &hooks, &cancelled);
  hooks.from = 1 / 3.0f;
  hooks.to = 2 / 3.0f;
  if (palette->len < 2 && !cancelled) {
    /* Strokes too thin for a 3x3 block of ink, such as small text, leave
       only the background flat: accept pixels most of whose neighbours
       match them.  Not always, for that also finds blends along edges.  */
    g_array_free(palette, TRUE);
    g_hash_table_remove_all(minor);
    palette = find_palette(bitmap, flat, LOOSE_NEIGHBOURS, minor, &hooks, &cancelled);
  }
  hooks.from = 2 / 3.0f;
  hooks.to = 1;
  colours = g_malloc_n(MAX(palette->len, 1), sizeof *colours);
  on_segment = g_new0(gboolean, MAX(palette->len, 1));
  for (i = 0; i < palette->len; i++)
    unpack(g_array_index(palette, guint32, i), np, colours[i]);
  /* Does colour I lie exactly on a segment that the search below tries
     before colour I itself?  */
  for (i = 0; i < palette->len; i++)
    for (j = 0; j < i && !on_segment[i]; j++) {
      unsigned m;
      for (m = j + 1; m < palette->len && !on_segment[i]; m++) {
        double t;
        on_segment[i] = m != i && to_segment(colours[i], colours[j], colours[m], np, &t) == 0;
      }
    }

  for (k = 0; k < n && palette->len > 0; k++) {
    unsigned char *p = bitmap->bitmap + k * np;
    double best = INFINITY;
    int *nearest = NULL;

    if (row_cancelled(&hooks, k, n, bitmap->width)) {
      cancelled = TRUE;
      break;
    }
    if (flat[k]) {
      guint32 to = GPOINTER_TO_UINT(g_hash_table_lookup(minor, GUINT_TO_POINTER(pack(p, np) + 1)));
      if (to) {
        unpack(to - 1, np, variant);
        nearest = variant;
        best = 0;
      }
    } else {
      /* A pixel of a palette colour stays as it is, which the search below
         would also find, unless the colour lies exactly on an earlier
         segment: a shortcut for dithered images, where most pixels that
         aren't flat are palette colours.  */
      for (i = 0; i < palette->len; i++) {
        for (ch = 0; ch < np && p[ch] == colours[i][ch]; ch++)
          ;
        if (ch == np)
          break;
      }
      if (i < palette->len && !on_segment[i])
        continue;
    }
    for (i = 0; i < palette->len && !flat[k]; i++) {
      double d = 0;
      for (ch = 0; ch < np; ch++)
        d += (p[ch] - colours[i][ch]) * (double)(p[ch] - colours[i][ch]);
      if (d < best) {
        best = d;
        nearest = colours[i];
      }
      /* The segment from colour I to colour J.  */
      for (j = i + 1; j < palette->len; j++) {
        int c[3];
        double t;
        for (ch = 0; ch < np; ch++)
          c[ch] = p[ch];
        d = to_segment(c, colours[i], colours[j], np, &t);
        if (d < best) {
          best = d;
          nearest = t < 0.5 ? colours[i] : colours[j];
        }
      }
    }
    if (nearest && scale * best <= SEGMENT_TOLERANCE * SEGMENT_TOLERANCE) {
      gboolean differs = FALSE;
      for (ch = 0; ch < np; ch++) {
        differs |= p[ch] != nearest[ch];
        p[ch] = (unsigned char)nearest[ch];
      }
      changed += differs;
    }
  }
  g_free(colours);
  g_free(on_segment);
  g_free(flat);
  g_array_free(palette, TRUE);
  g_hash_table_destroy(minor);
  if (changed_pixels)
    *changed_pixels = changed;
  return !cancelled;
}
