/* input.h: interface for input handlers

   Copyright (C) 1999, 2000, 2001 Bernhard Herzog.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public License
   as published by the Free Software Foundation; either version 2.1 of
   the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with this library; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
   USA. */

#ifndef INPUT_H
#define INPUT_H
#include "types.h"
#include "autotrace.h"
#include "exception.h"

#include <glib.h>

#ifdef __cplusplus
extern "C" {
#endif                          /* __cplusplus */

/* Input handler should be implemented with using
   following functions and macros. */

  typedef
   at_bitmap(*at_input_func) (gchar * name, at_input_opts_type * opts, at_msg_func msg_func, gpointer msg_data, gpointer user_data);

/* at_input_add_handler
   Register an input handler to autotrace.
   If a handler for the suffix is already existed, do nothing. */
  extern int at_input_add_handler(const gchar * suffix, const gchar * description, at_input_func reader);
/* at_input_add_handler_full
   If OVERRIDE is TRUE and if the old handler for suffix is existed,
   remove the old handler first then add new handler.
   If OVERRIDE is false, do nothing. */
  extern int at_input_add_handler_full(const gchar * suffix, const gchar * description, at_input_func reader, gboolean override, gpointer user_data, GDestroyNotify user_data_destroy_func);

/* at_bitmap_init
   Return initialized at_bitmap value.

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
   mem for at_bitmap; and returns a pointer
   for the mem.
   at_bitmap_new is for autotrace library user.
   at_bitmap_init is for input-handler developer.
   Don't use at_bitmap_new in your input-handler. */
  extern at_bitmap at_bitmap_init(unsigned char *area, unsigned short width, unsigned short height, unsigned int planes);

/* TODO: free storage */

/* The number of color planes of each pixel */
#define AT_BITMAP_PLANES(b)  ((b)->np)

/* The pixels, represented as an array of bytes (in contiguous storage).
   Each pixel is represented by np bytes.  */
#define AT_BITMAP_BITS(b)  ((b)->bitmap)

/* These are convenient abbreviations for geting inside the members.  */
#define AT_BITMAP_WIDTH(b)  ((b)->width)
#define AT_BITMAP_HEIGHT(b)  ((b)->height)

/* This is the pixel at [ROW,COL].  */
#define AT_BITMAP_PIXEL(b, row, col)					\
  ((AT_BITMAP_BITS (b) + (row) * AT_BITMAP_PLANES (b) * AT_BITMAP_WIDTH (b)	\
        + (col) * AT_BITMAP_PLANES(b)))

/* at_ prefix removed version */
#define AT_BITMAP_VALID_PIXEL(b, row, col)					\
  ((row) < AT_BITMAP_HEIGHT (b) && (col) < AT_BITMAP_WIDTH (b))

#ifdef __cplusplus
}
#endif                          /* __cplusplus */
#endif                          /* Not def: INPUT_H */
