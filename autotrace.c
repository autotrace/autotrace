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

#if HAVE_LIBPSTOEDIT 
#include "output-pstoedit.h"
#endif /* HAVE_LIBPSTOEDIT */  

#define AT_DEFAULT_DPI 72

at_fitting_opts_type *
at_fitting_opts_new(void)
{
  at_fitting_opts_type * opts;
  XMALLOC(opts, sizeof(at_fitting_opts_type));
  *opts = new_fitting_opts();
  return opts;
}

at_fitting_opts_type *
at_fitting_opts_copy (at_fitting_opts_type * original)
{
  at_fitting_opts_type * new_opts;
  if (original == NULL)
    return NULL;

  new_opts = at_fitting_opts_new ();
  *new_opts = *original;
  if (original->background_color)
    new_opts->background_color = at_color_copy(original->background_color);
  return new_opts;
}

void 
at_fitting_opts_free(at_fitting_opts_type * opts)
{
  if (opts->background_color != NULL)
    free (opts->background_color);
  free(opts);
}

at_input_opts_type *
at_input_opts_new(void)
{
  at_input_opts_type * opts;
  XMALLOC(opts, sizeof(at_input_opts_type));
  opts->background_color = NULL;
  return opts;
}

at_input_opts_type *
at_input_opts_copy(at_input_opts_type * original)
{
  at_input_opts_type * opts;
  opts = at_input_opts_new();
  if (original->background_color)
    opts->background_color = at_color_copy(original->background_color);
  return opts;
}

void
at_input_opts_free(at_input_opts_type * opts)
{
  if (opts->background_color != NULL)
    free (opts->background_color);
  free(opts);
}

at_output_opts_type *
at_output_opts_new(void)
{
  at_output_opts_type * opts;
  XMALLOC(opts, sizeof(at_output_opts_type));
  opts->dpi 	     = AT_DEFAULT_DPI;
  return opts;
}

at_output_opts_type *
at_output_opts_copy(at_output_opts_type * original)
{
  at_output_opts_type * opts =  at_output_opts_new();
  *opts = *original;
  return opts;
}

void
at_output_opts_free(at_output_opts_type * opts)
{
  free(opts);
}

at_bitmap_type *
at_bitmap_read (at_input_read_func input_reader,
		at_string filename,
		at_input_opts_type * opts,
		at_msg_func msg_func, at_address msg_data)
{
  at_bool new_opts = false;
  at_bitmap_type * bitmap;
  XMALLOC(bitmap, sizeof(at_bitmap_type)); 
  if (opts == NULL)
    {
      opts     = at_input_opts_new();
      new_opts = true;
    }
  *bitmap = (*input_reader) (filename, opts, msg_func, msg_data);
  if (new_opts)
    at_input_opts_free(opts);
  return bitmap;
}

at_bitmap_type *
at_bitmap_new(unsigned short width,
	      unsigned short height,
	      unsigned int planes)
{
  at_bitmap_type * bitmap;
  XMALLOC(bitmap, sizeof(at_bitmap_type)); 
  *bitmap = at_bitmap_init(NULL, width, height, planes);
  return bitmap;
}

at_bitmap_type *
at_bitmap_copy(at_bitmap_type * src)
{
  at_bitmap_type * dist;
  unsigned short width, height, planes;

  width  = at_bitmap_get_width(src);
  height = at_bitmap_get_height(src);
  planes = at_bitmap_get_planes(src);
    
  dist = at_bitmap_new(width, height, planes);
  memcpy(dist->bitmap, 
	 src->bitmap, 
	 width * height * planes * sizeof(unsigned char));
  return dist;
}

at_bitmap_type
at_bitmap_init(unsigned char * area,
	       unsigned short width,
	       unsigned short height,
	       unsigned int planes)
{
  at_bitmap_type bitmap;
  
  if (area)
    bitmap.bitmap = area;
  else
    {
      if (0 == (width * height))
	bitmap.bitmap = NULL;
      else
	XCALLOC(bitmap.bitmap, width * height * planes * sizeof(unsigned char));
    }

  bitmap.width 	  = width;
  bitmap.height   = height;
  bitmap.np       =  planes;
  return bitmap;  
}

void 
at_bitmap_free (at_bitmap_type * bitmap)
{
  free_bitmap (bitmap);
  free(bitmap);
}

unsigned short
at_bitmap_get_width (at_bitmap_type * bitmap)
{
  return bitmap->width;
}

unsigned short
at_bitmap_get_height (at_bitmap_type * bitmap)
{
  return bitmap->height;
}

unsigned short
at_bitmap_get_planes (at_bitmap_type * bitmap)
{
  return bitmap->np;
}

at_splines_type * 
at_splines_new (at_bitmap_type * bitmap,
		at_fitting_opts_type * opts,
		at_msg_func msg_func, at_address msg_data)
{
  return at_splines_new_full(bitmap, opts, 
			     msg_func, msg_data,
			     NULL, NULL, NULL, NULL);
}

/* at_splines_new_full modify its argument: BITMAP 
   when despeckle, quantize and/or thin_image are invoked. */
at_splines_type * 
at_splines_new_full (at_bitmap_type * bitmap,
		     at_fitting_opts_type * opts,
		     at_msg_func msg_func, 
		     at_address msg_data,
		     at_progress_func notify_progress,
		     at_address progress_data,
		     at_testcancel_func test_cancel,
		     at_address testcancel_data)
{
  image_header_type image_header;
  at_splines_type * splines = NULL;
  pixel_outline_list_type pixels;
  QuantizeObj *myQuant = NULL; /* curently not used */
  at_exception_type exp     = at_exception_new(msg_func, msg_data);
  distance_map_type dist_map, *dist = NULL;

#define CANCELP (test_cancel && test_cancel(testcancel_data))
#define FATALP  (at_exception_got_fatal(&exp))
#define FREE_SPLINE() do {if (splines) {at_splines_free(splines); splines = NULL;}} while(0)

#define CANCEL_THEN_CLEANUP_DIST() if (CANCELP) goto cleanup_dist;
#define CANCEL_THEN_CLEANUP_PIXELS() if (CANCELP) {FREE_SPLINE(); goto cleanup_pixels;}

#define FATAL_THEN_RETURN() if (FATALP) return splines;
#define FATAL_THEN_CLEANUP_DIST() if (FATALP) goto cleanup_dist;
#define FATAL_THEN_CLEANUP_PIXELS() if (FATALP) {FREE_SPLINE(); goto cleanup_pixels;}
  
  if (opts->despeckle_level > 0)
    {
      despeckle (bitmap, 
		 opts->despeckle_level, 
		 opts->despeckle_tightness,
		 &exp);
      FATAL_THEN_RETURN();
    }

  image_header.width = at_bitmap_get_width(bitmap);
  image_header.height = at_bitmap_get_height(bitmap);

  if (opts->color_count > 0)
    {
      quantize (bitmap, opts->color_count, opts->background_color, &myQuant, &exp);
      if (myQuant)
	quantize_object_free(myQuant); /* curently not used */
      FATAL_THEN_RETURN();
    }

  if (opts->centerline)
    {
      if (opts->preserve_width)
	{
          /* Preserve line width prior to thinning. */
          dist_map = new_distance_map(*bitmap, 255, /*padded=*/true, &exp);
          dist = &dist_map;
	  FATAL_THEN_RETURN();
        }
      /* Hereafter, dist is allocated. dist must be freed if 
	 the execution is canceled or exception is raised; 
	 use FATAL_THEN_CLEANUP_DIST. */
      thin_image (bitmap, opts->background_color, &exp);
      FATAL_THEN_CLEANUP_DIST()
    }

  /* Hereafter, pixels is allocated. pixels must be freed if 
     the execution is canceled; use CANCEL_THEN_CLEANUP_PIXELS. */
  if (opts->centerline)
    {
      color_type background_color = { 0xff, 0xff, 0xff };
      if (opts->background_color) 
        background_color = *opts->background_color;

      pixels = find_centerline_pixels(*bitmap, background_color, 
				      notify_progress, progress_data,
				      test_cancel, testcancel_data, &exp);
    }
  else
    pixels = find_outline_pixels(*bitmap, opts->background_color, 
				 notify_progress, progress_data,
				 test_cancel, testcancel_data, &exp);
  FATAL_THEN_CLEANUP_DIST();
  CANCEL_THEN_CLEANUP_DIST();
  
  XMALLOC(splines, sizeof(at_splines_type)); 
  *splines = fitted_splines (pixels, opts, dist,
			     image_header.width,
			     image_header.height,
			     &exp,
			     notify_progress, progress_data,
			     test_cancel, testcancel_data);
  FATAL_THEN_CLEANUP_PIXELS();
  CANCEL_THEN_CLEANUP_PIXELS();
  
  if (notify_progress)
    notify_progress(1.0, progress_data);

 cleanup_pixels:
  free_pixel_outline_list (&pixels);
 cleanup_dist:
  if (dist)
    free_distance_map (dist);
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

void 
at_splines_write (at_output_write_func output_writer,
		  FILE * writeto,
		  at_string file_name,
		  at_output_opts_type * opts,
		  at_splines_type * splines,
		  at_msg_func msg_func, at_address msg_data)
{
  at_bool new_opts = false;
  int llx, lly, urx, ury;
  llx = 0;
  lly = 0;
  urx = splines->width;
  ury = splines->height;

  if (!file_name)
    file_name = "";
  
  if (opts == NULL)
    {
      new_opts = true;
      opts     = at_output_opts_new();
    }
#if HAVE_LIBPSTOEDIT 
  if (output_pstoedit_is_writer(output_writer))
    output_pstoedit_invoke_writer (output_writer, 
				   writeto, file_name,
				   llx, lly, urx, ury,
				   opts,
				   *splines,
				   msg_func, msg_data);
  else
    (*output_writer) (writeto, file_name, llx, lly, urx, ury, opts, *splines,
		      msg_func, msg_data);
#else 
  (*output_writer) (writeto, file_name, llx, lly, urx, ury, opts, *splines,
		    msg_func, msg_data);
#endif /* HAVE_LIBPSTOEDIT */  
  if (new_opts)
    at_output_opts_free(opts);
}

void 
at_splines_free (at_splines_type * splines)
{
  free_spline_list_array (splines);
  if (splines->background_color)
    at_color_free(splines->background_color);
  free(splines);
}

at_color_type * 
at_color_new (unsigned char r, 
	      unsigned char g,
	      unsigned char b)
{
  at_color_type * color;
  XMALLOC (color, sizeof (at_color_type));
  color->r = r;
  color->g = g;
  color->b = b;
  return color;
}

at_color_type *
at_color_copy (at_color_type * original)
{
  if (original == NULL)
    return NULL;
  return at_color_new(original->r, 
		      original->g, 
		      original->b);
}

at_bool
at_color_equal (at_color_type * c1, at_color_type * c2)
{
  if (c1 == c2)
    return true;
  else 
    return (COLOR_EQUAL(*c1, *c2));
}

void 
at_color_free(at_color_type * color)
{
  free(color);
}

const char *
at_version (at_bool long_format)
{
  if (long_format)
    return "AutoTrace version " AUTOTRACE_VERSION;
  else
    return AUTOTRACE_VERSION;
}

const char * 
at_home_site (void)
{
  return AUTOTRACE_WEB;
}
