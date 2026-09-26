/*
 * Copyright (C) 2000 MenTaLguY
 * SPDX-FileCopyrightText: © 2000 MenTaLguY
 * SPDX-FileCopyrightText: © 2000-2003 Masatake YAMATO
 * SPDX-FileCopyrightText: © 2001-2002 Martin Weber
 * SPDX-FileCopyrightText: © 2012 Jon Ciesla
 * SPDX-FileCopyrightText: © 2017-2025 Peter Lemenkov
 * SPDX-FileCopyrightText: © 2020 Han Mertens
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include <setjmp.h>
#include <stdio.h>
#include <string.h>

#include "logreport.h"
#include <png.h>
#include "input-png.h"

static void handle_warning(png_structp png, const gchar *message)
{
  LOG("PNG warning: %s", message);
  at_exception_warning((at_exception_type *)png_get_error_ptr(png), message);
  /* at_exception_fatal((at_exception_type *)at_png->error_ptr,
     "PNG warning"); */
}

/* libpng's error callback must not return: if it does, libpng falls back to
   its default handler, which abort()s.  Record the error and jump back to
   the setjmp() in load_image().  */
static void handle_error(png_structp png, const gchar *message)
{
  LOG("PNG error: %s", message);
  at_exception_fatal((at_exception_type *)png_get_error_ptr(png), message);
  png_longjmp(png, 1);
}

static void finalize_structs(png_structp png, png_infop info, png_infop end_info)
{
  png_destroy_read_struct(png ? &png : NULL, info ? &info : NULL, end_info ? &end_info : NULL);
}

static int init_structs(png_structp *png, png_infop *info, png_infop *end_info,
                        at_exception_type *exp)
{
  *png = NULL;
  *info = *end_info = NULL;

  *png = png_create_read_struct(PNG_LIBPNG_VER_STRING, exp, (png_error_ptr)handle_error,
                                (png_error_ptr)handle_warning);

  if (*png) {
    *info = png_create_info_struct(*png);
    if (*info) {
      *end_info = png_create_info_struct(*png);
      if (*end_info)
        return 1;
    }
    finalize_structs(*png, *info, *end_info);
  }
  return 0;
}

#define CHECK_ERROR()                                                                              \
  do {                                                                                             \
    if (at_exception_got_fatal(exp)) {                                                             \
      result = 0;                                                                                  \
      goto cleanup;                                                                                \
    }                                                                                              \
  } while (0)

/* Read the header and ask libpng to hand us plain 8-bit gray or RGB rows,
   whatever the file stores.  */
static void set_up_transforms(png_structp png_ptr, png_infop info_ptr, at_input_opts_type *opts)
{
  png_color_16 my_bg;

  png_read_info(png_ptr, info_ptr);

  png_set_strip_16(png_ptr);
  png_set_packing(png_ptr);
  if ((png_get_bit_depth(png_ptr, info_ptr) < 8) ||
      (png_get_color_type(png_ptr, info_ptr) == PNG_COLOR_TYPE_PALETTE) ||
      (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS)))
    png_set_expand(png_ptr);

  if ((png_get_color_type(png_ptr, info_ptr) & PNG_COLOR_MASK_ALPHA) ||
      (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS))) {
    /* Composite transparent pixels against the requested background colour,
       or white when none was given.  The colour is expressed in the format
       libpng produces after the transformations above (8-bit gray or RGB),
       which is what need_expand = 0 means; see png_set_background() in the
       libpng manual. */
    my_bg.index = 0;

    if (opts && opts->background_color) {
      my_bg.red = opts->background_color->r;
      my_bg.green = opts->background_color->g;
      my_bg.blue = opts->background_color->b;
      my_bg.gray = at_color_luminance(opts->background_color);
    } else
      my_bg.red = my_bg.green = my_bg.blue = my_bg.gray = 0xFF;

    png_set_background(png_ptr, &my_bg, PNG_BACKGROUND_GAMMA_SCREEN, 0, 1.0);
  }
  png_set_interlace_handling(png_ptr);
  png_read_update_info(png_ptr, info_ptr);
}

static int load_image(at_bitmap *image, FILE *stream, at_input_opts_type *opts,
                      at_exception_type *exp)
{
  png_structp png;
  png_infop info, end_info;
  png_bytep *volatile rows = NULL;
  unsigned short width, height, row;
  int pixel_size;
  int result = 1;

  if (!init_structs(&png, &info, &end_info, exp))
    return 0;

  /* Any libpng error lands here via handle_error(); the exception has
     already been flagged fatal by then.  */
  if (setjmp(png_jmpbuf(png))) {
    result = 0;
    goto cleanup;
  }

  png_init_io(png, stream);
  CHECK_ERROR();

  set_up_transforms(png, info, opts);
  CHECK_ERROR();

  width = (unsigned short)png_get_image_width(png, info);
  height = (unsigned short)png_get_image_height(png, info);
  pixel_size = (png_get_color_type(png, info) == PNG_COLOR_TYPE_GRAY) ? 1 : 3;
  if (png_get_rowbytes(png, info) != (png_size_t)width * pixel_size) {
    at_exception_fatal(exp, "PNG: unexpected row layout");
    result = 0;
    goto cleanup;
  }

  /* Let libpng write each row straight into the bitmap.  */
  *image = at_bitmap_init(NULL, width, height, pixel_size);
  rows = g_new(png_bytep, height);
  for (row = 0; row < height; row++)
    rows[row] = AT_BITMAP_PIXEL(image, row, 0);
  png_read_image(png, rows);
  png_read_end(png, info);

cleanup:
  g_free(rows);
  finalize_structs(png, info, end_info);
  return result;
}

at_bitmap input_png_reader(gchar *filename, at_input_opts_type *opts, at_msg_func msg_func,
                           gpointer msg_data, gpointer user_data)
{
  FILE *stream;
  at_bitmap image = at_bitmap_init(0, 0, 0, 1);
  at_exception_type exp = at_exception_new(msg_func, msg_data);

  stream = fopen(filename, "rb");
  if (!stream) {
    LOG("Can't open \"%s\"\n", filename);
    at_exception_fatal(&exp, "Cannot open input png file");
    return image;
  }

  load_image(&image, stream, opts, &exp);
  fclose(stream);

  return image;
}
