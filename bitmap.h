/* bitmap.h: definition for a bitmap type.  No packing is done by
   default; each pixel is represented by an entire byte.  Among other
   things, this means the type can be used for both grayscale and binary
   images. */

#ifndef BITMAP_H
#define BITMAP_H

#include "autotrace.h"
#include <stdio.h>

/* The basic structure and macros to access it.  */
typedef at_bitmap_type bitmap_type;

/* The number of color planes of each pixel */
#define BITMAP_PLANES(b)  ((b).np)

/* The pixels, represented as an array of bytes (in contiguous storage).
   Each pixel is represented by np bytes.  */
#define BITMAP_BITS(b)  ((b).bitmap)

/* These are convenient abbreviations for geting inside the members.  */
#define BITMAP_WIDTH(b)  ((b).width)
#define BITMAP_HEIGHT(b)  ((b).height)

/* This is the pixel at [ROW,COL].  */
#define BITMAP_PIXEL(b, row, col)					\
  ((BITMAP_BITS (b) + (row) * BITMAP_PLANES (b) * BITMAP_WIDTH (b)	\
        + (col) * BITMAP_PLANES(b)))

#define BITMAP_VALID_PIXEL(b, row, col)					\
   	((row) < BITMAP_HEIGHT (b) && (col) < BITMAP_WIDTH (b))

/* Allocate storage for the bits, set them all to white, and return an
   initialized structure.  */
extern bitmap_type new_bitmap (unsigned int, unsigned int);

/* Free that storage.  */
extern void free_bitmap (bitmap_type *);

#endif /* not BITMAP_H */
