/* module.c --- Autotrace plugin module management subsystem

  Copyright (C) 2003 Martin Weber
  Copyright (C) 2003 Masatake YAMATO
  
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

#include "input.h"
#include "input-pnm.h"
#include "input-bmp.h"
#include "input-tga.h"
#ifdef HAVE_LIBPNG
#include "input-png.h"
#endif /* HAVE_LIBPNG */
#if HAVE_MAGICK
#include <sys/types.h> /* Needed for correct interpretation of magick/api.h */
#include <magick/api.h>
#include "input-magick.h"
#endif /* HAVE_MAGICK */

int
at_module_init (void)
{     
  /* TODO: Loading every thing in dynamic.
   For a while, these are staticly added. */
  
#ifdef HAVE_LIBPNG  
  at_input_add_handler      ("PNG", "Portable network graphics",      input_png_reader);
#endif
  at_input_add_handler      ("TGA", "Truevision Targa image",         input_tga_reader);
  at_input_add_handler      ("BMP", "Microsoft Windows bitmap image", input_bmp_reader);

  at_input_add_handler_full ("PBM", "Portable bitmap format",  input_pnm_reader, 0, "PBM", NULL);
  at_input_add_handler_full ("PNM", "Portable anymap format",  input_pnm_reader, 0, "PNM", NULL);
  at_input_add_handler_full ("PGM", "Portable graymap format", input_pnm_reader, 0, "PGM", NULL);
  at_input_add_handler_full ("PPM", "Portable pixmap format",  input_pnm_reader, 0, "PPM", NULL);


#if HAVE_MAGICK  

#if (MagickLibVersion < 0x0534)
#define AT_MAGICK_SET_INFO(X) X = GetMagickInfo(NULL)
#else  /* (MagickLibVersion < 0x0534) */
#define AT_MAGICK_SET_INFO(X)			\
  do {						\
    X = GetMagickInfo(NULL, &exception);	\
    if (X && !X->next)				\
      X = GetMagickInfo("*", &exception);	\
  } while (0)
#endif	/* (MagickLibVersion < 0x0534) */

#if (MagickLibVersion < 0x0537)
#define AT_MAGICK_SUFFIX_FIELD_NAME tag
#else  /* (MagickLibVersion < 0x0537) */
#define AT_MAGICK_SUFFIX_FIELD_NAME name
#endif	/* (MagickLibVersion < 0x0537) */

#if (MagickLibVersion < 0x0538)
#define AT_MAGICK_INITIALIZER()     MagickIncarnate("")
#else  /* (MagickLibVersion < 0x0538) */
#define AT_MAGICK_INITIALIZER()     InitializeMagick("")
#endif	/* (MagickLibVersion < 0x0538) */


#if (MagickLibVersion < 0x0540)
#define AT_MAGICK_INFO_TYPE_MODIFIER     const
#else  /* (MagickLibVersion < 0x0540) */
#define AT_MAGICK_INFO_TYPE_MODIFIER 
#endif	/* (MagickLibVersion < 0x0540)*/

  {
    ExceptionInfo exception;

    AT_MAGICK_INFO_TYPE_MODIFIER MagickInfo *info, *magickinfo;
    AT_MAGICK_INITIALIZER() ;

    GetExceptionInfo(&exception);

    AT_MAGICK_SET_INFO(info);
    magickinfo = info;

    while (info)
      {
	if (info->AT_MAGICK_SUFFIX_FIELD_NAME && info->description)
	  at_input_add_handler_full(info->AT_MAGICK_SUFFIX_FIELD_NAME,
				    info->description,
				    input_magick_reader,
				    0,
				    info->AT_MAGICK_SUFFIX_FIELD_NAME,
				    NULL);
	info = info->next ;
      }
  }
#endif
  return 0;
}
