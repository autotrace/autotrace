/* autotrace.c: Library interface. */

#include "autotrace.h"
#include "exception.h"

#include "fit.h"
#include "bitmap.h"
#include "spline.h"

#include "input.h"

#include "xstd.h"
#include "image-header.h"
#include "quantize.h"
#include "thin-image.h"
#include "despeckle.h"

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

at_bitmap_type *
at_bitmap_read (at_input_read_func input_reader,
		at_string filename,
		at_msg_func msg_func, at_address msg_data)
{
  at_bitmap_type * bitmap;
  XMALLOC(bitmap, sizeof(at_bitmap_type)); 
  *bitmap = (*input_reader) (filename, msg_func, msg_data);
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
   when despeckle, quantize and thin_image are invoked. */
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
  at_exception exp     = at_exception_new(msg_func, msg_data);

#define CANCELP (test_cancel && test_cancel(testcancel_data))
#define CANCEL_THEN_RETURN() if (CANCELP) return splines;
#define CANCEL_THEN_CLEANUP() if (CANCELP) goto cleanup;
  if (opts->despeckle_level > 0)
    {
      despeckle (bitmap, 
		 opts->despeckle_level, 
		 opts->despeckle_tightness,
		 &exp);
      if (at_exception_got_fatal(&exp))
	return splines;
    }
  CANCEL_THEN_RETURN();
  
  image_header.width = at_bitmap_get_width(bitmap);
  image_header.height = at_bitmap_get_height(bitmap);

  if (opts->color_count > 0)
    {
      quantize (bitmap, opts->color_count, opts->background_color, &myQuant, &exp);
      if (at_exception_got_fatal(&exp))
	return splines;
    }
  CANCEL_THEN_RETURN();

  if (opts->centerline)
    {
      thin_image (bitmap, opts->background_color, &exp);
      if (at_exception_got_fatal(&exp))
	return splines;
    }
  CANCEL_THEN_RETURN();

  /* Hereafter, pixels is allocated. pixels must be freed if 
     the execution is canceled; use CANCEL_THEN_CLEANUP. */
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
  if (at_exception_got_fatal(&exp))
    /* TODO: splines is empty? */
    goto cleanup;
  CANCEL_THEN_CLEANUP();

  
  XMALLOC(splines, sizeof(at_splines_type)); 
  *splines = fitted_splines (pixels, opts,
			     image_header.width,
			     image_header.height,
			     &exp,
			     notify_progress, progress_data,
			     test_cancel, testcancel_data);
  if (at_exception_got_fatal(&exp))
    /* TODO: splines is empty? */
    goto cleanup;

  if (CANCELP)
    {
      at_splines_free (splines);
      splines = NULL;
      goto cleanup;
    }
  
  if (notify_progress)
    notify_progress(1.0, progress_data);
  
 cleanup:
  free_pixel_outline_list (&pixels);
  return splines;
#undef CANCEL_THEN_CLEANUP
#undef CANCEL_THEN_RETURN
#undef CANCELP
}

void 
at_splines_write(at_splines_type * splines,
		 FILE * writeto,
		 at_string name,
		 int dpi,
		 at_output_write_func output_writer,
		 at_msg_func msg_func, 
		 at_address msg_data)
{
  int llx, lly, urx, ury;
  llx = 0;
  lly = 0;
  urx = splines->width;
  ury = splines->height;

  if (!name)
    name = "";
  (*output_writer) (writeto, name, llx, lly, urx, ury, dpi, *splines,
		    msg_func, msg_data);
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
