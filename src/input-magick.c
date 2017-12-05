/* input-magick.c: import files via image magick

   Copyright (C) 1999, 2000, 2001 Martin Weber.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public License
   as published by the Free Software Foundation; either version 2.1 of
   the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with this library; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
   USA. */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* Def: HAVE_CONFIG_H */

#include "input.h"
#include "input-magick.h"
#include "bitmap.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>          /* Needed for correct interpretation of magick/api.h */
#include <magick/api.h>

static at_bitmap input_magick_reader(gchar * filename, at_input_opts_type * opts, at_msg_func msg_func, gpointer msg_data, gpointer user_data)
{
  Image *image = NULL;
  ImageInfo *image_info;
  ImageType image_type;
  unsigned int i, j, point, np, runcount;
  at_bitmap bitmap;
  PixelPacket p;
  PixelPacket *pixel = &p;
  ExceptionInfo exception;
  InitializeMagick("");
  GetExceptionInfo(&exception);
  image_info = CloneImageInfo((ImageInfo *) NULL);
  (void)strcpy(image_info->filename, filename);
  image_info->antialias = 0;

  image = ReadImage(image_info, &exception);
  if (image == (Image *) NULL) {
    /* MagickError(exception.severity,exception.reason,exception.description); */
    if (msg_func)
      msg_func(exception.reason, AT_MSG_FATAL, msg_data);
    goto cleanup;
  }
  image_type = GetImageType(image, &exception);
  if (image_type == BilevelType || image_type == GrayscaleType)
    np = 1;
  else
    np = 3;

  bitmap = at_bitmap_init(NULL, image->columns, image->rows, np);

  for (j = 0, runcount = 0, point = 0; j < image->rows; j++)
    for (i = 0; i < image->columns; i++) {
#if (MagickLibVersion < 0x0645) || (MagickLibVersion >= 0x0649)
      p = GetOnePixel(image, i, j);
#else
      GetOnePixel(image, i, j, pixel);
#endif
      AT_BITMAP_BITS(&bitmap)[point++] = pixel->red;  /* if gray: red=green=blue */
      if (np == 3) {
        AT_BITMAP_BITS(&bitmap)[point++] = pixel->green;
        AT_BITMAP_BITS(&bitmap)[point++] = pixel->blue;
      }
    }

  DestroyImage(image);
cleanup:
  DestroyImageInfo(image_info);
  return (bitmap);
}

int install_input_magick_readers(void)
{
  ExceptionInfo exception;
  MagickInfo *info, *magickinfo;
  InitializeMagick("");

  GetExceptionInfo(&exception);

  info = GetMagickInfo(NULL, &exception);
  if (info && !info->next)
    info = GetMagickInfo("*", &exception);
  magickinfo = info;

  while (info) {
    if (info->name && info->description)
      at_input_add_handler_full(info->name, info->description, input_magick_reader, 0, info->name, NULL);
    info = info->next;
  }
  return 0;
}
