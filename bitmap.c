/* bitmap.c: operations on bitmaps. */

#include "types.h"
#include "bitmap.h"
#include "xstd.h"
#include <string.h>

bitmap_type
new_bitmap (dimensions_type d)
{
  bitmap_type answer;
  unsigned size = DIMENSIONS_WIDTH (d) * DIMENSIONS_HEIGHT (d);

  BITMAP_DIMENSIONS (answer) = d;
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

/* version 0.24 */
