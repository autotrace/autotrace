/* input.h: interface for input handlers
   Copyright (C) 1999 Bernhard Herzog. 

   About bitmap data type, see also bitmap.h. */

#ifndef INPUT_H
#define INPUT_H
#include "types.h"
#include "autotrace.h"

/* Input handler should be implemented with using 
   following functions and macros. */

/* at_input_add_handler
   Register an input handler to autotrace. */
extern int at_input_add_handler (at_string suffix, 
				 at_string description,
				 at_input_read_func func);

/* at_bitmap_init
   Return initialized at_bitmap_type value.

   args:
   AREA is used as a storage of returned value.
   If AREA is NULL, the storage is newly allocated
   by at_bitmap_init. In such case the size of storage
   is automatically calculated by WIDTH, HEIGHT and PLANES.

   PLANES must be 1(gray scale) or 3(RGB color).

   return value:
   The return value is not newly allocated.
   Only the storage is allocated if AREA is NULL.
   On the other hand, at_bitmap_new allocates
   mem for at_bitmap_type; and returns a pointer
   for the mem. 
   at_bitmap_new is for autotrace library user.
   at_bitmap_init is for input-handler developer.
   Don't use at_bitmap_new in your input-handler. */
extern at_bitmap_type at_bitmap_init(unsigned char * area,
				     unsigned short width,
				     unsigned short height,
				     unsigned int planes);

/* TODO: free storage */

/* The number of color planes of each pixel */
#define AT_BITMAP_PLANES(b)  ((b).np)

/* The pixels, represented as an array of bytes (in contiguous storage).
   Each pixel is represented by np bytes.  */
#define AT_BITMAP_BITS(b)  ((b).bitmap)

/* These are convenient abbreviations for geting inside the members.  */
#define AT_BITMAP_WIDTH(b)  ((b).width)
#define AT_BITMAP_HEIGHT(b)  ((b).height)

/* This is the pixel at [ROW,COL].  */
#define AT_BITMAP_PIXEL(b, row, col)					\
  ((AT_BITMAP_BITS (b) + (row) * AT_BITMAP_PLANES (b) * AT_BITMAP_WIDTH (b)	\
        + (col) * AT_BITMAP_PLANES(b)))

#endif /* Not def: INPUT_H */
