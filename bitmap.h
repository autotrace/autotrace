/* bitmap.h: definition for a bitmap type.  No packing is done by
   default; each pixel is represented by an entire byte.  Among other
   things, this means the type can be used for both grayscale and binary
   images. */

#ifndef BITMAP_H
#define BITMAP_H

#include "autotrace.h"
#include "input.h"
#include <stdio.h>

/* at_ prefix removed version */
typedef at_bitmap_type bitmap_type;
#define BITMAP_PLANES(b)          AT_BITMAP_PLANES(b)
#define BITMAP_BITS(b)            AT_BITMAP_BITS(b)  
#define BITMAP_WIDTH(b)           AT_BITMAP_WIDTH(b)  
#define BITMAP_HEIGHT(b)          AT_BITMAP_HEIGHT(b) 
#define BITMAP_PIXEL(b, row, col) AT_BITMAP_PIXEL(b, row, col)

#define BITMAP_VALID_PIXEL(b, row, col)					\
   	((row) < BITMAP_HEIGHT (b) && (col) < BITMAP_WIDTH (b))

/* Allocate storage for the bits, set them all to white, and return an
   initialized structure.  */
extern bitmap_type new_bitmap (unsigned short width, unsigned short height);

/* Free that storage.  */
extern void free_bitmap (bitmap_type *);

#endif /* not BITMAP_H */
