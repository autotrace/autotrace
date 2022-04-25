/* pxl-outline.c: find the outlines of a bitmap image; each outline is made up of one or more pixels;
   and each pixel participates via one or more edges. */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* Def: HAVE_CONFIG_H */

#include "logreport.h"
#include "types.h"
#include "bitmap.h"
#include "color.h"
#include "bitmap.h"
#include "xstd.h"
#include "pxl-outline.h"
#include <assert.h>

/* We consider each pixel to consist of four edges, and we travel along
   edges, instead of through pixel centers.  This is necessary for those
   unfortunate times when a single pixel is on both an inside and an
   outside outline.

   The numbers chosen here are not arbitrary; the code that figures out
   which edge to move to depends on particular values.  See the
   `TRY_PIXEL' macro in `edge.c'.  To emphasize this, I've written in the
   numbers we need for each edge value.  */

typedef enum {
  TOP = 1, LEFT = 2, BOTTOM = 3, RIGHT = 0, NO_EDGE = 4
} edge_type;

/* This choice is also not arbitrary: starting at the top edge makes the
   code find outside outlines before inside ones, which is certainly
   what we want.  */
#define START_EDGE  top

typedef enum {
  NORTH = 0, NORTHWEST = 1, WEST = 2, SOUTHWEST = 3, SOUTH = 4,
  SOUTHEAST = 5, EAST = 6, NORTHEAST = 7
} direction_type;

#define NUM_EDGES NO_EDGE

#define COMPUTE_DELTA(axis, dir)                        \
  ((dir) % 2 != 0                                       \
    ? COMPUTE_##axis##_DELTA ((dir) - 1)                \
      + COMPUTE_##axis##_DELTA (((dir) + 1) % 8)        \
    : COMPUTE_##axis##_DELTA (dir)                      \
  )

#define COMPUTE_ROW_DELTA(dir)                          \
  ((dir) == NORTH ? -1 : (dir) == SOUTH ? +1 : 0)

#define COMPUTE_COL_DELTA(dir)                  \
  ((dir) == WEST ? -1 : (dir) == EAST ? +1 : 0)

static pixel_outline_type find_one_outline(at_bitmap *, edge_type, unsigned short, unsigned short, at_bitmap *, gboolean, gboolean, at_exception_type *);
static pixel_outline_type find_one_centerline(at_bitmap *, direction_type, unsigned short, unsigned short, at_bitmap *);
static void append_pixel_outline(pixel_outline_list_type *, pixel_outline_type);
static pixel_outline_type new_pixel_outline(void);
static void free_pixel_outline(pixel_outline_type *);
static void concat_pixel_outline(pixel_outline_type *, const pixel_outline_type *);
static void append_outline_pixel(pixel_outline_type *, at_coord);
static gboolean is_marked_edge(edge_type, unsigned short, unsigned short, at_bitmap *);
static gboolean is_outline_edge(edge_type, at_bitmap *, unsigned short, unsigned short, at_color, at_exception_type *);
static gboolean is_unmarked_outline_edge(unsigned short, unsigned short, edge_type, at_bitmap *, at_bitmap *, at_color, at_exception_type *);

static void mark_edge(edge_type e, unsigned short, unsigned short, at_bitmap *);
/* static edge_type opposite_edge(edge_type); */

static gboolean is_marked_dir(unsigned short, unsigned short, direction_type, at_bitmap *);
static gboolean is_other_dir_marked(unsigned short, unsigned short, direction_type, at_bitmap *);
static void mark_dir(unsigned short, unsigned short, direction_type, at_bitmap *);
static gboolean next_unmarked_pixel(unsigned short *, unsigned short *, direction_type *, at_bitmap *, at_bitmap *);

gboolean is_valid_dir(unsigned short, unsigned short, direction_type, at_bitmap *, at_bitmap *);

static at_coord next_point(at_bitmap *, edge_type *, unsigned short *, unsigned short *, at_color, gboolean, at_bitmap *, at_exception_type *);
static unsigned num_neighbors(unsigned short, unsigned short, at_bitmap *);

#define CHECK_FATAL() if (at_exception_got_fatal(exp)) goto cleanup;

/* We go through a bitmap TOP to BOTTOM, LEFT to RIGHT, looking for each pixel with an unmarked edge
   that we consider a starting point of an outline. */

pixel_outline_list_type find_outline_pixels(at_bitmap * bitmap, at_color * bg_color, at_progress_func notify_progress, gpointer progress_data, at_testcancel_func test_cancel, gpointer testcancel_data, at_exception_type * exp)
{
  pixel_outline_list_type outline_list;
  unsigned short row, col;
  at_bitmap *marked = at_bitmap_new(AT_BITMAP_WIDTH(bitmap), AT_BITMAP_HEIGHT(bitmap), 1);
  unsigned int max_progress = AT_BITMAP_HEIGHT(bitmap) * AT_BITMAP_WIDTH(bitmap);

  O_LIST_LENGTH(outline_list) = 0;
  outline_list.data = NULL;

  for (row = 0; row < AT_BITMAP_HEIGHT(bitmap); row++) {
    for (col = 0; col < AT_BITMAP_WIDTH(bitmap); col++) {
      edge_type edge;
      at_color color;
      gboolean is_background;

      if (notify_progress)
        notify_progress((gfloat) (row * AT_BITMAP_WIDTH(bitmap) + col) / ((gfloat) max_progress * (gfloat) 3.0), progress_data);

      /* A valid edge can be TOP for an outside outline.
         Outside outlines are traced counterclockwise */
      at_bitmap_get_color(bitmap, row, col, &color);
      if (!(is_background = (gboolean) (bg_color && at_color_equal(&color, bg_color)))
          && is_unmarked_outline_edge(row, col, edge = TOP, bitmap, marked, color, exp)) {
        pixel_outline_type outline;

        CHECK_FATAL();          /* FREE(DONE) outline_list */

        LOG("#%u: (counterclockwise)", O_LIST_LENGTH(outline_list));

        outline = find_one_outline(bitmap, edge, row, col, marked, FALSE, FALSE, exp);
        CHECK_FATAL();          /* FREE(DONE) outline_list */

        O_CLOCKWISE(outline) = FALSE;
        append_pixel_outline(&outline_list, outline);

        LOG(" [%u].\n", O_LENGTH(outline));
      } else
        CHECK_FATAL();          /* FREE(DONE) outline_list */

      /* A valid edge can be BOTTOM for an inside outline.
         Inside outlines are traced clockwise */
      if (row != 0) {
        at_bitmap_get_color(bitmap, row - 1, col, &color);
        if (!(bg_color && at_color_equal(&color, bg_color))
            && is_unmarked_outline_edge(row - 1, col, edge = BOTTOM, bitmap, marked, color, exp)) {
          pixel_outline_type outline;

          CHECK_FATAL();        /* FREE(DONE) outline_list */

          /* This lines are for debugging only: */
          if (is_background) {
            LOG("#%u: (clockwise)", O_LIST_LENGTH(outline_list));

            outline = find_one_outline(bitmap, edge, row - 1, col, marked, TRUE, FALSE, exp);
            CHECK_FATAL();      /* FREE(DONE) outline_list */

            O_CLOCKWISE(outline) = TRUE;
            append_pixel_outline(&outline_list, outline);

            LOG(" [%u].\n", O_LENGTH(outline));
          } else {
            outline = find_one_outline(bitmap, edge, row - 1, col, marked, TRUE, TRUE, exp);
            CHECK_FATAL();      /* FREE(DONE) outline_list */
          }
        } else
          CHECK_FATAL();        /* FREE(DONE) outline_list */
      }
      if (test_cancel && test_cancel(testcancel_data)) {
        free_pixel_outline_list(&outline_list);
        goto cleanup;
      }
    }
  }
cleanup:
  at_bitmap_free(marked);
  if (at_exception_got_fatal(exp))
    free_pixel_outline_list(&outline_list);
  return outline_list;
}

/* We calculate one single outline here. We pass the position of the starting pixel and the
   starting edge. All edges we track along will be marked and the outline pixels are appended
   to the coordinate list. */

static pixel_outline_type find_one_outline(at_bitmap * bitmap, edge_type original_edge, unsigned short original_row, unsigned short original_col, at_bitmap * marked, gboolean clockwise, gboolean ignore, at_exception_type * exp)
{
  pixel_outline_type outline;
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
      LOG(" (%d,%d)", pos.x, pos.y);
      append_outline_pixel(&outline, pos);
    }

    mark_edge(edge, row, col, marked);
    pos = next_point(bitmap, &edge, &row, &col, outline.color, clockwise, marked, exp);
    CHECK_FATAL();
  }
  while (edge != NO_EDGE);

cleanup:
  if (at_exception_got_fatal(exp))
    free_pixel_outline(&outline);
  return outline;
}

gboolean is_valid_dir(unsigned short row, unsigned short col, direction_type dir, at_bitmap * bitmap, at_bitmap * marked)
{

  at_color c;
  int new_row = COMPUTE_DELTA(ROW, dir) + row;
  int new_col = COMPUTE_DELTA(COL, dir) + col;

  if ((new_row < 0) || (new_col < 0) || (new_row >= AT_BITMAP_HEIGHT(bitmap)) || (new_col >= AT_BITMAP_WIDTH(bitmap)))
	return FALSE;	// Must not call at_bitmap_get_color() with negative row or col.

  at_bitmap_get_color(bitmap, COMPUTE_DELTA(ROW, dir) + row, COMPUTE_DELTA(COL, dir) + col, &c);
  return ((gboolean) (!is_marked_dir(row, col, dir, marked)
                      && COMPUTE_DELTA(ROW, dir) + row > 0 && COMPUTE_DELTA(COL, dir) + col > 0 && AT_BITMAP_VALID_PIXEL(bitmap, COMPUTE_DELTA(ROW, dir) + row, COMPUTE_DELTA(COL, dir) + col)
                      && at_bitmap_equal_color(bitmap, row, col, &c)));
}

pixel_outline_list_type find_centerline_pixels(at_bitmap * bitmap, at_color bg_color, at_progress_func notify_progress, gpointer progress_data, at_testcancel_func test_cancel, gpointer testcancel_data, at_exception_type * exp)
{
  pixel_outline_list_type outline_list;
  signed short row, col;
  at_bitmap *marked = at_bitmap_new(AT_BITMAP_WIDTH(bitmap), AT_BITMAP_HEIGHT(bitmap), 1);
  unsigned int max_progress = AT_BITMAP_HEIGHT(bitmap) * AT_BITMAP_WIDTH(bitmap);

  O_LIST_LENGTH(outline_list) = 0;
  outline_list.data = NULL;

  for (row = 0; row < AT_BITMAP_HEIGHT(bitmap); row++) {
    for (col = 0; col < AT_BITMAP_WIDTH(bitmap);) {
      direction_type dir = EAST;
      pixel_outline_type outline;
      gboolean clockwise = FALSE;

      if (notify_progress)
        notify_progress((gfloat) (row * AT_BITMAP_WIDTH(bitmap) + col) / ((gfloat) max_progress * (gfloat) 3.0), progress_data);

      if (at_bitmap_equal_color(bitmap, row, col, &bg_color)) {
        col++;
        continue;
      }

      if (!is_valid_dir(row, col, dir, bitmap, marked)
          || (!is_valid_dir(COMPUTE_DELTA(ROW, dir) + row, COMPUTE_DELTA(COL, dir) + col, dir, bitmap, marked)
              && num_neighbors(row, col, bitmap) > 2)
          || num_neighbors(row, col, bitmap) > 4 || num_neighbors(COMPUTE_DELTA(ROW, dir) + row, COMPUTE_DELTA(COL, dir) + col, bitmap) > 4 || (is_other_dir_marked(row, col, dir, marked)
                                                                                                                                                && is_other_dir_marked(row + COMPUTE_DELTA(ROW, dir), col + COMPUTE_DELTA(COL, dir), dir, marked))) {
        dir = SOUTHEAST;
        if (!is_valid_dir(row, col, dir, bitmap, marked)
            || (!is_valid_dir(COMPUTE_DELTA(ROW, dir) + row, COMPUTE_DELTA(COL, dir) + col, dir, bitmap, marked)
                && num_neighbors(row, col, bitmap) > 2)
            || num_neighbors(row, col, bitmap) > 4 || num_neighbors(COMPUTE_DELTA(ROW, dir) + row, COMPUTE_DELTA(COL, dir) + col, bitmap) > 4 || (is_other_dir_marked(row, col, dir, marked)
                                                                                                                                                  && is_other_dir_marked(row + COMPUTE_DELTA(ROW, dir), col + COMPUTE_DELTA(COL, dir), dir, marked))) {
          dir = SOUTH;
          if (!is_valid_dir(row, col, dir, bitmap, marked)
              || (!is_valid_dir(COMPUTE_DELTA(ROW, dir) + row, COMPUTE_DELTA(COL, dir) + col, dir, bitmap, marked)
                  && num_neighbors(row, col, bitmap) > 2)
              || num_neighbors(row, col, bitmap) > 4 || num_neighbors(COMPUTE_DELTA(ROW, dir) + row, COMPUTE_DELTA(COL, dir) + col, bitmap) > 4 || (is_other_dir_marked(row, col, dir, marked)
                                                                                                                                                    && is_other_dir_marked(row + COMPUTE_DELTA(ROW, dir), col + COMPUTE_DELTA(COL, dir), dir, marked))) {
            dir = SOUTHWEST;
            if (!is_valid_dir(row, col, dir, bitmap, marked)
                || (!is_valid_dir(COMPUTE_DELTA(ROW, dir) + row, COMPUTE_DELTA(COL, dir) + col, dir, bitmap, marked)
                    && num_neighbors(row, col, bitmap) > 2)
                || num_neighbors(row, col, bitmap) > 4 || num_neighbors(COMPUTE_DELTA(ROW, dir) + row, COMPUTE_DELTA(COL, dir) + col, bitmap) > 4 || (is_other_dir_marked(row, col, dir, marked)
                                                                                                                                                      && is_other_dir_marked(row + COMPUTE_DELTA(ROW, dir), col + COMPUTE_DELTA(COL, dir), dir, marked))) {
              col++;
              continue;
            }
          }
        }
      }

      LOG("#%u: (%sclockwise) ", O_LIST_LENGTH(outline_list), clockwise ? "" : "counter");

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
          if (partial_outline.data)
            free(partial_outline.data);
        } else
          col++;
      }

      /* Outside outlines will start at a top edge, and move
         counterclockwise, and inside outlines will start at a
         bottom edge, and move clockwise.  This happens because of
         the order in which we look at the edges. */
      O_CLOCKWISE(outline) = clockwise;
      if (O_LENGTH(outline) > 1)
        append_pixel_outline(&outline_list, outline);
      LOG("(%s)", (outline.open ? " open" : " closed"));
      LOG(" [%u].\n", O_LENGTH(outline));
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

static pixel_outline_type find_one_centerline(at_bitmap * bitmap, direction_type search_dir, unsigned short original_row, unsigned short original_col, at_bitmap * marked)
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
  LOG(" (%d,%d)", pos.x, pos.y);
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
    mark_dir(row, col, (direction_type) ((search_dir + 4) % 8), marked);

    /* If we've returned to the starting pixel, we're done. */
    if (row == original_row && col == original_col)
      break;

    /* Add the new pixel to the output list. */
    pos.x = col;
    pos.y = AT_BITMAP_HEIGHT(bitmap) - row - 1;
    LOG(" (%d,%d)", pos.x, pos.y);
    append_outline_pixel(&outline, pos);
  }
  mark_dir(original_row, original_col, original_dir, marked);
  return outline;
}

/* Add an outline to an outline list. */

static void append_pixel_outline(pixel_outline_list_type * outline_list, pixel_outline_type outline)
{
  O_LIST_LENGTH(*outline_list)++;
  XREALLOC(outline_list->data, outline_list->length * sizeof(pixel_outline_type));
  O_LIST_OUTLINE(*outline_list, O_LIST_LENGTH(*outline_list) - 1) = outline;
}

/* Free the list of outline lists. */

void free_pixel_outline_list(pixel_outline_list_type * outline_list)
{
  unsigned this_outline;

  for (this_outline = 0; this_outline < outline_list->length; this_outline++) {
    pixel_outline_type o = outline_list->data[this_outline];
    free_pixel_outline(&o);
  }
  free(outline_list->data);
  outline_list->data = NULL;
  outline_list->length = 0;
}

/* Return an empty list of pixels.  */

static pixel_outline_type new_pixel_outline(void)
{
  pixel_outline_type pixel_outline;

  O_LENGTH(pixel_outline) = 0;
  pixel_outline.data = NULL;
  pixel_outline.open = FALSE;

  return pixel_outline;
}

static void free_pixel_outline(pixel_outline_type * outline)
{
  free(outline->data);
  outline->data = NULL;
  outline->length = 0;
}

/* Concatenate two pixel lists. The two lists are assumed to have the
   same starting pixel and to proceed in opposite directions therefrom. */

static void concat_pixel_outline(pixel_outline_type * o1, const pixel_outline_type * o2)
{
  int src, dst;
  unsigned o1_length, o2_length;
  if (!o1 || !o2 || O_LENGTH(*o2) <= 1)
    return;

  o1_length = O_LENGTH(*o1);
  o2_length = O_LENGTH(*o2);
  O_LENGTH(*o1) += o2_length - 1;
  /* Resize o1 to the sum of the lengths of o1 and o2 minus one (because
     the two lists are assumed to share the same starting pixel). */
  XREALLOC(o1->data, O_LENGTH(*o1) * sizeof(at_coord));
  /* Shift the contents of o1 to the end of the new array to make room
     to prepend o2. */
  for (src = o1_length - 1, dst = O_LENGTH(*o1) - 1; src >= 0; src--, dst--)
    O_COORDINATE(*o1, dst) = O_COORDINATE(*o1, src);
  /* Prepend the contents of o2 (in reverse order) to o1. */
  for (src = o2_length - 1, dst = 0; src > 0; src--, dst++)
    O_COORDINATE(*o1, dst) = O_COORDINATE(*o2, src);
}

/* Add a point to the pixel list. */

static void append_outline_pixel(pixel_outline_type * o, at_coord c)
{
  O_LENGTH(*o)++;
  XREALLOC(o->data, O_LENGTH(*o) * sizeof(at_coord));
  O_COORDINATE(*o, O_LENGTH(*o) - 1) = c;
}

/* Is this really an edge and is it still unmarked? */

static gboolean is_unmarked_outline_edge(unsigned short row, unsigned short col, edge_type edge, at_bitmap * bitmap, at_bitmap * marked, at_color color, at_exception_type * exp)
{
  return (gboolean) (!is_marked_edge(edge, row, col, marked)
                     && is_outline_edge(edge, bitmap, row, col, color, exp));
}

/* We check to see if the edge of the pixel at position ROW and COL
   is an outline edge */

static gboolean is_outline_edge(edge_type edge, at_bitmap * bitmap, unsigned short row, unsigned short col, at_color color, at_exception_type * exp)
{
  /* If this pixel isn't of the same color, it's not part of the outline. */
  if (!at_bitmap_equal_color(bitmap, row, col, &color))
    return FALSE;

  switch (edge) {
  case LEFT:
    return (gboolean) (col == 0 || !at_bitmap_equal_color(bitmap, row, col - 1, &color));
  case TOP:
    return (gboolean) (row == 0 || !at_bitmap_equal_color(bitmap, row - 1, col, &color));

  case RIGHT:
    return (gboolean) (col == AT_BITMAP_WIDTH(bitmap) - 1 || !at_bitmap_equal_color(bitmap, row, col + 1, &color));

  case BOTTOM:
    return (gboolean) (row == AT_BITMAP_HEIGHT(bitmap) - 1 || !at_bitmap_equal_color(bitmap, row + 1, col, &color));

  case NO_EDGE:
    g_assert_not_reached();
  default:
    g_assert_not_reached();
  }
  return FALSE;                 /* NOT REACHED */
}

/* If EDGE is not already marked, we mark it; otherwise, it's a fatal error.
   The position ROW and COL should be inside the bitmap MARKED. EDGE can be
   NO_EDGE. */

static void mark_edge(edge_type edge, unsigned short row, unsigned short col, at_bitmap * marked)
{
  *AT_BITMAP_PIXEL(marked, row, col) |= 1 << edge;
}

/* Mark the direction of the pixel ROW/COL in MARKED. */

static void mark_dir(unsigned short row, unsigned short col, direction_type dir, at_bitmap * marked)
{
  *AT_BITMAP_PIXEL(marked, row, col) |= 1 << dir;
}

/* Test if the direction of pixel at ROW/COL in MARKED is marked. */

static gboolean is_marked_dir(unsigned short row, unsigned short col, direction_type dir, at_bitmap * marked)
{
  return (gboolean) ((*AT_BITMAP_PIXEL(marked, row, col) & 1 << dir) != 0);
}

static gboolean is_other_dir_marked(unsigned short row, unsigned short col, direction_type dir, at_bitmap * marked)
{
  return (gboolean) ((*AT_BITMAP_PIXEL(marked, row, col) & (255 - (1 << dir) - (1 << ((dir + 4) % 8)))) != 0);
}

static gboolean next_unmarked_pixel(unsigned short *row, unsigned short *col, direction_type * dir, at_bitmap * bitmap, at_bitmap * marked)
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
      test_dir = (direction_type) ((orig_dir + 2) % 8);
    else if ((orig_dir + 2) % 8 == test_dir)
      test_dir = (direction_type) ((orig_dir + 6) % 8);
    else if ((orig_dir + 6) % 8 == test_dir)
      test_dir = (direction_type) ((orig_dir + 1) % 8);
    else if ((orig_dir + 1) % 8 == test_dir)
      test_dir = (direction_type) ((orig_dir + 7) % 8);
    else if ((orig_dir + 7) % 8 == test_dir)
      test_dir = (direction_type) ((orig_dir + 3) % 8);
    else if ((orig_dir + 3) % 8 == test_dir)
      test_dir = (direction_type) ((orig_dir + 5) % 8);
    else if ((orig_dir + 5) % 8 == test_dir)
      break;
  }
  while (1);
  if ((*row != orig_row || *col != orig_col) && (!(is_other_dir_marked(orig_row, orig_col, test_dir, marked)
                                                   && is_other_dir_marked(orig_row + COMPUTE_DELTA(ROW, test_dir), orig_col + COMPUTE_DELTA(COL, test_dir), test_dir, marked))))
    return TRUE;
  else
    return FALSE;
}

/* Return the number of pixels adjacent to pixel ROW/COL that are black. */

static unsigned num_neighbors(unsigned short row, unsigned short col, at_bitmap * bitmap)
{
  unsigned dir, count = 0;
  at_color color;

  at_bitmap_get_color(bitmap, row, col, &color);
  for (dir = NORTH; dir <= NORTHEAST; dir++) {
    int delta_r = COMPUTE_DELTA(ROW, dir);
    int delta_c = COMPUTE_DELTA(COL, dir);
    unsigned int test_row = row + delta_r;
    unsigned int test_col = col + delta_c;
    if (AT_BITMAP_VALID_PIXEL(bitmap, test_row, test_col)
        && at_bitmap_equal_color(bitmap, test_row, test_col, &color))
      ++count;
  }
  return count;
}

/* Test if the edge EDGE at ROW/COL in MARKED is marked.  */

static gboolean is_marked_edge(edge_type edge, unsigned short row, unsigned short col, at_bitmap * marked)
{
  return (gboolean) (edge == NO_EDGE ? FALSE : (*AT_BITMAP_PIXEL(marked, row, col) & (1 << edge)) != 0);
}

static at_coord next_point(at_bitmap * bitmap, edge_type * edge, unsigned short *row, unsigned short *col, at_color color, gboolean clockwise, at_bitmap * marked, at_exception_type * exp)
{
  at_coord pos = { 0, 0 };

  if (!clockwise)
    switch (*edge) {
    case TOP:
      /* WEST */
      if ((*col >= 1 && !is_marked_edge(TOP, *row, *col - 1, marked)
           && is_outline_edge(TOP, bitmap, *row, *col - 1, color, exp))) {
            /**edge = TOP;*/
        (*col)--;
        pos.x = *col;
        pos.y = AT_BITMAP_HEIGHT(bitmap) - *row;
        break;
      }
      CHECK_FATAL();
      /* NORTHWEST */
      if ((*col >= 1 && *row >= 1 && !is_marked_edge(RIGHT, *row - 1, *col - 1, marked)
           && is_outline_edge(RIGHT, bitmap, *row - 1, *col - 1, color, exp)) && !(is_marked_edge(LEFT, *row - 1, *col, marked) && is_marked_edge(TOP, *row, *col - 1, marked)) && !(is_marked_edge(BOTTOM, *row - 1, *col, marked) && is_marked_edge(RIGHT, *row, *col - 1, marked))) {
        *edge = RIGHT;
        (*col)--;
        (*row)--;
        pos.x = *col + 1;
        pos.y = AT_BITMAP_HEIGHT(bitmap) - *row;
        break;
      }
      CHECK_FATAL();
      if ((!is_marked_edge(LEFT, *row, *col, marked)
           && is_outline_edge(LEFT, bitmap, *row, *col, color, exp))) {
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
      if ((*row >= 1 && !is_marked_edge(RIGHT, *row - 1, *col, marked)
           && is_outline_edge(RIGHT, bitmap, *row - 1, *col, color, exp))) {
            /**edge = RIGHT;*/
        (*row)--;
        pos.x = *col + 1;
        pos.y = AT_BITMAP_HEIGHT(bitmap) - *row;
        break;
      }
      CHECK_FATAL();
      /* NORTHEAST */
      if ((*col + 1 < AT_BITMAP_WIDTH(marked) && *row >= 1 && !is_marked_edge(BOTTOM, *row - 1, *col + 1, marked)
           && is_outline_edge(BOTTOM, bitmap, *row - 1, *col + 1, color, exp)) && !(is_marked_edge(LEFT, *row, *col + 1, marked) && is_marked_edge(BOTTOM, *row - 1, *col, marked)) && !(is_marked_edge(TOP, *row, *col + 1, marked) && is_marked_edge(RIGHT, *row - 1, *col, marked))) {
        *edge = BOTTOM;
        (*col)++;
        (*row)--;
        pos.x = *col + 1;
        pos.y = AT_BITMAP_HEIGHT(bitmap) - *row - 1;
        break;
      }
      CHECK_FATAL();
      if ((!is_marked_edge(TOP, *row, *col, marked)
           && is_outline_edge(TOP, bitmap, *row, *col, color, exp))) {
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
      if ((*col + 1 < AT_BITMAP_WIDTH(marked)
           && !is_marked_edge(BOTTOM, *row, *col + 1, marked)
           && is_outline_edge(BOTTOM, bitmap, *row, *col + 1, color, exp))) {
            /**edge = BOTTOM;*/
        (*col)++;
        pos.x = *col + 1;
        pos.y = AT_BITMAP_HEIGHT(bitmap) - *row - 1;
        break;
      }
      CHECK_FATAL();
      /* SOUTHEAST */
      if ((*col + 1 < AT_BITMAP_WIDTH(marked) && *row + 1 < AT_BITMAP_HEIGHT(marked)
           && !is_marked_edge(LEFT, *row + 1, *col + 1, marked)
           && is_outline_edge(LEFT, bitmap, *row + 1, *col + 1, color, exp)) && !(is_marked_edge(TOP, *row + 1, *col, marked) && is_marked_edge(LEFT, *row, *col + 1, marked)) && !(is_marked_edge(RIGHT, *row + 1, *col, marked) && is_marked_edge(BOTTOM, *row, *col + 1, marked))) {
        *edge = LEFT;
        (*col)++;
        (*row)++;
        pos.x = *col;
        pos.y = AT_BITMAP_HEIGHT(bitmap) - *row - 1;
        break;
      }
      CHECK_FATAL();
      if ((!is_marked_edge(RIGHT, *row, *col, marked)
           && is_outline_edge(RIGHT, bitmap, *row, *col, color, exp))) {
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
      if ((*row + 1 < AT_BITMAP_HEIGHT(marked)
           && !is_marked_edge(LEFT, *row + 1, *col, marked)
           && is_outline_edge(LEFT, bitmap, *row + 1, *col, color, exp))) {
            /**edge = LEFT;*/
        (*row)++;
        pos.x = *col;
        pos.y = AT_BITMAP_HEIGHT(bitmap) - *row - 1;
        break;
      }
      CHECK_FATAL();
      /* SOUTHWEST */
      if ((*col >= 1 && *row + 1 < AT_BITMAP_HEIGHT(marked)
           && !is_marked_edge(TOP, *row + 1, *col - 1, marked)
           && is_outline_edge(TOP, bitmap, *row + 1, *col - 1, color, exp)) && !(is_marked_edge(RIGHT, *row, *col - 1, marked) && is_marked_edge(TOP, *row + 1, *col, marked)) && !(is_marked_edge(BOTTOM, *row, *col - 1, marked) && is_marked_edge(LEFT, *row + 1, *col, marked))) {
        *edge = TOP;
        (*col)--;
        (*row)++;
        pos.x = *col;
        pos.y = AT_BITMAP_HEIGHT(bitmap) - *row;
        break;
      }
      CHECK_FATAL();
      if ((!is_marked_edge(BOTTOM, *row, *col, marked)
           && is_outline_edge(BOTTOM, bitmap, *row, *col, color, exp))) {
        *edge = BOTTOM;
        pos.x = *col + 1;
        pos.y = AT_BITMAP_HEIGHT(bitmap) - *row - 1;
        break;
      }
      CHECK_FATAL();
    case NO_EDGE:
    default:
      *edge = NO_EDGE;
      break;
  } else
    switch (*edge) {
    case TOP:
      if ((!is_marked_edge(LEFT, *row, *col, marked)
           && is_outline_edge(LEFT, bitmap, *row, *col, color, exp))) {
        *edge = LEFT;
        pos.x = *col;
        pos.y = AT_BITMAP_HEIGHT(bitmap) - *row - 1;
        break;
      }
      CHECK_FATAL();
      /* WEST */
      if ((*col >= 1 && !is_marked_edge(TOP, *row, *col - 1, marked)
           && is_outline_edge(TOP, bitmap, *row, *col - 1, color, exp))) {
            /**edge = TOP;*/
        (*col)--;
        pos.x = *col;
        pos.y = AT_BITMAP_HEIGHT(bitmap) - *row;
        break;
      }
      CHECK_FATAL();
      /* NORTHWEST */
      if ((*col >= 1 && *row >= 1 && !is_marked_edge(RIGHT, *row - 1, *col - 1, marked)
           && is_outline_edge(RIGHT, bitmap, *row - 1, *col - 1, color, exp))) {
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
      if ((!is_marked_edge(TOP, *row, *col, marked)
           && is_outline_edge(TOP, bitmap, *row, *col, color, exp))) {
        *edge = TOP;
        pos.x = *col;
        pos.y = AT_BITMAP_HEIGHT(bitmap) - *row;
        break;
      }
      CHECK_FATAL();
      /* NORTH */
      if ((*row >= 1 && !is_marked_edge(RIGHT, *row - 1, *col, marked)
           && is_outline_edge(RIGHT, bitmap, *row - 1, *col, color, exp))) {
            /**edge = RIGHT;*/
        (*row)--;
        pos.x = *col + 1;
        pos.y = AT_BITMAP_HEIGHT(bitmap) - *row;
        break;
      }
      CHECK_FATAL();
      /* NORTHEAST */
      if ((*col + 1 < AT_BITMAP_WIDTH(marked) && *row >= 1 && !is_marked_edge(BOTTOM, *row - 1, *col + 1, marked)
           && is_outline_edge(BOTTOM, bitmap, *row - 1, *col + 1, color, exp))) {
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
      if ((!is_marked_edge(RIGHT, *row, *col, marked)
           && is_outline_edge(RIGHT, bitmap, *row, *col, color, exp))) {
        *edge = RIGHT;
        pos.x = *col + 1;
        pos.y = AT_BITMAP_HEIGHT(bitmap) - *row;
        break;
      }
      CHECK_FATAL();
      /* EAST */
      if ((*col + 1 < AT_BITMAP_WIDTH(marked)
           && !is_marked_edge(BOTTOM, *row, *col + 1, marked)
           && is_outline_edge(BOTTOM, bitmap, *row, *col + 1, color, exp))) {
            /**edge = BOTTOM;*/
        (*col)++;
        pos.x = *col + 1;
        pos.y = AT_BITMAP_HEIGHT(bitmap) - *row - 1;
        break;
      }
      CHECK_FATAL();
      /* SOUTHEAST */
      if ((*col + 1 < AT_BITMAP_WIDTH(marked) && *row + 1 < AT_BITMAP_HEIGHT(marked)
           && !is_marked_edge(LEFT, *row + 1, *col + 1, marked)
           && is_outline_edge(LEFT, bitmap, *row + 1, *col + 1, color, exp))) {
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
      if ((!is_marked_edge(BOTTOM, *row, *col, marked)
           && is_outline_edge(BOTTOM, bitmap, *row, *col, color, exp))) {
        *edge = BOTTOM;
        pos.x = *col + 1;
        pos.y = AT_BITMAP_HEIGHT(bitmap) - *row - 1;
        break;
      }
      CHECK_FATAL();
      /* SOUTH */
      if ((*row + 1 < AT_BITMAP_HEIGHT(marked)
           && !is_marked_edge(LEFT, *row + 1, *col, marked)
           && is_outline_edge(LEFT, bitmap, *row + 1, *col, color, exp))) {
            /**edge = LEFT;*/
        (*row)++;
        pos.x = *col;
        pos.y = AT_BITMAP_HEIGHT(bitmap) - *row - 1;
        break;
      }
      CHECK_FATAL();
      /* SOUTHWEST */
      if ((*col >= 1 && *row + 1 < AT_BITMAP_HEIGHT(marked)
           && !is_marked_edge(TOP, *row + 1, *col - 1, marked)
           && is_outline_edge(TOP, bitmap, *row + 1, *col - 1, color, exp))) {
        *edge = TOP;
        (*col)--;
        (*row)++;
        pos.x = *col;
        pos.y = AT_BITMAP_HEIGHT(bitmap) - *row;
        break;
      }
      CHECK_FATAL();
    case NO_EDGE:
    default:
      *edge = NO_EDGE;
      break;
    }
cleanup:
  return (pos);
}
