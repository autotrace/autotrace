/* autotrace.c: Library interface. */

#include "autotrace.h"

#include "fit.h"
#include "bitmap.h"
#include "input.h"
#include "output.h"
#include "spline.h"

#include "xstd.h"
#include "image-header.h"
#include "quantize.h"
#include "thin-image.h"

bool at_centerline = false;

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
  if (original->bgColor)
    new_opts->bgColor = at_color_copy(original->bgColor);
  return new_opts;
}

void 
at_fitting_opts_free(at_fitting_opts_type * opts)
{
  if (opts->bgColor != NULL)
    free (opts->bgColor);
  free(opts);
}

at_input_read_func
at_input_get_handler (string filename)
{
  return input_get_handler(filename);
}

at_input_read_func
at_input_get_handler_by_suffix (string suffix)
{
  return input_get_handler_by_suffix(suffix);
}

char **
at_input_list_new ()
{
  return input_list();
}
void
at_input_list_free(char ** list)
{
  free(list);
}

at_bitmap_type *
at_bitmap_new (at_input_read_func input_reader,
	       string filename)
{
  at_bitmap_type * bitmap;
  XMALLOC(bitmap, sizeof(at_bitmap_type)); 
  *bitmap = (*input_reader) (filename);
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
at_splines_type * 
at_splines_new (at_bitmap_type * bitmap,
		at_fitting_opts_type * opts)
{
  image_header_type image_header;
  at_splines_type * splines;
  pixel_outline_list_type pixels;
  QuantizeObj *myQuant = NULL; /* curently not used */

  image_header.width = at_bitmap_get_width(bitmap);
  image_header.height = at_bitmap_get_height(bitmap);

  if (at_centerline)
    opts->thin = true;

  if (opts->color_count > 0)
    quantize (bitmap, opts->color_count, opts->bgColor, &myQuant);
  
  if (opts->thin) 
    thin_image (bitmap, opts->bgColor); 
  
  if (at_centerline)
  {
    color_type bg_color = { 0xff, 0xff, 0xff };
    if (opts->bgColor) bg_color = *opts->bgColor;

    pixels = find_centerline_pixels(*bitmap, bg_color);
  }
  else
    pixels = find_outline_pixels(*bitmap, opts->bgColor);

  XMALLOC(splines, sizeof(at_splines_type)); 
  *splines = fitted_splines (pixels, opts);
  free_pixel_outline_list (&pixels);
  return splines;
}

void 
at_splines_free (at_splines_type * splines)
{
  free_spline_list_array (splines); 
  free(splines);
}
/* TODO internal data access */

at_output_write_func 
at_output_get_handler (string suffix)
{
  return output_get_handler(suffix);
}

void 
at_output_write(at_output_write_func output_writer,
		FILE * writeto,
		char * name,
		int llx, int lly, int urx, int ury,
		at_splines_type * splines)
{
  if (!name)
    name = "";
  (*output_writer) (writeto, name, llx, lly, urx, ury, *splines);
}

char **
at_output_list_new ()
{
  return output_list();
}

void
at_output_list_free(char ** list)
{
  free(list);
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
at_version ()
{
  return VERSION;
}

const char * 
at_home_site ()
{
  /* TODO: This should be defined in configure.in
     -- Masatake */
  return "http://autotrace.sourceforge.net";
}
