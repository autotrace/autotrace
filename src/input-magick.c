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
#ifdef HAVE_IMAGEMAGICK7
#include <MagickCore/MagickCore.h>
#else
#include <magick/api.h>
#endif

static at_bitmap input_magick_reader(gchar * filename, at_input_opts_type * opts, at_msg_func msg_func, gpointer msg_data, gpointer user_data)
{
  Image *image = NULL;
  ImageInfo *image_info;
  ImageType image_type;
  unsigned int i, j, point, np, runcount;
  unsigned char red, green, blue;
  at_bitmap bitmap;
#if defined(HAVE_IMAGEMAGICK7)
  Quantum q[MaxPixelChannels];
#else
  PixelPacket p;
  PixelPacket *pixel = &p;
#endif
#if defined(HAVE_IMAGEMAGICK7)
  ExceptionInfo *exception_ptr = AcquireExceptionInfo();
  MagickCoreGenesis("", MagickFalse);
#else
  ExceptionInfo exception;
  ExceptionInfo *exception_ptr = &exception;
  InitializeMagick("");
  GetExceptionInfo(exception_ptr);
#endif
  image_info = CloneImageInfo((ImageInfo *) NULL);
  (void)strcpy(image_info->filename, filename);
  image_info->antialias = 0;

  image = ReadImage(image_info, exception_ptr);
  if (image == (Image *) NULL) {
    /* MagickError(exception.severity,exception.reason,exception.description); */
    if (msg_func)
      msg_func(exception_ptr->reason, AT_MSG_FATAL, msg_data);
    goto cleanup;
  }
#if defined(HAVE_IMAGEMAGICK7)
  image_type = IdentifyImageType(image, exception_ptr);
#else
  image_type = GetImageType(image, exception_ptr);
#endif
  if (image_type == BilevelType || image_type == GrayscaleType)
    np = 1;
  else
    np = 3;

  bitmap = at_bitmap_init(NULL, image->columns, image->rows, np);

  for (j = 0, runcount = 0, point = 0; j < image->rows; j++)
    for (i = 0; i < image->columns; i++) {
#ifdef HAVE_GRAPHICSMAGICK
      ExceptionInfo exception;
      p = AcquireOnePixel(image, i, j, &exception);
#elif defined(HAVE_IMAGEMAGICK)
  #if defined(HAVE_IMAGEMAGICK7)
      ClearMagickException(exception_ptr);
      GetOneAuthenticPixel(image, i, j, q, exception_ptr);
      red = ScaleQuantumToChar(q[RedPixelChannel]);
      green = ScaleQuantumToChar(q[GreenPixelChannel]);
      blue = ScaleQuantumToChar(q[BluePixelChannel]);
  #else
  #if ((MagickLibVersion < 0x0645) || (MagickLibVersion >= 0x0649))
      p = GetOnePixel(image, i, j);
  #else
      GetOnePixel(image, i, j, pixel);
  #endif
      red = pixel->red;
      green = pixel->green;
      blue = pixel->blue;
  #endif
#endif
      AT_BITMAP_BITS(&bitmap)[point++] = red;  /* if gray: red=green=blue */
      if (np == 3) {
        AT_BITMAP_BITS(&bitmap)[point++] = green;
        AT_BITMAP_BITS(&bitmap)[point++] = blue;
      }
    }

  DestroyImage(image);
cleanup:
  DestroyImageInfo(image_info);
#if defined(HAVE_IMAGEMAGICK7)
  DestroyExceptionInfo(exception_ptr);
#endif
  return (bitmap);
}

int install_input_magick_readers(void)
{
  size_t n = 0;
#if defined(HAVE_IMAGEMAGICK7)
  ExceptionInfo *exception_ptr = AcquireExceptionInfo();
#else
  ExceptionInfo exception;
  ExceptionInfo *exception_ptr = &exception;
#endif
  MagickInfo *info;
  const MagickInfo **infos;
#if defined(HAVE_IMAGEMAGICK7)
  MagickCoreGenesis("", MagickFalse);
#else
  InitializeMagick("");

  GetExceptionInfo(exception_ptr);
#endif

#ifdef HAVE_GRAPHICSMAGICK
  info = GetMagickInfo("*", &exception);
  while (info) {
    if (info->name && info->description && strcasecmp(info->name, "tga")) {
      at_input_add_handler_full(info->name, info->description, input_magick_reader, 0, info->name, NULL);
    }
    info = info->next;
  }
#else
  infos = GetMagickInfoList("*", &n, exception_ptr);
  for (int i = 0; i < n; i++){
    info = infos[i];
    if (info->name && info->description)
      at_input_add_handler_full(info->name, info->description, input_magick_reader, 0, info->name, NULL);
  }
#endif // HAVE_GRAPHICSMAGICK

  DestroyExceptionInfo(exception_ptr);
  return 0;
}
