/* bitmap.c: operations on bitmaps. */

#include "bitmap.h"
#include "xstd.h"

bitmap_type
new_bitmap (unsigned short width, unsigned short height)
{
  bitmap_type answer;
  unsigned size = width * height;

  BITMAP_WIDTH (answer) = width;
  BITMAP_HEIGHT (answer) = height;
  BITMAP_PLANES (answer) = 1;
  XCALLOC (BITMAP_BITS (answer), size);

  return answer;
}


/* Free the storage that is allocated for a bitmap.  On the other hand,
   the bitmap might not have any storage allocated for it if it is zero
   in either dimension; in that case, don't free it.  */

void
free_bitmap (bitmap_type *b)
{
  if (BITMAP_BITS (*b) != NULL)
    free (BITMAP_BITS (*b));
}
