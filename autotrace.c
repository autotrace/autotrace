#include "autotrace.h"
#include "xstd.h"
#include "image-header.h"
#include "quantize.h"
#include "thin-image.h"

at_fitting_opts_type *
at_fitting_opts_new(void)
{
  at_fitting_opts_type * opts;
  XMALLOC(opts, sizeof(at_fitting_opts_type));
  *opts = new_fitting_opts();
  return opts;
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
  return DIMENSIONS_WIDTH (bitmap->dimensions);
}

unsigned short
at_bitmap_get_height (at_bitmap_type * bitmap)
{
  return DIMENSIONS_HEIGHT (bitmap->dimensions);
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

  if (opts->thin) 
    thin_image (bitmap); 
  
  if (opts->color_count > 0 && BITMAP_PLANES(*bitmap)== 3)
    quantize (bitmap->bitmap, 
	      bitmap->bitmap, 
	      image_header.width,
	      image_header.height,
	      opts->color_count,
	      opts->bgColor, &myQuant);
  
  pixels = find_outline_pixels (*bitmap);
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

char *
at_version ()
{
  return VERSION;
}
