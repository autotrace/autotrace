/* autotrace.c --- Autotrace API

  Copyright (C) 2000, 2001, 2002 Martin Weber

  The author can be contacted at <martweb@gmx.net>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA. */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* Def: HAVE_CONFIG_H */
#include "intl.h"

#include "private.h"

#include "autotrace.h"
#include "exception.h"

#include "fit.h"
#include "bitmap.h"
#include "spline.h"

#include "input.h"

#include "xstd.h"
#include "image-header.h"
#include "image-proc.h"
#include "quantize.h"
#include "thin-image.h"
#include "despeckle.h"

#include <locale.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>

#define AT_DEFAULT_DPI 72

at_fitting_opts_type *at_fitting_opts_new(void)
{
  at_fitting_opts_type *opts;
  XMALLOC(opts, sizeof(at_fitting_opts_type));
  *opts = new_fitting_opts();
  return opts;
}

at_fitting_opts_type *at_fitting_opts_copy(at_fitting_opts_type * original)
{
  at_fitting_opts_type *new_opts;
  if (original == NULL)
    return NULL;

  new_opts = at_fitting_opts_new();
  *new_opts = *original;
  if (original->background_color)
    new_opts->background_color = at_color_copy(original->background_color);
  return new_opts;
}

void at_fitting_opts_free(at_fitting_opts_type * opts)
{
  free(opts->background_color);
  free(opts);
}

at_input_opts_type *at_input_opts_new(void)
{
  at_input_opts_type *opts;
  XMALLOC(opts, sizeof(at_input_opts_type));
  opts->background_color = NULL;
  opts->charcode = 0;
  return opts;
}

at_input_opts_type *at_input_opts_copy(at_input_opts_type * original)
{
  at_input_opts_type *opts;
  opts = at_input_opts_new();
  *opts = *original;
  if (original->background_color)
    opts->background_color = at_color_copy(original->background_color);
  return opts;
}

void at_input_opts_free(at_input_opts_type * opts)
{
  free(opts->background_color);
  free(opts);
}

at_output_opts_type *at_output_opts_new(void)
{
  at_output_opts_type *opts;
  XMALLOC(opts, sizeof(at_output_opts_type));
  opts->dpi = AT_DEFAULT_DPI;
  return opts;
}

at_output_opts_type *at_output_opts_copy(at_output_opts_type * original)
{
  at_output_opts_type *opts = at_output_opts_new();
  *opts = *original;
  return opts;
}

void at_output_opts_free(at_output_opts_type * opts)
{
  free(opts);
}

at_bitmap *at_bitmap_read(at_bitmap_reader * reader, gchar * filename, at_input_opts_type * opts, at_msg_func msg_func, gpointer msg_data)
{
  gboolean new_opts = FALSE;
  at_bitmap *bitmap;
  XMALLOC(bitmap, sizeof(at_bitmap));
  if (opts == NULL) {
    opts = at_input_opts_new();
    new_opts = TRUE;
  }
  *bitmap = (*reader->func) (filename, opts, msg_func, msg_data, reader->data);
  if (new_opts)
    at_input_opts_free(opts);
  return bitmap;
}

at_bitmap *at_bitmap_new(unsigned short width, unsigned short height, unsigned int planes)
{
  at_bitmap *bitmap;
  XMALLOC(bitmap, sizeof(at_bitmap));
  *bitmap = at_bitmap_init(NULL, width, height, planes);
  return bitmap;
}

at_bitmap *at_bitmap_copy(const at_bitmap * src)
{
  at_bitmap *dist;
  unsigned short width, height, planes;

  width = at_bitmap_get_width(src);
  height = at_bitmap_get_height(src);
  planes = at_bitmap_get_planes(src);

  dist = at_bitmap_new(width, height, planes);
  memcpy(dist->bitmap, src->bitmap, width * height * planes * sizeof(unsigned char));
  return dist;
}

at_bitmap at_bitmap_init(unsigned char *area, unsigned short width, unsigned short height, unsigned int planes)
{
  at_bitmap bitmap;

  if (area)
    bitmap.bitmap = area;
  else {
    if (0 == (width * height))
      bitmap.bitmap = NULL;
    else
      XCALLOC(bitmap.bitmap, width * height * planes * sizeof(unsigned char));
  }

  bitmap.width = width;
  bitmap.height = height;
  bitmap.np = planes;
  return bitmap;
}

void at_bitmap_free(at_bitmap * bitmap)
{
  free(AT_BITMAP_BITS(bitmap));
  free(bitmap);
}

unsigned short at_bitmap_get_width(const at_bitmap * bitmap)
{
  return bitmap->width;
}

unsigned short at_bitmap_get_height(const at_bitmap * bitmap)
{
  return bitmap->height;
}

unsigned short at_bitmap_get_planes(const at_bitmap * bitmap)
{
  /* Here we use cast rather changing the type definition of
     at_bitmap::np to keep binary compatibility. */
  return (unsigned short)bitmap->np;
}

void at_bitmap_get_color(const at_bitmap * bitmap, unsigned int row, unsigned int col, at_color * color)
{
  unsigned char *p;
  g_return_if_fail(color);
  g_return_if_fail(bitmap);

  p = AT_BITMAP_PIXEL(bitmap, row, col);
  if (at_bitmap_get_planes(bitmap) >= 3)
    at_color_set(color, p[0], p[1], p[2]);
  else
    at_color_set(color, p[0], p[0], p[0]);
}

gboolean at_bitmap_equal_color(const at_bitmap * bitmap, unsigned int row, unsigned int col, at_color * color)
{
  at_color c;

  g_return_val_if_fail(bitmap, FALSE);
  g_return_val_if_fail(color, FALSE);

  at_bitmap_get_color(bitmap, row, col, &c);
  return at_color_equal(&c, color);
}

at_splines_type *at_splines_new(at_bitmap * bitmap, at_fitting_opts_type * opts, at_msg_func msg_func, gpointer msg_data)
{
  return at_splines_new_full(bitmap, opts, msg_func, msg_data, NULL, NULL, NULL, NULL);
}

/* at_splines_new_full modify its argument: BITMAP
   when despeckle, quantize and/or thin_image are invoked. */
at_splines_type *at_splines_new_full(at_bitmap * bitmap, at_fitting_opts_type * opts, at_msg_func msg_func, gpointer msg_data, at_progress_func notify_progress, gpointer progress_data, at_testcancel_func test_cancel, gpointer testcancel_data)
{
  image_header_type image_header;
  at_splines_type *splines = NULL;
  pixel_outline_list_type pixels;
  QuantizeObj *myQuant = NULL;  /* curently not used */
  at_exception_type exp = at_exception_new(msg_func, msg_data);
  at_distance_map dist_map, *dist = NULL;

#define CANCELP (test_cancel && test_cancel(testcancel_data))
#define FATALP  (at_exception_got_fatal(&exp))
#define FREE_SPLINE() do {if (splines) {at_splines_free(splines); splines = NULL;}} while(0)

#define CANCEL_THEN_CLEANUP_DIST() if (CANCELP) goto cleanup_dist;
#define CANCEL_THEN_CLEANUP_PIXELS() if (CANCELP) {FREE_SPLINE(); goto cleanup_pixels;}

#define FATAL_THEN_RETURN() if (FATALP) return splines;
#define FATAL_THEN_CLEANUP_DIST() if (FATALP) goto cleanup_dist;
#define FATAL_THEN_CLEANUP_PIXELS() if (FATALP) {FREE_SPLINE(); goto cleanup_pixels;}

  if (opts->despeckle_level > 0) {
    despeckle(bitmap, opts->despeckle_level, opts->despeckle_tightness, opts->noise_removal, &exp);
    FATAL_THEN_RETURN();
  }

  image_header.width = at_bitmap_get_width(bitmap);
  image_header.height = at_bitmap_get_height(bitmap);

  if (opts->color_count > 0) {
    quantize(bitmap, opts->color_count, opts->background_color, &myQuant, &exp);
    if (myQuant)
      quantize_object_free(myQuant);  /* curently not used */
    FATAL_THEN_RETURN();
  }

  if (opts->centerline) {
    if (opts->preserve_width) {
      /* Preserve line width prior to thinning. */
      dist_map = new_distance_map(bitmap, 255, /*padded= */ TRUE, &exp);
      dist = &dist_map;
      FATAL_THEN_RETURN();
    }
    /* Hereafter, dist is allocated. dist must be freed if
       the execution is canceled or exception is raised;
       use FATAL_THEN_CLEANUP_DIST. */
    thin_image(bitmap, opts->background_color, &exp);
    FATAL_THEN_CLEANUP_DIST()
  }

  /* Hereafter, pixels is allocated. pixels must be freed if
     the execution is canceled; use CANCEL_THEN_CLEANUP_PIXELS. */
  if (opts->centerline) {
    at_color background_color = { 0xff, 0xff, 0xff };
    if (opts->background_color)
      background_color = *opts->background_color;

    pixels = find_centerline_pixels(bitmap, background_color, notify_progress, progress_data, test_cancel, testcancel_data, &exp);
  } else
    pixels = find_outline_pixels(bitmap, opts->background_color, notify_progress, progress_data, test_cancel, testcancel_data, &exp);
  FATAL_THEN_CLEANUP_DIST();
  CANCEL_THEN_CLEANUP_DIST();

  XMALLOC(splines, sizeof(at_splines_type));
  *splines = fitted_splines(pixels, opts, dist, image_header.width, image_header.height, &exp, notify_progress, progress_data, test_cancel, testcancel_data);
  FATAL_THEN_CLEANUP_PIXELS();
  CANCEL_THEN_CLEANUP_PIXELS();

  if (notify_progress)
    notify_progress(1.0, progress_data);

cleanup_pixels:
  free_pixel_outline_list(&pixels);
cleanup_dist:
  if (dist)
    free_distance_map(dist);
  return splines;
#undef CANCELP
#undef FATALP
#undef FREE_SPLINE
#undef CANCEL_THEN_CLEANUP_DIST
#undef CANCEL_THEN_CLEANUP_PIXELS

#undef FATAL_THEN_RETURN
#undef FATAL_THEN_CLEANUP_DIST
#undef FATAL_THEN_CLEANUP_PIXELS

}

void at_splines_write(at_spline_writer * writer, FILE * writeto, gchar * file_name, at_output_opts_type * opts, at_splines_type * splines, at_msg_func msg_func, gpointer msg_data)
{
  gboolean new_opts = FALSE;
  int llx, lly, urx, ury;
  llx = 0;
  lly = 0;
  urx = splines->width;
  ury = splines->height;

  if (!file_name)
    file_name = "";

  if (opts == NULL) {
    new_opts = TRUE;
    opts = at_output_opts_new();
  }

  setlocale(LC_NUMERIC, "C");
  (*writer->func) (writeto, file_name, llx, lly, urx, ury, opts, *splines, msg_func, msg_data, writer->data);
  if (new_opts)
    at_output_opts_free(opts);
}

void at_splines_free(at_splines_type * splines)
{
  free_spline_list_array(splines);
  if (splines->background_color)
    at_color_free(splines->background_color);
  free(splines);
}

const char *at_version(gboolean long_format)
{
  if (long_format)
    return "AutoTrace version " AUTOTRACE_VERSION;

  return AUTOTRACE_VERSION;
}

const char *at_home_site(void)
{
  return AUTOTRACE_WEB;
}

void autotrace_init(void)
{
  static int initialized = 0;
  if (!initialized) {
#ifdef ENABLE_NLS
    setlocale(LC_ALL, "");
    bindtextdomain(PACKAGE, LOCALEDIR);
#endif /* Def: ENABLE_NLS */

    /* Initialize subsystems */
    at_input_init();
    at_output_init();
    at_module_init();

    initialized = 1;
  }
}

const char *at_fitting_opts_doc_func(char *string)
{
  return _(string);
}
