/* pxl-outline.c: find the edges of the bitmap image; we call each such
   edge an ``outline''; each outline is made up of one or more pixels;
   and each pixel participates via one or more edges. */

#include "message.h"
#include "types.h"
#include "bitmap.h"
#include "edge.h"
#include "bitmap.h"
#include "logreport.h"
#include "xmem.h"
#include "pxl-outline.h"

static const color_type WHITE = {255, 255, 255};

static pixel_outline_type find_one_outline (bitmap_type, edge_type,
			  		    unsigned, unsigned, bitmap_type *);
static void append_pixel_outline (pixel_outline_list_type *,
                                  pixel_outline_type);
static pixel_outline_type new_pixel_outline (void);
static void append_outline_pixel (pixel_outline_type *, coordinate_type);
static void append_coordinate (pixel_outline_type *, int, int, edge_type);


/* A character is made up of a list of one or more outlines.  Here, we
   go through a character's bitmap top to bottom, left to right, looking
   for the next pixel with an unmarked edge also on the character's outline.
   Each one of these we find is the starting place for one outline.  We
   find these outlines and put them in a list to return.  */

#ifdef _EXPORTING
__declspec(dllexport) pixel_outline_list_type
__stdcall find_outline_pixels (bitmap_type bitmap)
#else
pixel_outline_list_type
find_outline_pixels (bitmap_type bitmap)
#endif
{
  pixel_outline_list_type outline_list;
  unsigned row, col;
  bitmap_type marked = new_bitmap (BITMAP_DIMENSIONS (bitmap));
  color_type color;

  O_LIST_LENGTH (outline_list) = 0;
  outline_list.data = NULL;

  for (row = 0; row < BITMAP_HEIGHT (bitmap); row++)
    for (col = 0; col < BITMAP_WIDTH (bitmap); col++)
      {
	edge_type edge;

        color = GET_COLOR (bitmap, row, col);
        if (COLOR_EQUAL (color, WHITE)) /* pixel is white */
          continue;

        edge = next_unmarked_outline_edge (row, col, START_EDGE, bitmap,
                                           marked, color);

	if (edge != no_edge)
	  {
            pixel_outline_type outline;
            boolean clockwise = edge == bottom;

            LOG2 ("#%u: (%sclockwise)", O_LIST_LENGTH (outline_list),
                                        clockwise ? "" : "counter");

            outline = find_one_outline (bitmap, edge, row, col, &marked);

            /* Outside outlines will start at a top edge, and move
               counterclockwise, and inside outlines will start at a
               bottom edge, and move clockwise.  This happens because of
               the order in which we look at the edges.  */
            O_CLOCKWISE (outline) = clockwise;
	    append_pixel_outline (&outline_list, outline);

            LOG1 (" [%u].\n", O_LENGTH (outline));
	  }
      }

  free_bitmap (&marked);

  flush_log_output ();

  return outline_list;
}


/* Here we find one of a character C's outlines.  We're passed the
   position (ORIGINAL_ROW and ORIGINAL_COL) of a starting pixel and one
   of its unmarked edges, ORIGINAL_EDGE.  We traverse the adjacent edges
   of the outline pixels, appending to the coordinate list.  We keep
   track of the marked edges in MARKED, so it should be initialized to
   zeros when we first get it.  */

static pixel_outline_type
find_one_outline (bitmap_type bitmap, edge_type original_edge,
		  unsigned original_row, unsigned original_col,
		  bitmap_type *marked)
{
  pixel_outline_type outline = new_pixel_outline ();
  unsigned row = original_row, col = original_col;
  edge_type edge = original_edge;

  outline.color = GET_COLOR (bitmap, row, col);

  do
    {
      /* Put this edge on to the output list, changing to Cartesian, and
         taking account of the side bearings.  */
      append_coordinate (&outline, col,
                         BITMAP_HEIGHT (bitmap) - row, edge);

      mark_edge (edge, row, col, marked);
      next_outline_edge (bitmap, &edge, &row, &col, outline.color);
    }
  while (row != original_row || col != original_col || edge != original_edge);

  return outline;
}


/* Append an outline to an outline list.  This is called when we have
   completed an entire pixel outline.  */

static void
append_pixel_outline (pixel_outline_list_type *outline_list,
		      pixel_outline_type outline)
{
  O_LIST_LENGTH (*outline_list)++;
  XREALLOC (outline_list->data, outline_list->length * sizeof (pixel_outline_type));
  O_LIST_OUTLINE (*outline_list, O_LIST_LENGTH (*outline_list) - 1) = outline;
}


/* Here is a routine that frees a list of such lists.  */

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

  if (outline_list->data != NULL)
    free (outline_list->data);

  flush_log_output ();
}


/* Return an empty list of pixels.  */


static pixel_outline_type
new_pixel_outline (void)
{
  pixel_outline_type pixel_outline;

  O_LENGTH (pixel_outline) = 0;
  pixel_outline.data = NULL;

  return pixel_outline;
}


/* Add the coordinate C to the pixel list O.  */

static void
append_outline_pixel (pixel_outline_type *o, coordinate_type c)
{
  O_LENGTH (*o)++;
  XREALLOC (o->data, O_LENGTH (*o) * sizeof (coordinate_type));
  O_COORDINATE (*o, O_LENGTH (*o) - 1) = c;
}


/* We are given an (X,Y) in Cartesian coordinates, and the edge of the pixel
   we're on.  We append a corner of that pixel as our coordinate.
   If we're on a top edge, we use the upper-left hand corner; right edge
   => upper right; bottom edge => lower right; left edge => lower left.  */

static void
append_coordinate (pixel_outline_type *o, int x, int y, edge_type edge)
{
  coordinate_type c;
  c.x = x;
  c.y = y;

  switch (edge)
    {
    case top:
      c.y++;
      break;

    case right:
      c.x++;
      c.y++;
      break;

    case bottom:
      c.x++;
      break;

    case left:
      break;

    default:
      FATAL1 ("append_coordinate: Bad edge (%d)", edge);
    }

  LOG2 (" (%d,%d)", c.x, c.y);
  append_outline_pixel (o, c);
}

/* version 0.24 */
