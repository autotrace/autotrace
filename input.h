/* input.h: input routines
   Copyright (C) 1999 Bernhard Herzog. */

#ifndef INPUT_H
#define INPUT_H 

#include "types.h"
#include "bitmap.h"

#if HAVE_LIBPNG
#define PNG_SUFFIX "png, "
#else 
#define PNG_SUFFIX ""
#endif /* HAVE_LIBPNG */

#if HAVE_MAGICK
#define MAGICK_SUFFIX "magick, "
#else 
#define MAGICK_SUFFIX ""
#endif /* HAVE_MAGICK */

#define INPUT_SUFFIX_LIST PNG_SUFFIX MAGICK_SUFFIX "tga, pbm, pnm, pgm, ppm or bmp"

typedef bitmap_type (*input_read) (string name);
input_read input_get_handler (string filename);
input_read input_get_handler_by_suffix (string suffix);
char ** input_list (void);

#endif /* Not def: INPUT_H */
