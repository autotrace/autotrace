/* outline.c: find the outlines of a bitmap image; each outline is made up of one or more pixels;
   and each pixel participates via one or more edges. */

#include "message.h"
#include "types.h"
#include "bitmap.h"
#include "color.h"
#include "bitmap.h"
#include "logreport.h"
#include "xstd.h"
#include "pxl-outline.h"
#include <assert.h>

typedef enum
{
  TOP, LEFT, BOTTOM, RIGHT, NO_EDGE
} edge_type;

static pixel_outline_type find_one_outline (bitmap_type, edge_type, unsigned,
  unsigned, bitmap_type *, bool, bool);
static void append_pixel_outline (pixel_outline_list_type *,
  pixel_outline_type);
static pixel_outline_type new_pixel_outline (void);
static void append_outline_pixel (pixel_outline_type *, coordinate_type);
static bool is_marked_edge (edge_type, unsigned, unsigned, bitmap_type);
static bool is_outline_edge (edge_type, bitmap_type, unsigned, unsigned,
  color_type);
static bool is_unmarked_outline_edge (unsigned, unsigned, edge_type,
  bitmap_type, bitmap_type, color_type);

static void mark_edge (edge_type e, unsigned row, unsigned col, bitmap_type *marked);
static coordinate_type NextPoint(bitmap_type, edge_type *, unsigned int *, unsigned int *, 
  color_type, bool, bitmap_type);


/* We go through a bitmap TOP to BOTTOM, LEFT to RIGHT, looking for each pixel with an unmarked edge
   that we consider a starting point of an outline. */

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

        /* A valid edge can be TOP for an outside outline.
           Outside outlines are traced counterclockwise */
		if (is_unmarked_outline_edge (row, col, edge = TOP, bitmap, marked, color))
		{
          pixel_outline_type outline;

          LOG1 ("#%u: (counterclockwise)", O_LIST_LENGTH (outline_list));

          outline = find_one_outline (bitmap, edge, row, col, &marked, false, false);

          O_CLOCKWISE (outline) = false;
		  append_pixel_outline (&outline_list, outline);

          LOG1 (" [%u].\n", O_LENGTH (outline));
		}

        /* A valid edge can be BOTTOM for an inside outline.
           Inside outlines are traced clockwise */
        if (row!=0) color = GET_COLOR (bitmap, row-1, col);

	    if (row != 0 && is_unmarked_outline_edge (row-1, col, edge = BOTTOM,
		  bitmap, marked, color))
	    {
          pixel_outline_type outline;

          /* This lines are for debugging only:
          LOG1 ("#%u: (clockwise)", O_LIST_LENGTH (outline_list));*/

          outline = find_one_outline (bitmap, edge, row-1, col, &marked, true, true);

          /* This lines are for debugging only:
		  O_CLOCKWISE (outline) = true;
	      append_pixel_outline (&outline_list, outline);

          LOG1 (" [%u].\n", O_LENGTH (outline));*/
        }

    }

  free_bitmap (&marked);

  flush_log_output ();

  return outline_list;
}


/* We calculate one single outline here. We pass the position of the starting pixel and the
   starting edge. All edges we track along will be marked and the outline pixels are appended
   to the coordinate list. */

static pixel_outline_type
find_one_outline (bitmap_type bitmap, edge_type original_edge,
		  unsigned original_row, unsigned original_col,
		  bitmap_type *marked, bool clockwise, bool ignore)
{
  pixel_outline_type outline;
  unsigned row = original_row, col = original_col;
  edge_type edge = original_edge;
  coordinate_type pos;

  pos.x = col;
  pos.y = BITMAP_HEIGHT (bitmap) - row + 1;

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
    pos = NextPoint (bitmap, &edge, &row, &col, outline.color, clockwise, *marked);
  }
  while (edge != NO_EDGE);

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


/* Add a point to the pixel list. */

static void
append_outline_pixel (pixel_outline_type *o, coordinate_type c)
{
  O_LENGTH (*o)++;
  XREALLOC (o->data, O_LENGTH (*o) * sizeof (coordinate_type));
  O_COORDINATE (*o, O_LENGTH (*o) - 1) = c;
}


/* Is this really an edge and is it still unmarked? */

static bool
is_unmarked_outline_edge (unsigned row, unsigned col,
	                    edge_type edge, bitmap_type character,
                            bitmap_type marked, color_type color)
{
  return
    !is_marked_edge (edge, row, col, marked)
	&& is_outline_edge (edge, character, row, col, color);
}


/* We check to see if the edge of the pixel at position ROW and COL
   is an outline edge */

static bool
is_outline_edge (edge_type edge, bitmap_type character,
		 unsigned row, unsigned col, color_type color)
{
  /* If this pixel isn't of the same color, it's not part of the outline. */
  if (!COLOR_EQUAL (GET_COLOR (character, row, col), color))
    return false;

  switch (edge)
  {
    case LEFT:
      return col == 0 || !COLOR_EQUAL (GET_COLOR (character, row, col - 1), color);

    case TOP:
      return row == 0 || !COLOR_EQUAL (GET_COLOR (character, row - 1, col), color);

    case RIGHT:
      return col == BITMAP_WIDTH (character) - 1
	    || !COLOR_EQUAL(GET_COLOR (character, row, col + 1), color);

    case BOTTOM:
      return row == BITMAP_HEIGHT (character) - 1
	    || !COLOR_EQUAL(GET_COLOR (character, row + 1, col), color);

    case NO_EDGE:
    default:
      FATAL1 ("is_outline_edge: Bad edge value(%d)", edge);
  }

  return 0; /* NOT REACHED */
}


/* If EDGE is not already marked, we mark it; otherwise, it's a fatal error.
   The position ROW and COL should be inside the bitmap MARKED. EDGE can be
   NO_EDGE. */

static void
mark_edge (edge_type edge, unsigned row, unsigned col, bitmap_type *marked)
{
  *BITMAP_PIXEL (*marked, row, col) |= 1 << edge;
}


/* Test if the edge EDGE at ROW/COL in MARKED is marked.  */

static bool
is_marked_edge (edge_type edge, unsigned row, unsigned col, bitmap_type marked)
{
  return
    edge == NO_EDGE ? false : (*BITMAP_PIXEL (marked, row, col) & (1 << edge)) != 0;
}


static coordinate_type NextPoint(bitmap_type bitmap, edge_type *edge, unsigned int *row, unsigned int *col,
  color_type color, bool clockwise, bitmap_type marked)
{
  coordinate_type pos;

  if (!clockwise)
    switch(*edge)
  {
      case TOP:
	  /* WEST */
      if((*col >=1
	    &&!is_marked_edge(TOP,*row,*col-1, marked)
	    && is_outline_edge(TOP,bitmap,*row,*col-1, color)))
	  {
        /**edge = TOP;*/
                (*col)--;
		pos.x = *col;
		pos.y = BITMAP_HEIGHT (bitmap) - (*row-1);
	    break;
      }
	/* NORTHWEST */
    if((*col >=1 && *row >= 1
	  && !is_marked_edge(RIGHT,*row-1,*col-1, marked)
	  && is_outline_edge(RIGHT,bitmap,*row-1,*col-1, color)) &&
	  !(is_marked_edge(LEFT,*row-1,*col, marked) && is_marked_edge(TOP, *row,*col-1, marked)) &&
	  !(is_marked_edge(BOTTOM,*row-1,*col, marked) && is_marked_edge(RIGHT, *row,*col-1, marked)))
    {
	  *edge = RIGHT;
      (*col)--;
	  (*row)--;
	  pos.x = *col+1;
	  pos.y = BITMAP_HEIGHT (bitmap) - (*row-1);
	  break;
    } 
    if ((!is_marked_edge(LEFT,*row,*col, marked)
	  && is_outline_edge(LEFT,bitmap,*row,*col, color)))
    {
	  *edge = LEFT;
	  pos.x = *col;
	  pos.y = BITMAP_HEIGHT (bitmap) - (*row);
	  break;
    }
	*edge = NO_EDGE;
    break;
  case RIGHT: 
	/* NORTH */
    if((*row >=1
	  &&!is_marked_edge(RIGHT,*row-1,*col, marked)
	  && is_outline_edge(RIGHT,bitmap,*row-1,*col, color)))
    {
      /**edge = RIGHT;*/
	  (*row)--;
	  pos.x = *col+1;
	  pos.y = BITMAP_HEIGHT (bitmap) - (*row-1);
	  break;
    }
	/* NORTHEAST */
    if((*col+1 < marked.dimensions.width && *row >= 1
	  && !is_marked_edge(BOTTOM,*row-1,*col+1, marked)
	  && is_outline_edge(BOTTOM,bitmap,*row-1,*col+1, color)) &&
	  !(is_marked_edge(LEFT,*row,*col+1, marked) && is_marked_edge(BOTTOM, *row-1,*col, marked)) &&
	  !(is_marked_edge(TOP,*row,*col+1, marked) && is_marked_edge(RIGHT, *row-1,*col, marked)))
    {
	  *edge = BOTTOM;
      (*col)++;
	  (*row)--;
	  pos.x = *col+1;
	  pos.y = BITMAP_HEIGHT (bitmap) - (*row);
	  break;
    } 
    if ((!is_marked_edge(TOP,*row,*col, marked)
	  && is_outline_edge(TOP,bitmap,*row,*col, color)))
    {
	  *edge = TOP;
	  pos.x = *col;
	  pos.y = BITMAP_HEIGHT (bitmap) - (*row-1);
	  break;
    }
	*edge = NO_EDGE;
    break;
  case BOTTOM: 
	/* EAST */
    if((*col+1 < marked.dimensions.width
	  && !is_marked_edge(BOTTOM,*row,*col+1, marked)
	  && is_outline_edge(BOTTOM,bitmap,*row,*col+1, color)))
    {
      /**edge = BOTTOM;*/
      (*col)++;
	  pos.x = *col+1;
	  pos.y = BITMAP_HEIGHT (bitmap) - (*row);
	  break;
    }
	/* SOUTHEAST */
    if((*col+1 < marked.dimensions.width && *row+1 < marked.dimensions.height
	  && !is_marked_edge(LEFT,*row+1,*col+1, marked)
	  && is_outline_edge(LEFT,bitmap,*row+1,*col+1, color)) &&
	  !(is_marked_edge(TOP,*row+1,*col, marked) && is_marked_edge(LEFT, *row,*col+1, marked)) &&
	  !(is_marked_edge(RIGHT,*row+1,*col, marked) && is_marked_edge(BOTTOM,*row,*col+1, marked)))
    {
	  *edge = LEFT;
      (*col)++;
	  (*row)++;
	  pos.x = *col;
	  pos.y = BITMAP_HEIGHT (bitmap) - (*row);
	  break;
    } 
    if ((!is_marked_edge(RIGHT,*row,*col, marked)
	  && is_outline_edge(RIGHT,bitmap,*row,*col, color)))
    {
	  *edge = RIGHT;
	  pos.x = *col+1;
	  pos.y = BITMAP_HEIGHT (bitmap) - (*row-1);
	  break;
    }
	*edge = NO_EDGE;
    break;
  case LEFT: 
	/* SOUTH */
    if((*row+1 < marked.dimensions.height
	  && !is_marked_edge(LEFT,*row+1,*col, marked)
	  && is_outline_edge(LEFT,bitmap,*row+1,*col, color)))
    {
      /**edge = LEFT;*/
	  (*row)++;
	  pos.x = *col;
	  pos.y = BITMAP_HEIGHT (bitmap) - (*row);
	  break;
    }
	/* SOUTHWEST */
    if((*col >= 1 && *row+1 < marked.dimensions.height
	  && !is_marked_edge(TOP,*row+1,*col-1, marked)
	  && is_outline_edge(TOP,bitmap,*row+1,*col-1, color)) &&
	  !(is_marked_edge(RIGHT,*row,*col-1, marked) && is_marked_edge(TOP, *row+1,*col, marked)) &&
	  !(is_marked_edge(BOTTOM, *row,*col-1, marked) && is_marked_edge(LEFT, *row+1,*col, marked)))
    {
	  *edge = TOP;
      (*col)--;
	  (*row)++;
	  pos.x = *col;
	  pos.y = BITMAP_HEIGHT (bitmap) - (*row-1);
	  break;
    } 
    if ((!is_marked_edge(BOTTOM,*row,*col, marked)
	  && is_outline_edge(BOTTOM,bitmap,*row,*col, color)))
    {
	  *edge = BOTTOM;
	  pos.x = *col+1;
	  pos.y = BITMAP_HEIGHT (bitmap) - (*row);
	  break;
    }
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
	  && is_outline_edge(LEFT,bitmap,*row,*col, color)))
    {
	  *edge = LEFT;
	  pos.x=*col;
	  pos.y=BITMAP_HEIGHT (bitmap) - (*row);
	  break;
    }
	/* WEST */
    if((*col >=1
	  &&!is_marked_edge(TOP,*row,*col-1, marked)
	  && is_outline_edge(TOP,bitmap,*row,*col-1, color)))
	{
      /**edge = TOP;*/
      (*col)--;
	  pos.x = *col;
	  pos.y = BITMAP_HEIGHT (bitmap) - (*row-1);
	  break;
    }
	/* NORTHWEST */
    if((*col >=1 && *row >= 1
	  && !is_marked_edge(RIGHT,*row-1,*col-1, marked)
	  && is_outline_edge(RIGHT,bitmap,*row-1,*col-1, color)))
    {
	  *edge = RIGHT;
      (*col)--;
	  (*row)--;
	  pos.x = *col+1;
	  pos.y = BITMAP_HEIGHT (bitmap) - (*row-1);
	  break;
    } 
	*edge = NO_EDGE;
    break;
  case RIGHT: 
    if ((!is_marked_edge(TOP,*row,*col, marked)
	  && is_outline_edge(TOP,bitmap,*row,*col, color)))
    {
	  *edge = TOP;
	  pos.x = *col;
	  pos.y = BITMAP_HEIGHT (bitmap) - (*row-1);
	  break;
    }
	/* NORTH */
    if((*row >=1
	  &&!is_marked_edge(RIGHT,*row-1,*col, marked)
	  && is_outline_edge(RIGHT,bitmap,*row-1,*col, color)))
    {
      /**edge = RIGHT;*/
	  (*row)--;
	  pos.x = *col+1;
	  pos.y = BITMAP_HEIGHT (bitmap) - (*row-1);
	  break;
    }
	/* NORTHEAST */
    if((*col+1 < marked.dimensions.width && *row >= 1
	  && !is_marked_edge(BOTTOM,*row-1,*col+1, marked)
	  && is_outline_edge(BOTTOM,bitmap,*row-1,*col+1, color)))
    {
	  *edge = BOTTOM;
      (*col)++;
	  (*row)--;
	  pos.x = *col+1;
	  pos.y = BITMAP_HEIGHT (bitmap) - (*row);
	  break;
    } 
	*edge = NO_EDGE;
    break;
  case BOTTOM: 
    if ((!is_marked_edge(RIGHT,*row,*col, marked)
	  && is_outline_edge(RIGHT,bitmap,*row,*col, color)))
    {
	  *edge = RIGHT;
	  pos.x = *col+1;
	  pos.y = BITMAP_HEIGHT (bitmap) - (*row-1);
	  break;
    }
	/* EAST */
    if((*col+1 < marked.dimensions.width
	  && !is_marked_edge(BOTTOM,*row,*col+1, marked)
	  && is_outline_edge(BOTTOM,bitmap,*row,*col+1, color)))
    {
      /**edge = BOTTOM;*/
      (*col)++;
	  pos.x = *col+1;
	  pos.y = BITMAP_HEIGHT (bitmap) - (*row);
	  break;
    }
	/* SOUTHEAST */
    if((*col+1 < marked.dimensions.width && *row+1 < marked.dimensions.height
	  && !is_marked_edge(LEFT,*row+1,*col+1, marked)
	  && is_outline_edge(LEFT,bitmap,*row+1,*col+1, color)))
    {
	  *edge = LEFT;
      (*col)++;
	  (*row)++;
	  pos.x = *col;
	  pos.y = BITMAP_HEIGHT (bitmap) - (*row);
	  break;
    } 
    *edge = NO_EDGE;
    break;
  case LEFT: 
    if ((!is_marked_edge(BOTTOM,*row,*col, marked)
	  && is_outline_edge(BOTTOM,bitmap,*row,*col, color)))
    {
	  *edge = BOTTOM;
	  pos.x = *col+1;
	  pos.y = BITMAP_HEIGHT (bitmap) - (*row);
	  break;
    }
	/* SOUTH */
    if((*row+1 < marked.dimensions.height
	  && !is_marked_edge(LEFT,*row+1,*col, marked)
	  && is_outline_edge(LEFT,bitmap,*row+1,*col, color)))
    {
    /**edge = LEFT;*/
	  (*row)++;
	  pos.x = *col;
	  pos.y = BITMAP_HEIGHT (bitmap) - (*row);
	  break;
    }
	/* SOUTHWEST */
    if((*col >= 1 && *row+1 < marked.dimensions.height
	  && !is_marked_edge(TOP,*row+1,*col-1, marked)
	  && is_outline_edge(TOP,bitmap,*row+1,*col-1, color)))
    {
	  *edge = TOP;
      (*col)--;
	  (*row)++;
	  pos.x = *col;
	  pos.y = BITMAP_HEIGHT (bitmap) - (*row-1);
	  break;
    }
  case NO_EDGE:
  default: 
    *edge = NO_EDGE;
    break;
  }
  return(pos);
}
