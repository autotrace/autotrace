/* pxl-outline.h: find a list of outlines which make up one character. */

#ifndef PXL_OUTLINE_H
#define PXL_OUTLINE_H

#include "autotrace.h"
#include "exception.h"
#include "bitmap.h"
#include "color.h"

/* This is a list of contiguous points on the bitmap.  */
typedef struct {
  at_coord *data;
  unsigned length;
  gboolean clockwise;
  at_color color;
  gboolean open;
} pixel_outline_type;

/* The Nth coordinate in the list.  */
#define O_COORDINATE(p_o, n) ((p_o).data[n])

/* The length of the list.  */
#define O_LENGTH(p_o) ((p_o).length)

/* Whether the outline moves clockwise or counterclockwise.  */
#define O_CLOCKWISE(p_o) ((p_o).clockwise)

/* Since a pixel outline is cyclic, the index of the next coordinate
   after the last is the first, and the previous coordinate before the
   first is the last.  */
#define O_NEXT(p_o, n) (((n) + 1) % O_LENGTH (p_o))
#define O_PREV(p_o, n) ((n) == 0				\
                         ? O_LENGTH (p_o) - 1			\
                         : (n) - 1)

/* And the character turns into a list of such lists.  */
typedef struct {
  pixel_outline_type *data;
  unsigned length;
} pixel_outline_list_type;

/* The Nth list in the list of lists.  */
#define O_LIST_OUTLINE(p_o_l, n) ((p_o_l).data[n])

/* The length of the list of lists.  */
#define O_LIST_LENGTH(p_o_l) ((p_o_l).length)

/* Find all pixels on the outline in the character C.  */
extern pixel_outline_list_type find_outline_pixels(at_bitmap * bitmap, at_color * bg_color, at_progress_func notify_progress, gpointer progress_data, at_testcancel_func test_cancel, gpointer testcancel_data, at_exception_type * exp);

/* Find all pixels on the center line of the character C.  */
extern pixel_outline_list_type find_centerline_pixels(at_bitmap * bitmap, at_color bg_color, at_progress_func notify_progress, gpointer progress_data, at_testcancel_func test_cancel, gpointer testcancel_data, at_exception_type * exp);

/* Free the memory in the list.  */
extern void free_pixel_outline_list(pixel_outline_list_type *);

#endif /* not PXL_OUTLINE_H */
