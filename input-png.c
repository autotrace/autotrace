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

#include "bitmap.h"
#include "message.h"
#include "xmem.h"
#include <png.h>
#include "input-png.h"

static volatile char rcsid[]="$Id: input-png.c,v 1.1 2000/10/11 14:00:26 masata-y Exp $";

/* for pre-1.0.6 versions of libpng */
#ifndef png_jmpbuf
#	define png_jmpbuf(png_ptr) (png_ptr)->jmpbuf
#endif

static void handle_warning(png_structp png, const char *message) {
	WARNING1("PNG warning: %s", message);
}

static void handle_error(png_structp png, const char *message) {
	FATAL1("PNG error: %s", message);
}

static void finalize_structs(png_structp png, png_infop info,
                             png_infop end_info)
{
	png_destroy_read_struct(png ? &png : NULL,
	                        info ? &info : NULL,
	                        end_info ? &end_info : NULL);
}

static int init_structs(png_structp *png, png_infop *info,
                        png_infop *end_info)
{
	*png = NULL;
	*info = *end_info = NULL;

	*png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL,
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

static int load_image(bitmap_type *image, FILE *stream) {
	png_structp png;
	png_infop info, end_info;
	png_bytep *rows;
	unsigned long width, height, row;
	int pixel_size;

	if (!init_structs(&png, &info, &end_info)) return 0;

	png_init_io(png, stream);
	png_read_png(png, info, PNG_TRANSFORM_STRIP_16
	                      | PNG_TRANSFORM_STRIP_ALPHA
	                      | PNG_TRANSFORM_PACKING
	                      | PNG_TRANSFORM_EXPAND, NULL);
	
	rows = png_get_rows(png, info);

	width = png_get_image_width(png, info);
	height = png_get_image_height(png, info);
	if ( png_get_color_type == PNG_COLOR_TYPE_GRAY ) {
		pixel_size = 1;
	} else {
		pixel_size = 3;
	}

	XMALLOC(BITMAP_BITS(*image),
	        width * height * pixel_size * sizeof(unsigned char)); 
	BITMAP_WIDTH(*image) = width;	
	BITMAP_HEIGHT(*image) = height;
	BITMAP_PLANES(*image) = pixel_size;
	for ( row = 0 ; row < height ; row++, rows++ ) {
		memcpy(BITMAP_PIXEL(*image, row, 0), *rows,
		       width * pixel_size * sizeof(unsigned char));
	}

	finalize_structs(png, info, end_info);

	return 1;
}

bitmap_type png_load_image(string filename) {
	FILE *stream;
	bitmap_type image;

	stream = fopen(filename, "rb");
	if (!stream) FATAL1("Can't open \"%s\"\n", filename);

	if (!load_image(&image, stream)) {
		FATAL1("Error loading PNG image from \"%s\"\n", filename);
	}

	fclose(stream);

	return image;
}
