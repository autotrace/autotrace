/* bitmap.c: operations on bitmaps. */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* Def: HAVE_CONFIG_H */

#include "bitmap.h"
#include "xstd.h"

bitmap_type
new_bitmap (unsigned short width, unsigned short height)
{
  return at_bitmap_init(NULL,width,height,1);
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
