/*
    input-png.c: PNG loader for autotrace
    Copyright (C) 2000 MenTaLguY <mental@rydia.net>

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
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111 USA
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "types.h"
#include "bitmap.h"
#include "message.h"
#include "xstd.h"
#include <png.h>
#include "input-png.h"

static volatile char rcsid[]="$Id: input-png.c,v 1.8 2002/01/14 17:27:33 masata-y Exp $";

/* for pre-1.0.6 versions of libpng */
#ifndef png_jmpbuf
#	define png_jmpbuf(png_ptr) (png_ptr)->jmpbuf
#endif

static void handle_warning(png_structp png, const char *message) {
        LOG1("PNG warning: %s", message);
	at_exception_warning((at_exception *)png->error_ptr,
			     message);
	/* at_exception_fatal((at_exception *)at_png->error_ptr,
	   "PNG warning"); */
}

static void handle_error(png_structp png, const char *message) {
	LOG1("PNG error: %s", message);
	at_exception_fatal((at_exception *)png->error_ptr,
			   message);
	/* at_exception_fatal((at_exception *)at_png->error_ptr,
	   "PNG error"); */
	
}

static void finalize_structs(png_structp png, png_infop info,
                             png_infop end_info)
{
	png_destroy_read_struct(png ? &png : NULL,
	                        info ? &info : NULL,
	                        end_info ? &end_info : NULL);
}

static int init_structs(png_structp *png, png_infop *info,
                        png_infop *end_info, at_exception * exp)
{
	*png = NULL;
	*info = *end_info = NULL;

	*png = png_create_read_struct(PNG_LIBPNG_VER_STRING, exp,
	                              handle_error, handle_warning);
	
	if (*png) {
		*info = png_create_info_struct(*png);
		if (*info) {
			*end_info = png_create_info_struct(*png);
			if (*end_info) return 1;
		}
		finalize_structs(*png, *info, *end_info);
	}
	return 0;
}

#define CHECK_ERROR() 	do { if (at_exception_got_fatal(exp))	\
	  {							\
	    result = 0;						\
	    goto cleanup;					\
	  } } while (0)

static int load_image(at_bitmap_type *image, FILE *stream, at_exception * exp) {
	png_structp png;
	png_infop info, end_info;
	png_bytep *rows;
	unsigned long width, height, row;
	int pixel_size;
	int result = 1;
	
	if (!init_structs(&png, &info, &end_info, exp)) 
	  return 0;

	png_init_io(png, stream);
	CHECK_ERROR();

	png_read_png(png, info, PNG_TRANSFORM_STRIP_16
	                      | PNG_TRANSFORM_STRIP_ALPHA
	                      | PNG_TRANSFORM_PACKING
	                      | PNG_TRANSFORM_EXPAND, NULL);
	CHECK_ERROR();

	rows = png_get_rows(png, info);

	width = png_get_image_width(png, info);
	height = png_get_image_height(png, info);
	if ( png_get_color_type(png, info) == PNG_COLOR_TYPE_GRAY ) {
		pixel_size = 1;
	} else {
		pixel_size = 3;
	}

	*image = at_bitmap_init(NULL, width, height, pixel_size);
	for ( row = 0 ; row < height ; row++, rows++ ) {
		memcpy(AT_BITMAP_PIXEL(*image, row, 0), *rows,
		       width * pixel_size * sizeof(unsigned char));
	}
 cleanup:
	finalize_structs(png, info, end_info);
	return result;
}

at_bitmap_type png_load_image(at_string filename, 
			      at_msg_func msg_func, at_address msg_data) {
	FILE *stream;
	at_bitmap_type image = at_bitmap_init(0, 0, 0, 1);
	at_exception exp = at_exception_new(msg_func, msg_data);

	stream = fopen(filename, "rb");
	if (!stream)
	  {
	    LOG1("Can't open \"%s\"\n", filename);
	    at_exception_fatal(&exp, "Cannot open input png file");
	    return image;
	  }

	load_image(&image, stream, &exp);
	fclose(stream);

	return image;
}
