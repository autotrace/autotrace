/* pxl-outline.c: find the outlines of a bitmap image; each outline is made up of one or more pixels;
   and each pixel participates via one or more edges. */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* Def: HAVE_CONFIG_H */

#include "message.h"
#include "types.h"
#include "bitmap.h"
#include "color.h"
#include "bitmap.h"
#include "logreport.h"
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

typedef enum
{
  TOP = 1, LEFT = 2, BOTTOM = 3, RIGHT = 0, NO_EDGE = 4
} edge_type;

/* This choice is also not arbitrary: starting at the top edge makes the
   code find outside outlines before inside ones, which is certainly
   what we want.  */
#define START_EDGE  top

typedef enum
{
  NORTH = 0, NORTHWEST = 1, WEST = 2, SOUTHWEST = 3, SOUTH = 4,
  SOUTHEAST = 5, EAST = 6, NORTHEAST = 7
} direction_type;

#define NUM_EDGES NO_EDGE

#define COMPUTE_DELTA(axis, dir)					\
  ((dir) % 2 != 0							\
    ? COMPUTE_##axis##_DELTA ((dir) - 1)				\
      + COMPUTE_##axis##_DELTA (((dir) + 1) % 8)			\
    : COMPUTE_##axis##_DELTA (dir)					\
  )

#define COMPUTE_ROW_DELTA(dir)						\
  ((dir) == NORTH ? -1 : (dir) == SOUTH ? +1 : 0)

#define COMPUTE_COL_DELTA(dir)						\
  ((dir) == WEST ? -1 : (dir) == EAST ? +1 : 0)

static pixel_outline_type find_one_outline (bitmap_type, edge_type, unsigned short,
  unsigned short, bitmap_type *, at_bool, at_bool, at_exception_type *);
static pixel_outline_type find_one_centerline (bitmap_type, edge_type,
 unsigned short, unsigned short, bitmap_type *);
static void append_pixel_outline (pixel_outline_list_type *,
  pixel_outline_type);
static pixel_outline_type new_pixel_outline (void);
static void concat_pixel_outline (pixel_outline_type *,
  const pixel_outline_type*);
static void append_outline_pixel (pixel_outline_type *, at_coord);
static at_bool is_marked_edge (edge_type, unsigned short, unsigned short, bitmap_type);
static at_bool is_outline_edge (edge_type, bitmap_type, unsigned short, unsigned short,
				color_type, at_exception_type * exp);
static edge_type next_edge (edge_type);
static at_bool is_unmarked_outline_edge (unsigned short, unsigned short, edge_type,
  bitmap_type, bitmap_type, color_type, at_exception_type * exp);
static edge_type next_unmarked_outline_edge (unsigned short, unsigned short,
  edge_type, bitmap_type, bitmap_type, at_exception_type * exp);

static void mark_edge (edge_type e, unsigned short, unsigned short, bitmap_type *);
static edge_type opposite_edge(edge_type);

static void mark_pixel(unsigned short, unsigned short, bitmap_type *);
static at_bool next_unmarked_outline_pixel(unsigned short *, unsigned short *,
  direction_type *, bitmap_type, bitmap_type *);
static at_bool is_open_junction(unsigned short, unsigned short,
  bitmap_type, bitmap_type);

static at_coord NextPoint(bitmap_type, edge_type *, unsigned short *, unsigned short *, 
  color_type, at_bool, bitmap_type, at_exception_type * );

#define CHECK_FATAL() if (at_exception_got_fatal(exp)) goto cleanup;

/* We go through a bitmap TOP to BOTTOM, LEFT to RIGHT, looking for each pixel with an unmarked edge
   that we consider a starting point of an outline. */

#ifdef _EXPORTING
__declspec(dllexport) pixel_outline_list_type
__stdcall find_outline_pixels (bitmap_type bitmap, color_type *bg_color,
			       at_progress_func notify_progress, address progress_data,
			       testcancel_func test_cancel, address testcancel_data,
			       at_exception_type * exp)
#else
     pixel_outline_list_type
find_outline_pixels (bitmap_type bitmap, color_type *bg_color,
		     at_progress_func notify_progress, at_address progress_data,
		     at_testcancel_func test_cancel, at_address testcancel_data,
		     at_exception_type * exp)
		     
#endif
{
  pixel_outline_list_type outline_list;
  unsigned short row, col;
  bitmap_type marked = new_bitmap (BITMAP_WIDTH (bitmap), BITMAP_HEIGHT (bitmap));
  unsigned int max_progress = BITMAP_HEIGHT (bitmap) * BITMAP_WIDTH (bitmap);

  O_LIST_LENGTH (outline_list) = 0;
  outline_list.data = NULL;

  for (row = 0; row < BITMAP_HEIGHT (bitmap); row++)
    {
      for (col = 0; col < BITMAP_WIDTH (bitmap); col++)
	{
	  edge_type edge;
	  color_type color;
	  at_bool is_background;

	  if (notify_progress)
	    notify_progress((at_real)(row * BITMAP_WIDTH(bitmap) + col) / ((at_real) max_progress * (at_real)3.0),
			    progress_data);

	  /* A valid edge can be TOP for an outside outline.
	     Outside outlines are traced counterclockwise */
	  color = GET_COLOR (bitmap, row, col);
	  if (!(is_background = (bg_color && COLOR_EQUAL(color, *bg_color)))
	      && is_unmarked_outline_edge (row, col, edge = TOP,
					   bitmap, marked, color, exp))
	    {
	      pixel_outline_type outline;

	      CHECK_FATAL ();

	      LOG1 ("#%u: (counterclockwise)", O_LIST_LENGTH (outline_list));

	      outline = find_one_outline (bitmap, edge, row, col, &marked, false, false, exp);
	      CHECK_FATAL();

	      O_CLOCKWISE (outline) = false;
	      append_pixel_outline (&outline_list, outline);

	      LOG1 (" [%u].\n", O_LENGTH (outline));
	    }
	  else
	    CHECK_FATAL ();
	  
	  /* A valid edge can be BOTTOM for an inside outline.
	     Inside outlines are traced clockwise */
	  if (row!=0)
          {
            color = GET_COLOR (bitmap, row-1, col);
            if (!(bg_color && COLOR_EQUAL(color, *bg_color))
	          && is_unmarked_outline_edge (row-1, col, edge = BOTTOM,
					       bitmap, marked, color, exp))
              {
                pixel_outline_type outline;

		CHECK_FATAL();

                /* This lines are for debugging only:*/
                if (is_background)
                  {
                    LOG1 ("#%u: (clockwise)", O_LIST_LENGTH (outline_list));

                    outline = find_one_outline (bitmap, edge, row-1, col,
						&marked, true, false, exp);
		    CHECK_FATAL();

		    O_CLOCKWISE (outline) = true;
                    append_pixel_outline (&outline_list, outline);

                    LOG1 (" [%u].\n", O_LENGTH (outline));
                  }
                else
		  {
		    outline = find_one_outline (bitmap, edge, row-1, col,
						&marked, true, true, exp);
		    CHECK_FATAL();
		  }
		CHECK_FATAL();
              }
          }
	  if (test_cancel && test_cancel(testcancel_data))
	    {
	      if (O_LIST_LENGTH (outline_list) != 0)
		free_pixel_outline_list(&outline_list);
	      goto cleanup;
	    }
	}
    }
 cleanup:
  free_bitmap (&marked);
  flush_log_output ();

  return outline_list;
}


/* We calculate one single outline here. We pass the position of the starting pixel and the
   starting edge. All edges we track along will be marked and the outline pixels are appended
   to the coordinate list. */

static pixel_outline_type
find_one_outline (bitmap_type bitmap, edge_type original_edge,
		  unsigned short original_row, unsigned short original_col,
		  bitmap_type *marked, at_bool clockwise, at_bool ignore,
		  at_exception_type * exp)
{
  pixel_outline_type outline;
  unsigned short row = original_row, col = original_col;
  edge_type edge = original_edge;
  at_coord pos;

  pos.x = col + ((edge == RIGHT) || (edge == BOTTOM) ? 1 : 0);
  pos.y = BITMAP_HEIGHT (bitmap) - row - 1 + ((edge == TOP) || (edge == RIGHT) ? 1 : 0);

  if (!ignore)
    outline = new_pixel_outline ();
  outline.color = GET_COLOR (bitmap, row, col);

  do
  {
    /* Put this edge into the output list */
    if (!ignore)
	{
      LOG2 (" (%d,%d)", pos.x, pos.y);
      append_outline_pixel (&outline, pos);
	}

    mark_edge (edge, row, col, marked);
    pos = NextPoint (bitmap, &edge, &row, &col, outline.color, clockwise, *marked, exp);
    CHECK_FATAL();
  }
  while (edge != NO_EDGE);

 cleanup:
  return outline;
}


#ifdef _EXPORTING
__declspec(dllexport) pixel_outline_list_type
__stdcall find_centerline_pixels (bitmap_type bitmap, color_type bg_color,
				  at_progress_func notify_progress, address progress_data,
				  at_testcancel_func test_cancel, address testcancel_data,
				  at_exception_type * exp)
#else
pixel_outline_list_type
find_centerline_pixels(bitmap_type bitmap, color_type bg_color,
		       at_progress_func notify_progress, at_address progress_data,
		       at_testcancel_func test_cancel, at_address testcancel_data,
		       at_exception_type * exp)
#endif
{
    pixel_outline_list_type outline_list;
    unsigned short row, col;
    bitmap_type marked = new_bitmap(BITMAP_WIDTH(bitmap), BITMAP_HEIGHT (bitmap));
    unsigned int max_progress = BITMAP_HEIGHT (bitmap) * BITMAP_WIDTH (bitmap);
    
    O_LIST_LENGTH(outline_list) = 0;
    outline_list.data = NULL;

    for (row = 0; row < BITMAP_HEIGHT(bitmap); row++)
    {
	for (col = 0; col < BITMAP_WIDTH(bitmap); col++)
	{
	    edge_type edge;

	    if (notify_progress)
	      notify_progress((at_real)(row * BITMAP_WIDTH(bitmap) + col) / ((at_real) max_progress * (at_real)3.0),
			      progress_data);

	    if (COLOR_EQUAL(GET_COLOR(bitmap, row, col), bg_color)) continue;

	    edge = next_unmarked_outline_edge(row, col, TOP, bitmap, marked, exp);
	    CHECK_FATAL ();

	    if (edge != NO_EDGE)
	    {
		pixel_outline_type outline;
		at_bool clockwise = (at_bool)(edge == BOTTOM);

		LOG2("#%u: (%sclockwise, ", O_LIST_LENGTH(outline_list),
		    clockwise ? "" : "counter");

		outline = find_one_centerline(bitmap, edge, row, col, &marked);

		/* If the outline is open (i.e., we didn't return to the
		   starting pixel), search from the starting pixel in the
		   opposite direction and concatenate the two outlines. */
		if (outline.open)
		{
		    pixel_outline_type partial_outline;
		    edge_type opp_edge = opposite_edge(edge);
		    edge = next_unmarked_outline_edge(row, col, opp_edge,
						      bitmap, marked, exp);
		    CHECK_FATAL ();

		    if (edge == opp_edge)
		    {
			partial_outline =
			  find_one_centerline(bitmap, edge, row, col, &marked);
			concat_pixel_outline(&outline, &partial_outline);
		    }
		}

		/* Outside outlines will start at a top edge, and move
		   counterclockwise, and inside outlines will start at a
		   bottom edge, and move clockwise.  This happens because of
		   the order in which we look at the edges. */
		O_CLOCKWISE(outline) = clockwise;
/* if (clockwise) */
if (O_LENGTH(outline) > 1)
		    append_pixel_outline(&outline_list, outline);

		LOG1("%s)", (outline.open ? "open" : "closed"));
		LOG1(" [%u].\n", O_LENGTH(outline));
	    }
	}
	if (test_cancel && test_cancel(testcancel_data))
	  {
	    goto cleanup;
	  }
    }
 cleanup:
    free_bitmap(&marked);
    flush_log_output();
    return outline_list;
}


static pixel_outline_type
find_one_centerline(bitmap_type bitmap, edge_type original_edge,
		    unsigned short original_row, unsigned short original_col, bitmap_type *marked)
{
    pixel_outline_type outline = new_pixel_outline();
    unsigned short row = original_row, col = original_col;
    unsigned short prev_row, prev_col;
    direction_type search_dir = EAST;
    at_coord pos;

    outline.open = false;
    outline.color = GET_COLOR(bitmap, row, col);

    /* Add the starting pixel to the output list, changing from bitmap
       to Cartesian coordinates and specifying the left edge so that
       the coordinates won't be adjusted.
       TODO: On output, should add 0.5 to both coordinates. */
    pos.x = col; pos.y = BITMAP_HEIGHT(bitmap) - row - 1;
    append_outline_pixel(&outline, pos);

    for ( ; ; )
    {
	prev_row = row; prev_col = col;

	/* If there is no adjacent, unmarked pixel, we can't proceed
	   any further, so return an open outline. */
	if (!next_unmarked_outline_pixel(&row, &col, &search_dir,
					 bitmap, marked))
	{
	    outline.open = true;
	    break;
	}

	/* If we've returned to the starting pixel, we're done. */
	if (row == original_row && col == original_col)
	    break;

	/* If we've moved to a new pixel, mark all edges of the previous
	   pixel so that it won't be revisited.  Exceptions are the
	   starting pixel (because we need to be able to return to it
	   to close the outline) and pixels at junctions through which
	   there are additional, unmarked paths yet to be followed). */
	if ((prev_row != original_row || prev_col != original_col)
	    && !is_open_junction(prev_row, prev_col, bitmap, *marked))
	    mark_pixel(prev_row, prev_col, marked);

	/* Add the new pixel to the output list. */
	pos.x = col; pos.y = BITMAP_HEIGHT(bitmap) - row - 1;
	append_outline_pixel(&outline, pos);
    }
    if (!outline.open)
	mark_pixel(original_row, original_col, marked);

    return outline;
}


/* Add an outline to an outline list. */

static void
append_pixel_outline (pixel_outline_list_type *outline_list,
		      pixel_outline_type outline)
{
  O_LIST_LENGTH (*outline_list)++;
  XREALLOC (outline_list->data, outline_list->length * sizeof (pixel_outline_type));
  O_LIST_OUTLINE (*outline_list, O_LIST_LENGTH (*outline_list) - 1) = outline;
}


/* Free the list of outline lists. */

#ifdef _EXPORTING
__declspec(dllexport) void
__stdcall free_pixel_outline_list (pixel_outline_list_type *outline_list)
#else
void
free_pixel_outline_list (pixel_outline_list_type *outline_list)
#endif
{
  unsigned this_outline;

  for (this_outline = 0; this_outline < outline_list->length; this_outline++)
  {
    pixel_outline_type o = outline_list->data[this_outline];
    if (o.data != NULL)
      free (o.data);
  }
  outline_list->length = 0;

  if (outline_list->data != NULL)
  {
    free (outline_list->data);
    outline_list->data = NULL;
  }

  flush_log_output ();
}


/* Return an empty list of pixels.  */


static pixel_outline_type
new_pixel_outline (void)
{
  pixel_outline_type pixel_outline;

  O_LENGTH (pixel_outline) = 0;
  pixel_outline.data = NULL;
  pixel_outline.open = false;

  return pixel_outline;
}


/* Concatenate two pixel lists.  The two lists are assumed to have the
   same starting pixel and to proceed in opposite directions therefrom. */

static void
concat_pixel_outline(pixel_outline_type *o1, const pixel_outline_type *o2)
{
    int src, dst;
    unsigned o1_length, o2_length;
    if (!o1 || !o2 || O_LENGTH(*o2) <= 1) return;

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

static void
append_outline_pixel (pixel_outline_type *o, at_coord c)
{
  O_LENGTH (*o)++;
  XREALLOC (o->data, O_LENGTH (*o) * sizeof (at_coord));
  O_COORDINATE (*o, O_LENGTH (*o) - 1) = c;
}


/* Is this really an edge and is it still unmarked? */

static at_bool
is_unmarked_outline_edge (unsigned short row, unsigned short col,
	                    edge_type edge, bitmap_type character,
                            bitmap_type marked, color_type color, at_exception_type * exp)
{
  return
    (at_bool)(!is_marked_edge (edge, row, col, marked)
	&& is_outline_edge (edge, character, row, col, color, exp));
}


/* We return the next edge on the pixel at position ROW and COL which is
   an unmarked outline edge.  By ``next'' we mean either the one sent in
   in STARTING_EDGE, if it qualifies, or the next such returned by
   `next_edge'.  */

static edge_type
next_unmarked_outline_edge (unsigned short row, unsigned short col,
	                    edge_type starting_edge, bitmap_type character,
                            bitmap_type marked,
			    at_exception_type * exp)
{
  color_type color;
  edge_type edge = starting_edge;

  assert (edge != NO_EDGE);

  color = GET_COLOR(character, row, col);
  while (is_marked_edge (edge, row, col, marked)
	 || !is_outline_edge (edge, character, row, col, color, exp))
    {
      CHECK_FATAL();
      
      edge = next_edge (edge);
      if (edge == starting_edge)
        return NO_EDGE;
    }
 cleanup:
  return edge;
}


/* We check to see if the edge of the pixel at position ROW and COL
   is an outline edge */

static at_bool
is_outline_edge (edge_type edge, bitmap_type character,
		 unsigned short row, unsigned short col, color_type color,
		 at_exception_type * exp)
{
  /* If this pixel isn't of the same color, it's not part of the outline. */
  if (!COLOR_EQUAL (GET_COLOR (character, row, col), color))
    return false;

  switch (edge)
  {
    case LEFT:
      return (at_bool)(col == 0 || !COLOR_EQUAL (GET_COLOR (character, row, col - 1), color));

    case TOP:
      return (at_bool)(row == 0 || !COLOR_EQUAL (GET_COLOR (character, row - 1, col), color));

    case RIGHT:
      return (at_bool)(col == BITMAP_WIDTH (character) - 1
	    || !COLOR_EQUAL(GET_COLOR (character, row, col + 1), color));

    case BOTTOM:
      return (at_bool)(row == BITMAP_HEIGHT (character) - 1
	    || !COLOR_EQUAL(GET_COLOR (character, row + 1, col), color));

    case NO_EDGE:
    default:
      LOG1 ("is_outline_edge: Bad edge value(%d)", edge);
      at_exception_fatal(exp, "is_outline_edge: Bad edge value");
  }

  return false; /* NOT REACHED */
}


/* Return the edge which is counterclockwise-adjacent to EDGE.  This
   code makes use of the ``numericness'' of C enumeration constants;
   sorry about that.  */

static edge_type
next_edge (edge_type edge)
{
  return (edge_type)(((int)edge == (int)NO_EDGE) ? edge : (edge + 1) % NUM_EDGES);
}


/* Return the edge opposite to EDGE.  */

static edge_type
opposite_edge(edge_type edge)
{
    return (edge_type)(((int)edge == (int)NO_EDGE) ? edge : (edge + 2) % NUM_EDGES);
}


/* If EDGE is not already marked, we mark it; otherwise, it's a fatal error.
   The position ROW and COL should be inside the bitmap MARKED. EDGE can be
   NO_EDGE. */

static void
mark_edge (edge_type edge, unsigned short row, unsigned short col, bitmap_type *marked)
{
  *BITMAP_PIXEL (*marked, row, col) |= 1 << edge;
}


/* Mark all edges of the pixel ROW/COL in MARKED. */

static void
mark_pixel(unsigned short row, unsigned short col, bitmap_type *marked)
{
    *BITMAP_PIXEL(*marked, row, col) |= ((1 << NUM_EDGES) - 1);
}


/* Test if the pixel at ROW/COL in MARKED is marked. */
#if 0
static at_bool
is_marked_pixel(unsigned short row, unsigned short col, bitmap_type marked)
{
    unsigned mark = (1 << NUM_EDGES) - 1;
    return (at_bool)((*BITMAP_PIXEL(marked, row, col) & mark) == mark);
}
#endif /* 0 */

static at_bool
next_unmarked_outline_pixel(unsigned short *row, unsigned short *col,
    direction_type *dir, bitmap_type character, bitmap_type *marked)
{
    color_type color;
    int next_dir_offset = 1;
    unsigned orig_row = *row, orig_col = *col;
    direction_type orig_dir = *dir, test_dir = *dir;
    edge_type test_edge;

    color = GET_COLOR(character, *row, *col);
    do
    {
	short delta_r = COMPUTE_DELTA(ROW, test_dir);
	short delta_c = COMPUTE_DELTA(COL, test_dir);
	unsigned short test_row = orig_row + delta_r;
	unsigned short test_col = orig_col + delta_c;

	test_edge = (edge_type)((test_dir / 2 + 3) % 4);
	if (BITMAP_VALID_PIXEL(character, test_row, test_col)
	    && COLOR_EQUAL(GET_COLOR(character, test_row, test_col), color)
	    && !is_marked_edge(test_edge, test_row, test_col, *marked))
	{
	    mark_edge(test_edge, test_row, test_col, marked);
	    mark_edge(opposite_edge(test_edge), orig_row, orig_col, marked);
	    *row = test_row;
	    *col = test_col;
	    *dir = test_dir;
	    break;
	}

	test_dir = (direction_type)((orig_dir + next_dir_offset + 8) % 8);
	next_dir_offset = -next_dir_offset;
	if (next_dir_offset > 0)
	    ++next_dir_offset;
    }
    while (next_dir_offset != -4);

    return ((*row != orig_row || *col != orig_col) ? true : false);
}


/* Return the number of pixels adjacent to pixel ROW/COL that are black. */

static unsigned
num_neighbors(unsigned short row, unsigned short col, bitmap_type character)
{
    unsigned dir, count = 0;
    color_type color = GET_COLOR(character, row, col);
    for (dir = NORTH; dir <= NORTHEAST; dir++)
    {
	int delta_r = COMPUTE_DELTA(ROW, dir);
	int delta_c = COMPUTE_DELTA(COL, dir);
	unsigned int test_row = row + delta_r;
	unsigned int test_col = col + delta_c;
	if (BITMAP_VALID_PIXEL(character, test_row, test_col)
	    && COLOR_EQUAL(GET_COLOR(character, test_row, test_col), color))
	    ++count;
    }
    return count;
}


/* Return the number of pixels adjacent to pixel ROW/COL that are marked. */

static unsigned
num_marked_neighbors(unsigned short row, unsigned short col, bitmap_type marked)
{
    unsigned dir, count = 0, mark = (1 << NUM_EDGES) - 1;
    for (dir = NORTH; dir <= NORTHEAST; dir++)
    {
	int delta_r = COMPUTE_DELTA(ROW, dir);
	int delta_c = COMPUTE_DELTA(COL, dir);
	unsigned int test_row = row + delta_r;
	unsigned int test_col = col + delta_c;
	if (BITMAP_VALID_PIXEL(marked, test_row, test_col)
	    && (*BITMAP_PIXEL(marked, test_row, test_col) & mark))
	    ++count;
    }
    return count;
}


/* Test if the pixel at ROW/COL lies at a junction through which more
   than one unmarked path passes. */

static at_bool
is_open_junction(unsigned short row, unsigned short col, bitmap_type character,
    bitmap_type marked)
{
    unsigned n = num_neighbors(row, col, character);
    return (at_bool)(n > 2 && (n - num_marked_neighbors(row, col, marked) > 1));
}


/* Test if the edge EDGE at ROW/COL in MARKED is marked.  */

static at_bool
is_marked_edge (edge_type edge, unsigned short row, unsigned short col, bitmap_type marked)
{
  return
    (at_bool)(edge == NO_EDGE ? false : (*BITMAP_PIXEL (marked, row, col) & (1 << edge)) != 0);
}

static at_coord NextPoint(bitmap_type bitmap, edge_type *edge, unsigned short *row, unsigned short *col,
			  color_type color, at_bool clockwise, bitmap_type marked,
			  at_exception_type * exp)
{
  at_coord pos = {0, 0};

  if (!clockwise)
    switch(*edge)
  {
      case TOP:
	  /* WEST */
      if((*col >=1
	    &&!is_marked_edge(TOP,*row,*col-1, marked)
	    && is_outline_edge(TOP,bitmap,*row,*col-1, color, exp)))
	  {
        /**edge = TOP;*/
	    (*col)--;
	    pos.x = *col;
	    pos.y = BITMAP_HEIGHT (bitmap) - *row;
	    break;
	  }
      CHECK_FATAL();
	/* NORTHWEST */
    if((*col >=1 && *row >= 1
	  && !is_marked_edge(RIGHT,*row-1,*col-1, marked)
	  && is_outline_edge(RIGHT,bitmap,*row-1,*col-1, color, exp)) &&
	  !(is_marked_edge(LEFT,*row-1,*col, marked) && is_marked_edge(TOP, *row,*col-1, marked)) &&
	  !(is_marked_edge(BOTTOM,*row-1,*col, marked) && is_marked_edge(RIGHT, *row,*col-1, marked)))
    {
          *edge = RIGHT;
          (*col)--;
	  (*row)--;
	  pos.x = *col+1;
	  pos.y = BITMAP_HEIGHT (bitmap) - *row;
	  break;
    } 
    CHECK_FATAL();
    if ((!is_marked_edge(LEFT,*row,*col, marked)
	  && is_outline_edge(LEFT,bitmap,*row,*col, color, exp)))
    {
	  *edge = LEFT;
	  pos.x = *col;
	  pos.y = BITMAP_HEIGHT (bitmap) - *row - 1;
	  break;
    }
    CHECK_FATAL();
    *edge = NO_EDGE;
    break;
  case RIGHT: 
	/* NORTH */
    if((*row >=1
	  &&!is_marked_edge(RIGHT,*row-1,*col, marked)
	  && is_outline_edge(RIGHT,bitmap,*row-1,*col, color, exp)))
    {
      /**edge = RIGHT;*/
	  (*row)--;
	  pos.x = *col+1;
	  pos.y = BITMAP_HEIGHT (bitmap) - *row;
	  break;
    }
    CHECK_FATAL();
	/* NORTHEAST */
    if((*col+1 < BITMAP_WIDTH (marked) && *row >= 1
	  && !is_marked_edge(BOTTOM,*row-1,*col+1, marked)
	  && is_outline_edge(BOTTOM,bitmap,*row-1,*col+1, color, exp)) &&
	  !(is_marked_edge(LEFT,*row,*col+1, marked) && is_marked_edge(BOTTOM, *row-1,*col, marked)) &&
	  !(is_marked_edge(TOP,*row,*col+1, marked) && is_marked_edge(RIGHT, *row-1,*col, marked)))
    {
	  *edge = BOTTOM;
	  (*col)++;
	  (*row)--;
	  pos.x = *col+1;
	  pos.y = BITMAP_HEIGHT (bitmap) - *row - 1;
	  break;
    } 
    CHECK_FATAL();
    if ((!is_marked_edge(TOP,*row,*col, marked)
	  && is_outline_edge(TOP,bitmap,*row,*col, color, exp)))
    {
	  *edge = TOP;
	  pos.x = *col;
	  pos.y = BITMAP_HEIGHT (bitmap) - *row;
	  break;
    }
    CHECK_FATAL();
    *edge = NO_EDGE;
    break;
  case BOTTOM: 
	/* EAST */
    if((*col+1 < BITMAP_WIDTH (marked)
	  && !is_marked_edge(BOTTOM,*row,*col+1, marked)
          && is_outline_edge(BOTTOM,bitmap,*row,*col+1, color, exp)))
    {
      /**edge = BOTTOM;*/
      (*col)++;
	  pos.x = *col+1;
	  pos.y = BITMAP_HEIGHT (bitmap) - *row - 1;
	  break;
    }
    CHECK_FATAL();
	/* SOUTHEAST */
    if((*col+1 < BITMAP_WIDTH (marked) && *row+1 < BITMAP_HEIGHT (marked)
	  && !is_marked_edge(LEFT,*row+1,*col+1, marked)
	  && is_outline_edge(LEFT,bitmap,*row+1,*col+1, color, exp)) &&
	  !(is_marked_edge(TOP,*row+1,*col, marked) && is_marked_edge(LEFT, *row,*col+1, marked)) &&
	  !(is_marked_edge(RIGHT,*row+1,*col, marked) && is_marked_edge(BOTTOM,*row,*col+1, marked)))
    {
	  *edge = LEFT;
	  (*col)++;
	  (*row)++;
	  pos.x = *col;
	  pos.y = BITMAP_HEIGHT (bitmap) - *row - 1;
	  break;
    }
    CHECK_FATAL();
    if ((!is_marked_edge(RIGHT,*row,*col, marked)
	  && is_outline_edge(RIGHT,bitmap,*row,*col, color, exp)))
    {
	  *edge = RIGHT;
	  pos.x = *col+1;
	  pos.y = BITMAP_HEIGHT (bitmap) - *row;
	  break;
    }
    CHECK_FATAL();
    *edge = NO_EDGE;
    break;
  case LEFT: 
	/* SOUTH */
    if((*row+1 < BITMAP_HEIGHT (marked)
	  && !is_marked_edge(LEFT,*row+1,*col, marked)
	  && is_outline_edge(LEFT,bitmap,*row+1,*col, color, exp)))
    {
      /**edge = LEFT;*/
	  (*row)++;
	  pos.x = *col;
	  pos.y = BITMAP_HEIGHT (bitmap) - *row - 1;
	  break;
    }
    CHECK_FATAL();
	/* SOUTHWEST */
    if((*col >= 1 && *row+1 < BITMAP_HEIGHT (marked)
	  && !is_marked_edge(TOP,*row+1,*col-1, marked)
	  && is_outline_edge(TOP,bitmap,*row+1,*col-1, color, exp)) &&
	  !(is_marked_edge(RIGHT,*row,*col-1, marked) && is_marked_edge(TOP, *row+1,*col, marked)) &&
	  !(is_marked_edge(BOTTOM, *row,*col-1, marked) && is_marked_edge(LEFT, *row+1,*col, marked)))
    {
	  *edge = TOP;
      (*col)--;
	  (*row)++;
	  pos.x = *col;
	  pos.y = BITMAP_HEIGHT (bitmap) - *row;
	  break;
    } 
    CHECK_FATAL();
    if ((!is_marked_edge(BOTTOM,*row,*col, marked)
	  && is_outline_edge(BOTTOM,bitmap,*row,*col, color, exp)))
    {
	  *edge = BOTTOM;
	  pos.x = *col+1;
	  pos.y = BITMAP_HEIGHT (bitmap) - *row - 1;
	  break;
    }
    CHECK_FATAL();
  case NO_EDGE:
  default:
    *edge = NO_EDGE;
    break;
  }
else
  switch(*edge)
  {
  case TOP:
    if ((!is_marked_edge(LEFT,*row,*col, marked)
	  && is_outline_edge(LEFT,bitmap,*row,*col, color, exp)))
    {
	  *edge = LEFT;
	  pos.x=*col;
	  pos.y=BITMAP_HEIGHT (bitmap) - *row - 1;
	  break;
    }
    CHECK_FATAL();
	/* WEST */
    if((*col >=1
	  &&!is_marked_edge(TOP,*row,*col-1, marked)
	  && is_outline_edge(TOP,bitmap,*row,*col-1, color, exp)))
	{
      /**edge = TOP;*/
      (*col)--;
	  pos.x = *col;
	  pos.y = BITMAP_HEIGHT (bitmap) - *row;
	  break;
	}
    CHECK_FATAL();
	/* NORTHWEST */
    if((*col >=1 && *row >= 1
	  && !is_marked_edge(RIGHT,*row-1,*col-1, marked)
	  && is_outline_edge(RIGHT,bitmap,*row-1,*col-1, color, exp)))
    {
	  *edge = RIGHT;
      (*col)--;
	  (*row)--;
	  pos.x = *col+1;
	  pos.y = BITMAP_HEIGHT (bitmap) - *row;
	  break;
    } 
    CHECK_FATAL();
    *edge = NO_EDGE;
    break;
  case RIGHT: 
    if ((!is_marked_edge(TOP,*row,*col, marked)
	  && is_outline_edge(TOP,bitmap,*row,*col, color, exp)))
    {
	  *edge = TOP;
	  pos.x = *col;
	  pos.y = BITMAP_HEIGHT (bitmap) - *row;
	  break;
    }
    CHECK_FATAL();
	/* NORTH */
    if((*row >=1
	  &&!is_marked_edge(RIGHT,*row-1,*col, marked)
	  && is_outline_edge(RIGHT,bitmap,*row-1,*col, color, exp)))
    {
      /**edge = RIGHT;*/
	  (*row)--;
	  pos.x = *col+1;
	  pos.y = BITMAP_HEIGHT (bitmap) - *row;
	  break;
    }
    CHECK_FATAL();
	/* NORTHEAST */
    if((*col+1 < BITMAP_WIDTH (marked) && *row >= 1
	  && !is_marked_edge(BOTTOM,*row-1,*col+1, marked)
	  && is_outline_edge(BOTTOM,bitmap,*row-1,*col+1, color, exp)))
    {
	  *edge = BOTTOM;
      (*col)++;
	  (*row)--;
	  pos.x = *col+1;
	  pos.y = BITMAP_HEIGHT (bitmap) - *row - 1;
	  break;
    }
    CHECK_FATAL();
    *edge = NO_EDGE;
    break;
  case BOTTOM: 
    if ((!is_marked_edge(RIGHT,*row,*col, marked)
	  && is_outline_edge(RIGHT,bitmap,*row,*col, color, exp)))
    {
	  *edge = RIGHT;
	  pos.x = *col+1;
	  pos.y = BITMAP_HEIGHT (bitmap) - *row;
	  break;
    }
    CHECK_FATAL();
	/* EAST */
    if((*col+1 < BITMAP_WIDTH (marked)
	  && !is_marked_edge(BOTTOM,*row,*col+1, marked)
	  && is_outline_edge(BOTTOM,bitmap,*row,*col+1, color, exp)))
    {
      /**edge = BOTTOM;*/
      (*col)++;
	  pos.x = *col+1;
	  pos.y = BITMAP_HEIGHT (bitmap) - *row - 1;
	  break;
    }
    CHECK_FATAL();
	/* SOUTHEAST */
    if((*col+1 < BITMAP_WIDTH (marked) && *row+1 < BITMAP_HEIGHT (marked)
	  && !is_marked_edge(LEFT,*row+1,*col+1, marked)
	  && is_outline_edge(LEFT,bitmap,*row+1,*col+1, color, exp)))
    {
	  *edge = LEFT;
      (*col)++;
	  (*row)++;
	  pos.x = *col;
	  pos.y = BITMAP_HEIGHT (bitmap) - *row - 1;
	  break;
    }
    CHECK_FATAL();
    *edge = NO_EDGE;
    break;
  case LEFT: 
    if ((!is_marked_edge(BOTTOM,*row,*col, marked)
	  && is_outline_edge(BOTTOM,bitmap,*row,*col, color, exp)))
    {
	  *edge = BOTTOM;
	  pos.x = *col+1;
	  pos.y = BITMAP_HEIGHT (bitmap) - *row - 1;
	  break;
    }
    CHECK_FATAL();
	/* SOUTH */
    if((*row+1 < BITMAP_HEIGHT (marked)
	  && !is_marked_edge(LEFT,*row+1,*col, marked)
	  && is_outline_edge(LEFT,bitmap,*row+1,*col, color, exp)))
    {
    /**edge = LEFT;*/
	  (*row)++;
	  pos.x = *col;
	  pos.y = BITMAP_HEIGHT (bitmap) - *row - 1;
	  break;
    }
    CHECK_FATAL();
	/* SOUTHWEST */
    if((*col >= 1 && *row+1 < BITMAP_HEIGHT (marked)
	  && !is_marked_edge(TOP,*row+1,*col-1, marked)
	  && is_outline_edge(TOP,bitmap,*row+1,*col-1, color, exp)))
    {
	  *edge = TOP;
	  (*col)--;
	  (*row)++;
	  pos.x = *col;
	  pos.y = BITMAP_HEIGHT (bitmap) - *row;
	  break;
    }
    CHECK_FATAL();
  case NO_EDGE:
  default: 
    *edge = NO_EDGE;
    break;
  }
 cleanup:
  return(pos);
}
