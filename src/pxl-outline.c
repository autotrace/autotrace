/*
 * SPDX-FileCopyrightText: © 2000-2002 Martin Weber
 * SPDX-FileCopyrightText: © 2000-2003 Masatake YAMATO
 * SPDX-FileCopyrightText: © 2001 Dan Mills
 * SPDX-FileCopyrightText: © 2001 Peter Cucka
 * SPDX-FileCopyrightText: © 2017-2025 Peter Lemenkov
 * SPDX-FileCopyrightText: © 2018 Juergen Weigert
 * SPDX-FileCopyrightText: © 2020 Han Mertens
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

/* pxl-outline.c: find the outlines of a bitmap image; each outline is made up of one or more
   pixels; and each pixel participates via one or more edges. */

#include "autotrace.h"
#include "color.h"
#include "input.h"
#include "logreport.h"
#include "pxl-outline.h"
#include "types.h"
#include <stdio.h>

/* We consider each pixel to consist of four edges, and we travel along
   edges, instead of through pixel centers.  This is necessary for those
   unfortunate times when a single pixel is on both an inside and an
   outside outline.

   The numbers chosen here are not arbitrary; the code that figures out
   which edge to move to depends on particular values.  See the
   `TRY_PIXEL' macro in `edge.c'.  To emphasize this, I've written in the
   numbers we need for each edge value.  */

typedef enum { TOP = 1, LEFT = 2, BOTTOM = 3, RIGHT = 0, NO_EDGE = 4 } edge_type;

/* This choice is also not arbitrary: starting at the top edge makes the
   code find outside outlines before inside ones, which is certainly
   what we want.  */
#define START_EDGE top

typedef enum {
  NORTH = 0,
  NORTHWEST = 1,
  WEST = 2,
  SOUTHWEST = 3,
  SOUTH = 4,
  SOUTHEAST = 5,
  EAST = 6,
  NORTHEAST = 7
} direction_type;

#define NUM_EDGES NO_EDGE

#define COMPUTE_DELTA(axis, dir)                                                                   \
  ((dir) % 2 != 0 ? COMPUTE_##axis##_DELTA((dir) - 1) + COMPUTE_##axis##_DELTA(((dir) + 1) % 8)    \
                  : COMPUTE_##axis##_DELTA(dir))

#define COMPUTE_ROW_DELTA(dir) ((dir) == NORTH ? -1 : (dir) == SOUTH ? +1 : 0)

#define COMPUTE_COL_DELTA(dir) ((dir) == WEST ? -1 : (dir) == EAST ? +1 : 0)

static pixel_outline_type find_one_outline(at_bitmap *, edge_type, unsigned short, unsigned short,
                                           at_bitmap *, gboolean, gboolean, at_exception_type *);
static pixel_outline_type find_one_centerline(at_bitmap *, direction_type, unsigned short,
                                              unsigned short, at_bitmap *);
static void append_pixel_outline(pixel_outline_list_type *, pixel_outline_type);
static void record_top_edges(at_bitmap *, const pixel_outline_type *, guint, guint *);
static guint hole_parent(at_bitmap *, const pixel_outline_list_type *, const GArray *,
                         const guint *, unsigned short, unsigned short, at_color);
static void group_holes(pixel_outline_list_type *, const GArray *);
static pixel_outline_type new_pixel_outline(void);
static void free_pixel_outline(pixel_outline_type *);
static void concat_pixel_outline(pixel_outline_type *, const pixel_outline_type *);
static void append_outline_pixel(pixel_outline_type *, at_coord);
static void restore_square_corners(pixel_outline_type *);
static gboolean is_marked_edge(edge_type, unsigned short, unsigned short, at_bitmap *);
static gboolean is_outline_edge(edge_type, at_bitmap *, unsigned short, unsigned short, at_color,
                                at_exception_type *);
static gboolean is_unmarked_outline_edge(unsigned short, unsigned short, edge_type, at_bitmap *,
                                         at_bitmap *, at_color, at_exception_type *);

static void mark_edge(edge_type e, unsigned short, unsigned short, at_bitmap *);

static gboolean is_marked_dir(unsigned short, unsigned short, direction_type, at_bitmap *);
static gboolean is_other_dir_marked(unsigned short, unsigned short, direction_type, at_bitmap *);
static void mark_dir(unsigned short, unsigned short, direction_type, at_bitmap *);
static gboolean next_unmarked_pixel(unsigned short *, unsigned short *, direction_type *,
                                    at_bitmap *, at_bitmap *);

static gboolean is_valid_dir(unsigned short, unsigned short, direction_type, at_bitmap *,
                             at_bitmap *);

static at_coord next_point(at_bitmap *, edge_type *, unsigned short *, unsigned short *, at_color,
                           gboolean, at_bitmap *, at_exception_type *);
static unsigned num_neighbors(unsigned short, unsigned short, at_bitmap *);

/* No outside outline: see group_holes(). */
#define NO_PARENT G_MAXUINT

#define CHECK_FATAL()                                                                              \
  if (at_exception_got_fatal(exp))                                                                 \
    goto cleanup;

/* We go through a bitmap TOP to BOTTOM, LEFT to RIGHT, looking for each pixel with an unmarked edge
   that we consider a starting point of an outline. */

pixel_outline_list_type find_outline_pixels(at_bitmap *bitmap, at_color *bg_color,
                                            at_progress_func notify_progress,
                                            gpointer progress_data, at_testcancel_func test_cancel,
                                            gpointer testcancel_data, at_exception_type *exp)
{
  pixel_outline_list_type outline_list;
  unsigned short row, col;
  at_bitmap *marked = at_bitmap_new(AT_BITMAP_WIDTH(bitmap), AT_BITMAP_HEIGHT(bitmap), 1);
  unsigned int max_progress = AT_BITMAP_HEIGHT(bitmap) * AT_BITMAP_WIDTH(bitmap);
  /* Only with a background colour, to put holes in place (issue #379). */
  guint *owner =
      bg_color ? g_new0(guint, (gsize)AT_BITMAP_WIDTH(bitmap) * AT_BITMAP_HEIGHT(bitmap)) : NULL;
  GArray *parents = bg_color ? g_array_new(FALSE, FALSE, sizeof(guint)) : NULL;
  guint no_parent = NO_PARENT;

  outline_list.data = g_array_new(FALSE, FALSE, sizeof(pixel_outline_type));

  for (row = 0; row < AT_BITMAP_HEIGHT(bitmap); row++) {
    for (col = 0; col < AT_BITMAP_WIDTH(bitmap); col++) {
      edge_type edge;
      at_color color;

      if (notify_progress)
        notify_progress((gfloat)(row * AT_BITMAP_WIDTH(bitmap) + col) /
                            ((gfloat)max_progress * (gfloat)3.0),
                        progress_data);

      /* A valid edge can be TOP for an outside outline.
         Outside outlines are traced counterclockwise */
      at_bitmap_get_color(bitmap, row, col, &color);
      if (!(bg_color && at_color_equal(&color, bg_color)) &&
          is_unmarked_outline_edge(row, col, edge = TOP, bitmap, marked, color, exp)) {
        pixel_outline_type outline;

        CHECK_FATAL(); /* FREE(DONE) outline_list */

        DEBUG("#%u: (counterclockwise)", O_LIST_LENGTH(outline_list));

        outline = find_one_outline(bitmap, edge, row, col, marked, FALSE, FALSE, exp);
        CHECK_FATAL(); /* FREE(DONE) outline_list */

        O_CLOCKWISE(outline) = FALSE;
        if (bg_color) {
          record_top_edges(bitmap, &outline, O_LIST_LENGTH(outline_list), owner);
          g_array_append_val(parents, no_parent);
        }
        append_pixel_outline(&outline_list, outline);

        DEBUG(" [%u].\n", O_LENGTH(outline));
      } else
        CHECK_FATAL(); /* FREE(DONE) outline_list */

      /* A valid edge can be BOTTOM for an inside outline.
         Inside outlines are traced clockwise */
      if (row != 0) {
        at_bitmap_get_color(bitmap, row - 1, col, &color);
        if (!(bg_color && at_color_equal(&color, bg_color)) &&
            is_unmarked_outline_edge(row - 1, col, edge = BOTTOM, bitmap, marked, color, exp)) {
          pixel_outline_type outline;

          CHECK_FATAL(); /* FREE(DONE) outline_list */

          /* Without a background colour, every region is traced and
             painted, so the regions inside a hole cover it and the hole
             itself is only marked. With one, a hole can also contain
             background, which nothing paints, so keep every hole. It used
             to be kept only if the pixel below this edge was background,
             but in anti-aliased input that pixel is usually an edge shade,
             with the background further in (issue #379). */
          if (bg_color) {
            DEBUG("#%u: (clockwise)", O_LIST_LENGTH(outline_list));

            outline = find_one_outline(bitmap, edge, row - 1, col, marked, TRUE, FALSE, exp);
            CHECK_FATAL(); /* FREE(DONE) outline_list */

            O_CLOCKWISE(outline) = TRUE;
            {
              guint parent =
                  hole_parent(bitmap, &outline_list, parents, owner, row - 1, col, color);

              record_top_edges(bitmap, &outline, O_LIST_LENGTH(outline_list), owner);
              g_array_append_val(parents, parent);
            }
            append_pixel_outline(&outline_list, outline);

            DEBUG(" [%u].\n", O_LENGTH(outline));
          } else {
            outline = find_one_outline(bitmap, edge, row - 1, col, marked, TRUE, TRUE, exp);
            CHECK_FATAL(); /* FREE(DONE) outline_list */
          }
        } else
          CHECK_FATAL(); /* FREE(DONE) outline_list */
      }
      if (test_cancel && test_cancel(testcancel_data)) {
        free_pixel_outline_list(&outline_list);
        goto cleanup;
      }
    }
  }
  if (bg_color)
    group_holes(&outline_list, parents);
cleanup:
  at_bitmap_free(marked);
  g_free(owner);
  if (parents)
    g_array_free(parents, TRUE);
  if (at_exception_got_fatal(exp))
    free_pixel_outline_list(&outline_list);
  return outline_list;
}

/* We calculate one single outline here. We pass the position of the starting pixel and the
   starting edge. All edges we track along will be marked and the outline pixels are appended
   to the coordinate list. */

static pixel_outline_type find_one_outline(at_bitmap *bitmap, edge_type original_edge,
                                           unsigned short original_row, unsigned short original_col,
                                           at_bitmap *marked, gboolean clockwise, gboolean ignore,
                                           at_exception_type *exp)
{
  pixel_outline_type outline = {0};
  unsigned short row = original_row, col = original_col;
  edge_type edge = original_edge;
  at_coord pos;

  pos.x = col + ((edge == RIGHT) || (edge == BOTTOM) ? 1 : 0);
  pos.y = AT_BITMAP_HEIGHT(bitmap) - row - 1 + ((edge == TOP) || (edge == RIGHT) ? 1 : 0);

  if (!ignore)
    outline = new_pixel_outline();

  at_bitmap_get_color(bitmap, row, col, &outline.color);

  do {
    /* Put this edge into the output list */
    if (!ignore) {
      DEBUG(" (%d,%d)", pos.x, pos.y);
      append_outline_pixel(&outline, pos);
    }

    mark_edge(edge, row, col, marked);
    pos = next_point(bitmap, &edge, &row, &col, outline.color, clockwise, marked, exp);
    CHECK_FATAL();
  } while (edge != NO_EDGE);

cleanup:
  if (at_exception_got_fatal(exp))
    free_pixel_outline(&outline);
  return outline;
}

static gboolean is_valid_dir(unsigned short row, unsigned short col, direction_type dir,
                             at_bitmap *bitmap, at_bitmap *marked)
{

  at_color c;
  int new_row = COMPUTE_DELTA(ROW, dir) + row;
  int new_col = COMPUTE_DELTA(COL, dir) + col;

  if ((new_row < 0) || (new_col < 0) || (new_row >= AT_BITMAP_HEIGHT(bitmap)) ||
      (new_col >= AT_BITMAP_WIDTH(bitmap)))
    return FALSE; // Must not call at_bitmap_get_color() with negative row or col.

  at_bitmap_get_color(bitmap, COMPUTE_DELTA(ROW, dir) + row, COMPUTE_DELTA(COL, dir) + col, &c);
  return ((gboolean)(!is_marked_dir(row, col, dir, marked) && COMPUTE_DELTA(ROW, dir) + row > 0 &&
                     COMPUTE_DELTA(COL, dir) + col > 0 &&
                     AT_BITMAP_VALID_PIXEL(bitmap, COMPUTE_DELTA(ROW, dir) + row,
                                           COMPUTE_DELTA(COL, dir) + col) &&
                     at_bitmap_equal_color(bitmap, row, col, &c)));
}

pixel_outline_list_type find_centerline_pixels(at_bitmap *bitmap, at_color bg_color,
                                               at_progress_func notify_progress,
                                               gpointer progress_data,
                                               at_testcancel_func test_cancel,
                                               gpointer testcancel_data, at_exception_type *exp)
{
  pixel_outline_list_type outline_list;
  unsigned int row, col;
  at_bitmap *marked = at_bitmap_new(AT_BITMAP_WIDTH(bitmap), AT_BITMAP_HEIGHT(bitmap), 1);
  unsigned int max_progress = AT_BITMAP_HEIGHT(bitmap) * AT_BITMAP_WIDTH(bitmap);

  outline_list.data = g_array_new(FALSE, FALSE, sizeof(pixel_outline_type));

  for (row = 0; row < AT_BITMAP_HEIGHT(bitmap); row++) {
    for (col = 0; col < AT_BITMAP_WIDTH(bitmap);) {
      direction_type dir = EAST;
      pixel_outline_type outline;
      gboolean clockwise = FALSE;

      if (notify_progress)
        notify_progress((gfloat)(row * AT_BITMAP_WIDTH(bitmap) + col) /
                            ((gfloat)max_progress * (gfloat)3.0),
                        progress_data);

      if (at_bitmap_equal_color(bitmap, row, col, &bg_color)) {
        col++;
        continue;
      }

      if (!is_valid_dir(row, col, dir, bitmap, marked) ||
          (!is_valid_dir(COMPUTE_DELTA(ROW, dir) + row, COMPUTE_DELTA(COL, dir) + col, dir, bitmap,
                         marked) &&
           num_neighbors(row, col, bitmap) > 2) ||
          num_neighbors(row, col, bitmap) > 4 ||
          num_neighbors(COMPUTE_DELTA(ROW, dir) + row, COMPUTE_DELTA(COL, dir) + col, bitmap) > 4 ||
          (is_other_dir_marked(row, col, dir, marked) &&
           is_other_dir_marked(row + COMPUTE_DELTA(ROW, dir), col + COMPUTE_DELTA(COL, dir), dir,
                               marked))) {
        dir = SOUTHEAST;
        if (!is_valid_dir(row, col, dir, bitmap, marked) ||
            (!is_valid_dir(COMPUTE_DELTA(ROW, dir) + row, COMPUTE_DELTA(COL, dir) + col, dir,
                           bitmap, marked) &&
             num_neighbors(row, col, bitmap) > 2) ||
            num_neighbors(row, col, bitmap) > 4 ||
            num_neighbors(COMPUTE_DELTA(ROW, dir) + row, COMPUTE_DELTA(COL, dir) + col, bitmap) >
                4 ||
            (is_other_dir_marked(row, col, dir, marked) &&
             is_other_dir_marked(row + COMPUTE_DELTA(ROW, dir), col + COMPUTE_DELTA(COL, dir), dir,
                                 marked))) {
          dir = SOUTH;
          if (!is_valid_dir(row, col, dir, bitmap, marked) ||
              (!is_valid_dir(COMPUTE_DELTA(ROW, dir) + row, COMPUTE_DELTA(COL, dir) + col, dir,
                             bitmap, marked) &&
               num_neighbors(row, col, bitmap) > 2) ||
              num_neighbors(row, col, bitmap) > 4 ||
              num_neighbors(COMPUTE_DELTA(ROW, dir) + row, COMPUTE_DELTA(COL, dir) + col, bitmap) >
                  4 ||
              (is_other_dir_marked(row, col, dir, marked) &&
               is_other_dir_marked(row + COMPUTE_DELTA(ROW, dir), col + COMPUTE_DELTA(COL, dir),
                                   dir, marked))) {
            dir = SOUTHWEST;
            if (!is_valid_dir(row, col, dir, bitmap, marked) ||
                (!is_valid_dir(COMPUTE_DELTA(ROW, dir) + row, COMPUTE_DELTA(COL, dir) + col, dir,
                               bitmap, marked) &&
                 num_neighbors(row, col, bitmap) > 2) ||
                num_neighbors(row, col, bitmap) > 4 ||
                num_neighbors(COMPUTE_DELTA(ROW, dir) + row, COMPUTE_DELTA(COL, dir) + col,
                              bitmap) > 4 ||
                (is_other_dir_marked(row, col, dir, marked) &&
                 is_other_dir_marked(row + COMPUTE_DELTA(ROW, dir), col + COMPUTE_DELTA(COL, dir),
                                     dir, marked))) {
              col++;
              continue;
            }
          }
        }
      }

      DEBUG("#%u: (%sclockwise) ", O_LIST_LENGTH(outline_list), clockwise ? "" : "counter");

      outline = find_one_centerline(bitmap, dir, row, col, marked);

      /* If the outline is open (i.e., we didn't return to the
         starting pixel), search from the starting pixel in the
         opposite direction and concatenate the two outlines. */

      if (outline.open) {
        pixel_outline_type partial_outline;
        gboolean okay = FALSE;

        if (dir == EAST) {
          dir = SOUTH;
          if (!(okay = is_valid_dir(row, col, dir, bitmap, marked))) {
            dir = SOUTHWEST;
            if (!(okay = is_valid_dir(row, col, dir, bitmap, marked))) {
              dir = SOUTHEAST;
              okay = is_valid_dir(row, col, dir, bitmap, marked);
            }
          }
        } else if (dir == SOUTHEAST) {
          dir = SOUTHWEST;
          if (!(okay = is_valid_dir(row, col, dir, bitmap, marked))) {
            dir = EAST;
            if (!(okay = is_valid_dir(row, col, dir, bitmap, marked))) {
              dir = SOUTH;
              okay = is_valid_dir(row, col, dir, bitmap, marked);
            }
          }
        } else if (dir == SOUTH) {
          dir = EAST;
          if (!(okay = is_valid_dir(row, col, dir, bitmap, marked))) {
            dir = SOUTHEAST;
            if (!(okay = is_valid_dir(row, col, dir, bitmap, marked))) {
              dir = SOUTHWEST;
              okay = is_valid_dir(row, col, dir, bitmap, marked);
            }
          }
        } else if (dir == SOUTHWEST) {
          dir = SOUTHEAST;
          if (!(okay = is_valid_dir(row, col, dir, bitmap, marked))) {
            dir = EAST;
            if (!(okay = is_valid_dir(row, col, dir, bitmap, marked))) {
              dir = SOUTH;
              okay = is_valid_dir(row, col, dir, bitmap, marked);
            }
          }
        }
        if (okay) {
          partial_outline = find_one_centerline(bitmap, dir, row, col, marked);
          concat_pixel_outline(&outline, &partial_outline);
          free_pixel_outline(&partial_outline);
        } else
          col++;
      }

      /* Outside outlines will start at a top edge, and move
         counterclockwise, and inside outlines will start at a
         bottom edge, and move clockwise.  This happens because of
         the order in which we look at the edges. */
      O_CLOCKWISE(outline) = clockwise;
      if (O_LENGTH(outline) > 1) {
        restore_square_corners(&outline);
        append_pixel_outline(&outline_list, outline);
      }
      DEBUG("(%s)", (outline.open ? " open" : " closed"));
      DEBUG(" [%u].\n", O_LENGTH(outline));
      if (O_LENGTH(outline) == 1)
        free_pixel_outline(&outline);
    }
  }
  if (test_cancel && test_cancel(testcancel_data)) {
    if (O_LIST_LENGTH(outline_list) != 0)
      free_pixel_outline_list(&outline_list);
    goto cleanup;
  }
cleanup:
  at_bitmap_free(marked);
  return outline_list;
}

static pixel_outline_type find_one_centerline(at_bitmap *bitmap, direction_type search_dir,
                                              unsigned short original_row,
                                              unsigned short original_col, at_bitmap *marked)
{
  pixel_outline_type outline = new_pixel_outline();
  direction_type original_dir = search_dir;
  unsigned short row = original_row, col = original_col;
  unsigned short prev_row, prev_col;
  at_coord pos;

  outline.open = FALSE;
  at_bitmap_get_color(bitmap, row, col, &outline.color);

  /* Add the starting pixel to the output list, changing from bitmap
     to Cartesian coordinates and specifying the left edge so that
     the coordinates won't be adjusted. */
  pos.x = col;
  pos.y = AT_BITMAP_HEIGHT(bitmap) - row - 1;
  DEBUG(" (%d,%d)", pos.x, pos.y);
  append_outline_pixel(&outline, pos);

  for (;;) {
    prev_row = row;
    prev_col = col;

    /* If there is no adjacent, unmarked pixel, we can't proceed
       any further, so return an open outline. */
    if (!next_unmarked_pixel(&row, &col, &search_dir, bitmap, marked)) {
      outline.open = TRUE;
      break;
    }

    /* If we've moved to a new pixel, mark all edges of the previous
       pixel so that it won't be revisited. */
    if (!(prev_row == original_row && prev_col == original_col))
      mark_dir(prev_row, prev_col, search_dir, marked);
    mark_dir(row, col, (direction_type)((search_dir + 4) % 8), marked);

    /* If we've returned to the starting pixel, we're done. */
    if (row == original_row && col == original_col)
      break;

    /* Add the new pixel to the output list. */
    pos.x = col;
    pos.y = AT_BITMAP_HEIGHT(bitmap) - row - 1;
    DEBUG(" (%d,%d)", pos.x, pos.y);
    append_outline_pixel(&outline, pos);
  }
  mark_dir(original_row, original_col, original_dir, marked);
  return outline;
}

/* Add an outline to an outline list. */

static void append_pixel_outline(pixel_outline_list_type *outline_list, pixel_outline_type outline)
{
  g_array_append_val(outline_list->data, outline);
}

/* With a background colour, holes are kept, and each must follow the outside
   outline it is a hole in, together with that outline's other holes. The
   output formats join consecutive outlines of one colour into one path, so
   that the hole is cut out of it; with outlines of other colours in between,
   the hole would be filled instead (issue #379).

   To find that outline, OWNER records, for the top edge of each pixel, the
   outline of the pixel's colour that runs along it, as its index plus one.
   PARENTS gives, for each outline in the list, the index of the outside
   outline a hole belongs to, or NO_PARENT. */

static void record_top_edges(at_bitmap *bitmap, const pixel_outline_type *outline, guint index,
                             guint *owner)
{
  unsigned short width = AT_BITMAP_WIDTH(bitmap), height = AT_BITMAP_HEIGHT(bitmap);
  unsigned i, n = O_LENGTH(*outline);

  for (i = 0; i < n; i++) {
    at_coord a = O_COORDINATE(*outline, i), b = O_COORDINATE(*outline, O_NEXT(*outline, i));
    unsigned short row, col;
    at_color c;

    /* A horizontal edge at height Y is the top edge of row HEIGHT - Y. */
    if (a.y != b.y || a.y == 0 || a.y > height || (a.x > b.x ? a.x - b.x : b.x - a.x) != 1)
      continue;
    row = height - a.y;
    col = MIN(a.x, b.x);
    if (col >= width)
      continue;
    at_bitmap_get_color(bitmap, row, col, &c);
    if (at_color_equal(&c, &outline->color))
      owner[(gsize)row * width + col] = index + 1;
  }
}

/* The outside outline of the region that the pixel at ROW, COL belongs to,
   for a hole found just below that pixel. Going up from it through the
   region's colour leads to a top edge of the region, which is either on its
   outside outline or on one of its holes, traced already in both cases. */
static guint hole_parent(at_bitmap *bitmap, const pixel_outline_list_type *list,
                         const GArray *parents, const guint *owner, unsigned short row,
                         unsigned short col, at_color color)
{
  guint index;

  for (; row > 0; row--) {
    at_color above;

    at_bitmap_get_color(bitmap, row - 1, col, &above);
    if (!at_color_equal(&above, &color))
      break;
  }
  index = owner[(gsize)row * AT_BITMAP_WIDTH(bitmap) + col];
  if (index == 0) /* Not expected; the hole stays where it is. */
    return NO_PARENT;
  index--;
  return O_CLOCKWISE(O_LIST_OUTLINE(*list, index)) ? g_array_index(parents, guint, index) : index;
}

/* Reorder LIST so that each hole follows its outside outline, in the order
   they were found. */
static void group_holes(pixel_outline_list_type *list, const GArray *parents)
{
  guint n = O_LIST_LENGTH(*list), i;
  guint *first = g_new(guint, n), *last = g_new(guint, n), *next = g_new(guint, n);
  GArray *grouped = g_array_sized_new(FALSE, FALSE, sizeof(pixel_outline_type), n);

  for (i = 0; i < n; i++)
    first[i] = next[i] = NO_PARENT;
  for (i = 0; i < n; i++) {
    guint p = g_array_index(parents, guint, i);

    if (p == NO_PARENT)
      continue;
    if (first[p] == NO_PARENT)
      first[p] = i;
    else
      next[last[p]] = i;
    last[p] = i;
  }
  for (i = 0; i < n; i++) {
    guint h;

    if (g_array_index(parents, guint, i) != NO_PARENT)
      continue;
    g_array_append_val(grouped, O_LIST_OUTLINE(*list, i));
    for (h = first[i]; h != NO_PARENT; h = next[h])
      g_array_append_val(grouped, O_LIST_OUTLINE(*list, h));
  }
  g_array_free(list->data, TRUE);
  list->data = grouped;
  g_free(first);
  g_free(last);
  g_free(next);
}

/* Free the list of outline lists. */

void free_pixel_outline_list(pixel_outline_list_type *outline_list)
{
  unsigned this_outline;

  for (this_outline = 0; this_outline < O_LIST_LENGTH(*outline_list); this_outline++)
    free_pixel_outline(&O_LIST_OUTLINE(*outline_list, this_outline));
  g_array_free(outline_list->data, TRUE);
  outline_list->data = NULL;
}

/* Return an empty list of pixels.  */

static pixel_outline_type new_pixel_outline(void)
{
  pixel_outline_type pixel_outline;

  pixel_outline.data = g_array_new(FALSE, FALSE, sizeof(at_coord));
  pixel_outline.open = FALSE;

  return pixel_outline;
}

static void free_pixel_outline(pixel_outline_type *outline)
{
  if (outline->data)
    g_array_free(outline->data, TRUE);
  outline->data = NULL;
}

/* Concatenate two pixel lists. The two lists are assumed to have the
   same starting pixel and to proceed in opposite directions therefrom. */

static void concat_pixel_outline(pixel_outline_type *o1, const pixel_outline_type *o2)
{
  GArray *joined;
  int src;

  if (!o1 || !o2 || O_LENGTH(*o2) <= 1)
    return;

  /* The result is o2 backwards, without its first pixel (that is the one
     shared with o1), followed by o1.  */
  joined = g_array_sized_new(FALSE, FALSE, sizeof(at_coord), O_LENGTH(*o1) + O_LENGTH(*o2) - 1);
  for (src = O_LENGTH(*o2) - 1; src > 0; src--)
    g_array_append_val(joined, O_COORDINATE(*o2, src));
  g_array_append_vals(joined, o1->data->data, O_LENGTH(*o1));

  g_array_free(o1->data, TRUE);
  o1->data = joined;
}

/* Add a point to the pixel list. */

static void append_outline_pixel(pixel_outline_type *o, at_coord c)
{
  g_array_append_val(o->data, c);
}

/* Is this really an edge and is it still unmarked? */

static gboolean is_unmarked_outline_edge(unsigned short row, unsigned short col, edge_type edge,
                                         at_bitmap *bitmap, at_bitmap *marked, at_color color,
                                         at_exception_type *exp)
{
  return (gboolean)(!is_marked_edge(edge, row, col, marked) &&
                    is_outline_edge(edge, bitmap, row, col, color, exp));
}

/* We check to see if the edge of the pixel at position ROW and COL
   is an outline edge */

static gboolean is_outline_edge(edge_type edge, at_bitmap *bitmap, unsigned short row,
                                unsigned short col, at_color color, at_exception_type *exp)
{
  /* If this pixel isn't of the same color, it's not part of the outline. */
  if (!at_bitmap_equal_color(bitmap, row, col, &color))
    return FALSE;

  switch (edge) {
  case LEFT:
    return (gboolean)(col == 0 || !at_bitmap_equal_color(bitmap, row, col - 1, &color));
  case TOP:
    return (gboolean)(row == 0 || !at_bitmap_equal_color(bitmap, row - 1, col, &color));

  case RIGHT:
    return (gboolean)(col == AT_BITMAP_WIDTH(bitmap) - 1 ||
                      !at_bitmap_equal_color(bitmap, row, col + 1, &color));

  case BOTTOM:
    return (gboolean)(row == AT_BITMAP_HEIGHT(bitmap) - 1 ||
                      !at_bitmap_equal_color(bitmap, row + 1, col, &color));

  case NO_EDGE:
    g_assert_not_reached();
  default:
    g_assert_not_reached();
  }
  return FALSE; /* NOT REACHED */
}

/* If EDGE is not already marked, we mark it; otherwise, it's a fatal error.
   The position ROW and COL should be inside the bitmap MARKED. EDGE can be
   NO_EDGE. */

static void mark_edge(edge_type edge, unsigned short row, unsigned short col, at_bitmap *marked)
{
  *AT_BITMAP_PIXEL(marked, row, col) |= 1 << edge;
}

/* Mark the direction of the pixel ROW/COL in MARKED. */

static void mark_dir(unsigned short row, unsigned short col, direction_type dir, at_bitmap *marked)
{
  *AT_BITMAP_PIXEL(marked, row, col) |= 1 << dir;
}

/* Test if the direction of pixel at ROW/COL in MARKED is marked. */

static gboolean is_marked_dir(unsigned short row, unsigned short col, direction_type dir,
                              at_bitmap *marked)
{
  return (gboolean)((*AT_BITMAP_PIXEL(marked, row, col) & 1 << dir) != 0);
}

static gboolean is_other_dir_marked(unsigned short row, unsigned short col, direction_type dir,
                                    at_bitmap *marked)
{
  return (gboolean)((*AT_BITMAP_PIXEL(marked, row, col) &
                     (255 - (1 << dir) - (1 << ((dir + 4) % 8)))) != 0);
}

static gboolean next_unmarked_pixel(unsigned short *row, unsigned short *col, direction_type *dir,
                                    at_bitmap *bitmap, at_bitmap *marked)
{
  unsigned short orig_row = *row, orig_col = *col;
  direction_type orig_dir = *dir, test_dir = *dir;

  do {
    if (is_valid_dir(orig_row, orig_col, test_dir, bitmap, marked)) {
      *row = orig_row + COMPUTE_DELTA(ROW, test_dir);
      *col = orig_col + COMPUTE_DELTA(COL, test_dir);
      *dir = test_dir;
      break;
    }

    if (orig_dir == test_dir)
      test_dir = (direction_type)((orig_dir + 2) % 8);
    else if ((orig_dir + 2) % 8 == test_dir)
      test_dir = (direction_type)((orig_dir + 6) % 8);
    else if ((orig_dir + 6) % 8 == test_dir)
      test_dir = (direction_type)((orig_dir + 1) % 8);
    else if ((orig_dir + 1) % 8 == test_dir)
      test_dir = (direction_type)((orig_dir + 7) % 8);
    else if ((orig_dir + 7) % 8 == test_dir)
      test_dir = (direction_type)((orig_dir + 3) % 8);
    else if ((orig_dir + 3) % 8 == test_dir)
      test_dir = (direction_type)((orig_dir + 5) % 8);
    else if ((orig_dir + 5) % 8 == test_dir)
      break;
  } while (1);
  if ((*row != orig_row || *col != orig_col) &&
      (!(is_other_dir_marked(orig_row, orig_col, test_dir, marked) &&
         is_other_dir_marked(orig_row + COMPUTE_DELTA(ROW, test_dir),
                             orig_col + COMPUTE_DELTA(COL, test_dir), test_dir, marked))))
    return TRUE;
  else
    return FALSE;
}

/* Return the number of pixels adjacent to pixel ROW/COL that are black. */

static unsigned num_neighbors(unsigned short row, unsigned short col, at_bitmap *bitmap)
{
  unsigned dir, count = 0;
  at_color color;

  at_bitmap_get_color(bitmap, row, col, &color);
  for (dir = NORTH; dir <= NORTHEAST; dir++) {
    int delta_r = COMPUTE_DELTA(ROW, dir);
    int delta_c = COMPUTE_DELTA(COL, dir);
    unsigned int test_row = row + delta_r;
    unsigned int test_col = col + delta_c;
    if (AT_BITMAP_VALID_PIXEL(bitmap, test_row, test_col) &&
        at_bitmap_equal_color(bitmap, test_row, test_col, &color))
      ++count;
  }
  return count;
}

/* Test if the edge EDGE at ROW/COL in MARKED is marked.  */

static gboolean is_marked_edge(edge_type edge, unsigned short row, unsigned short col,
                               at_bitmap *marked)
{
  return (gboolean)(edge == NO_EDGE ? FALSE
                                    : (*AT_BITMAP_PIXEL(marked, row, col) & (1 << edge)) != 0);
}

/* Both edges EDGE1 at ROW1/COL1 and EDGE2 at ROW2/COL2 are marked in MARKED.  */
static gboolean both_edges_marked(edge_type edge1, unsigned short row1, unsigned short col1,
                                  edge_type edge2, unsigned short row2, unsigned short col2,
                                  at_bitmap *marked)
{
  return is_marked_edge(edge1, row1, col1, marked) && is_marked_edge(edge2, row2, col2, marked);
}

/* In next_point(), the bitmap, marked edges, colour and exception are the same for
   every edge it tests.  */
#define UNMARKED_OUTLINE_EDGE(edge, row, col)                                                      \
  is_unmarked_outline_edge(row, col, edge, bitmap, marked, color, exp)

static at_coord next_point(at_bitmap *bitmap, edge_type *edge, unsigned short *row,
                           unsigned short *col, at_color color, gboolean clockwise,
                           at_bitmap *marked, at_exception_type *exp)
{
  at_coord pos = {0, 0};

  if (!clockwise)
    switch (*edge) {
    case TOP:
      /* WEST */
      if (*col >= 1 && UNMARKED_OUTLINE_EDGE(TOP, *row, *col - 1)) {
        (*col)--;
        pos.x = *col;
        pos.y = AT_BITMAP_HEIGHT(bitmap) - *row;
        break;
      }
      CHECK_FATAL();
      /* NORTHWEST */
      if ((*col >= 1 && *row >= 1 && UNMARKED_OUTLINE_EDGE(RIGHT, *row - 1, *col - 1)) &&
          !both_edges_marked(LEFT, *row - 1, *col, TOP, *row, *col - 1, marked) &&
          !both_edges_marked(BOTTOM, *row - 1, *col, RIGHT, *row, *col - 1, marked)) {
        *edge = RIGHT;
        (*col)--;
        (*row)--;
        pos.x = *col + 1;
        pos.y = AT_BITMAP_HEIGHT(bitmap) - *row;
        break;
      }
      CHECK_FATAL();
      if (UNMARKED_OUTLINE_EDGE(LEFT, *row, *col)) {
        *edge = LEFT;
        pos.x = *col;
        pos.y = AT_BITMAP_HEIGHT(bitmap) - *row - 1;
        break;
      }
      CHECK_FATAL();
      *edge = NO_EDGE;
      break;
    case RIGHT:
      /* NORTH */
      if (*row >= 1 && UNMARKED_OUTLINE_EDGE(RIGHT, *row - 1, *col)) {
        (*row)--;
        pos.x = *col + 1;
        pos.y = AT_BITMAP_HEIGHT(bitmap) - *row;
        break;
      }
      CHECK_FATAL();
      /* NORTHEAST */
      if ((*col + 1 < AT_BITMAP_WIDTH(marked) && *row >= 1 &&
           UNMARKED_OUTLINE_EDGE(BOTTOM, *row - 1, *col + 1)) &&
          !both_edges_marked(LEFT, *row, *col + 1, BOTTOM, *row - 1, *col, marked) &&
          !both_edges_marked(TOP, *row, *col + 1, RIGHT, *row - 1, *col, marked)) {
        *edge = BOTTOM;
        (*col)++;
        (*row)--;
        pos.x = *col + 1;
        pos.y = AT_BITMAP_HEIGHT(bitmap) - *row - 1;
        break;
      }
      CHECK_FATAL();
      if (UNMARKED_OUTLINE_EDGE(TOP, *row, *col)) {
        *edge = TOP;
        pos.x = *col;
        pos.y = AT_BITMAP_HEIGHT(bitmap) - *row;
        break;
      }
      CHECK_FATAL();
      *edge = NO_EDGE;
      break;
    case BOTTOM:
      /* EAST */
      if (*col + 1 < AT_BITMAP_WIDTH(marked) && UNMARKED_OUTLINE_EDGE(BOTTOM, *row, *col + 1)) {
        (*col)++;
        pos.x = *col + 1;
        pos.y = AT_BITMAP_HEIGHT(bitmap) - *row - 1;
        break;
      }
      CHECK_FATAL();
      /* SOUTHEAST */
      if ((*col + 1 < AT_BITMAP_WIDTH(marked) && *row + 1 < AT_BITMAP_HEIGHT(marked) &&
           UNMARKED_OUTLINE_EDGE(LEFT, *row + 1, *col + 1)) &&
          !both_edges_marked(TOP, *row + 1, *col, LEFT, *row, *col + 1, marked) &&
          !both_edges_marked(RIGHT, *row + 1, *col, BOTTOM, *row, *col + 1, marked)) {
        *edge = LEFT;
        (*col)++;
        (*row)++;
        pos.x = *col;
        pos.y = AT_BITMAP_HEIGHT(bitmap) - *row - 1;
        break;
      }
      CHECK_FATAL();
      if (UNMARKED_OUTLINE_EDGE(RIGHT, *row, *col)) {
        *edge = RIGHT;
        pos.x = *col + 1;
        pos.y = AT_BITMAP_HEIGHT(bitmap) - *row;
        break;
      }
      CHECK_FATAL();
      *edge = NO_EDGE;
      break;
    case LEFT:
      /* SOUTH */
      if (*row + 1 < AT_BITMAP_HEIGHT(marked) && UNMARKED_OUTLINE_EDGE(LEFT, *row + 1, *col)) {
        (*row)++;
        pos.x = *col;
        pos.y = AT_BITMAP_HEIGHT(bitmap) - *row - 1;
        break;
      }
      CHECK_FATAL();
      /* SOUTHWEST */
      if ((*col >= 1 && *row + 1 < AT_BITMAP_HEIGHT(marked) &&
           UNMARKED_OUTLINE_EDGE(TOP, *row + 1, *col - 1)) &&
          !both_edges_marked(RIGHT, *row, *col - 1, TOP, *row + 1, *col, marked) &&
          !both_edges_marked(BOTTOM, *row, *col - 1, LEFT, *row + 1, *col, marked)) {
        *edge = TOP;
        (*col)--;
        (*row)++;
        pos.x = *col;
        pos.y = AT_BITMAP_HEIGHT(bitmap) - *row;
        break;
      }
      CHECK_FATAL();
      if (UNMARKED_OUTLINE_EDGE(BOTTOM, *row, *col)) {
        *edge = BOTTOM;
        pos.x = *col + 1;
        pos.y = AT_BITMAP_HEIGHT(bitmap) - *row - 1;
        break;
      }
      CHECK_FATAL();
      /* fall through */
    case NO_EDGE:
    default:
      *edge = NO_EDGE;
      break;
    }
  else
    switch (*edge) {
    case TOP:
      if (UNMARKED_OUTLINE_EDGE(LEFT, *row, *col)) {
        *edge = LEFT;
        pos.x = *col;
        pos.y = AT_BITMAP_HEIGHT(bitmap) - *row - 1;
        break;
      }
      CHECK_FATAL();
      /* WEST */
      if (*col >= 1 && UNMARKED_OUTLINE_EDGE(TOP, *row, *col - 1)) {
        (*col)--;
        pos.x = *col;
        pos.y = AT_BITMAP_HEIGHT(bitmap) - *row;
        break;
      }
      CHECK_FATAL();
      /* NORTHWEST */
      if (*col >= 1 && *row >= 1 && UNMARKED_OUTLINE_EDGE(RIGHT, *row - 1, *col - 1)) {
        *edge = RIGHT;
        (*col)--;
        (*row)--;
        pos.x = *col + 1;
        pos.y = AT_BITMAP_HEIGHT(bitmap) - *row;
        break;
      }
      CHECK_FATAL();
      *edge = NO_EDGE;
      break;
    case RIGHT:
      if (UNMARKED_OUTLINE_EDGE(TOP, *row, *col)) {
        *edge = TOP;
        pos.x = *col;
        pos.y = AT_BITMAP_HEIGHT(bitmap) - *row;
        break;
      }
      CHECK_FATAL();
      /* NORTH */
      if (*row >= 1 && UNMARKED_OUTLINE_EDGE(RIGHT, *row - 1, *col)) {
        (*row)--;
        pos.x = *col + 1;
        pos.y = AT_BITMAP_HEIGHT(bitmap) - *row;
        break;
      }
      CHECK_FATAL();
      /* NORTHEAST */
      if (*col + 1 < AT_BITMAP_WIDTH(marked) && *row >= 1 &&
          UNMARKED_OUTLINE_EDGE(BOTTOM, *row - 1, *col + 1)) {
        *edge = BOTTOM;
        (*col)++;
        (*row)--;
        pos.x = *col + 1;
        pos.y = AT_BITMAP_HEIGHT(bitmap) - *row - 1;
        break;
      }
      CHECK_FATAL();
      *edge = NO_EDGE;
      break;
    case BOTTOM:
      if (UNMARKED_OUTLINE_EDGE(RIGHT, *row, *col)) {
        *edge = RIGHT;
        pos.x = *col + 1;
        pos.y = AT_BITMAP_HEIGHT(bitmap) - *row;
        break;
      }
      CHECK_FATAL();
      /* EAST */
      if (*col + 1 < AT_BITMAP_WIDTH(marked) && UNMARKED_OUTLINE_EDGE(BOTTOM, *row, *col + 1)) {
        (*col)++;
        pos.x = *col + 1;
        pos.y = AT_BITMAP_HEIGHT(bitmap) - *row - 1;
        break;
      }
      CHECK_FATAL();
      /* SOUTHEAST */
      if (*col + 1 < AT_BITMAP_WIDTH(marked) && *row + 1 < AT_BITMAP_HEIGHT(marked) &&
          UNMARKED_OUTLINE_EDGE(LEFT, *row + 1, *col + 1)) {
        *edge = LEFT;
        (*col)++;
        (*row)++;
        pos.x = *col;
        pos.y = AT_BITMAP_HEIGHT(bitmap) - *row - 1;
        break;
      }
      CHECK_FATAL();
      *edge = NO_EDGE;
      break;
    case LEFT:
      if (UNMARKED_OUTLINE_EDGE(BOTTOM, *row, *col)) {
        *edge = BOTTOM;
        pos.x = *col + 1;
        pos.y = AT_BITMAP_HEIGHT(bitmap) - *row - 1;
        break;
      }
      CHECK_FATAL();
      /* SOUTH */
      if (*row + 1 < AT_BITMAP_HEIGHT(marked) && UNMARKED_OUTLINE_EDGE(LEFT, *row + 1, *col)) {
        (*row)++;
        pos.x = *col;
        pos.y = AT_BITMAP_HEIGHT(bitmap) - *row - 1;
        break;
      }
      CHECK_FATAL();
      /* SOUTHWEST */
      if (*col >= 1 && *row + 1 < AT_BITMAP_HEIGHT(marked) &&
          UNMARKED_OUTLINE_EDGE(TOP, *row + 1, *col - 1)) {
        *edge = TOP;
        (*col)--;
        (*row)++;
        pos.x = *col;
        pos.y = AT_BITMAP_HEIGHT(bitmap) - *row;
        break;
      }
      CHECK_FATAL();
      /* fall through */
    case NO_EDGE:
    default:
      *edge = NO_EDGE;
      break;
    }
cleanup:
  return (pos);
}

#undef UNMARKED_OUTLINE_EDGE

/* The step from A to B, which must be one pixel in each direction at most.  */
static at_coord step(at_coord a, at_coord b)
{
  at_coord d;

  d.x = (unsigned short)(b.x - a.x);
  d.y = (unsigned short)(b.y - a.y);
  return d;
}

static gboolean same_step(at_coord a, at_coord b)
{
  return a.x == b.x && a.y == b.y;
}

/* A horizontal or vertical step.  Steps are stored as unsigned short, so -1
   is 0xffff; only zero matters here.  */
static gboolean is_straight_step(at_coord d)
{
  return (d.x == 0) != (d.y == 0);
}

/* Thinning turns the corner of a right angle into a one-pixel diagonal
   step: a horizontal run ends at A, and a vertical run starts at B, one
   pixel diagonally from A (or the other way round).  find_corners() then
   sees two pixels with an angle of more than 100 degrees each instead of
   one with 90 degrees, so with the default -corner-threshold the corner is
   lost and fitted as a curve, and with a lower threshold it lands one
   pixel off on one side (issue #50).

   Put the corner pixel back where the two runs meet.  It was part of the
   line before thinning.  Both runs must be at least two steps long, so
   that the one-pixel diagonals on small curves are left alone.  */
static void restore_square_corners(pixel_outline_type *outline)
{
  unsigned n = O_LENGTH(*outline), k;
  gboolean closed = !outline->open;
  GArray *out;

  if (n < 6)
    return;

  out = g_array_sized_new(FALSE, FALSE, sizeof(at_coord), n + 4);
  for (k = 0; k < n; k++) {
    /* Is the step from point k-1 to point k a diagonal corner step?  For
       an open outline the runs must lie within it.  */
    if (closed || (k >= 3 && k + 2 < n)) {
      at_coord pm3 = O_COORDINATE(*outline, (k + n - 3) % n);
      at_coord pm2 = O_COORDINATE(*outline, (k + n - 2) % n);
      at_coord pm1 = O_COORDINATE(*outline, (k + n - 1) % n);
      at_coord p0 = O_COORDINATE(*outline, k);
      at_coord p1 = O_COORDINATE(*outline, (k + 1) % n);
      at_coord p2 = O_COORDINATE(*outline, (k + 2) % n);
      at_coord before = step(pm2, pm1), after = step(p0, p1), diag = step(pm1, p0);

      if (is_straight_step(before) && same_step(before, step(pm3, pm2)) &&
          is_straight_step(after) && same_step(after, step(p1, p2)) &&
          (before.x == 0) != (after.x == 0) && (unsigned short)(before.x + after.x) == diag.x &&
          (unsigned short)(before.y + after.y) == diag.y) {
        at_coord corner;

        corner.x = (unsigned short)(pm1.x + before.x);
        corner.y = (unsigned short)(pm1.y + before.y);
        g_array_append_val(out, corner);
      }
    }
    g_array_append_val(out, O_COORDINATE(*outline, k));
  }

  g_array_free(outline->data, TRUE);
  outline->data = out;
}
