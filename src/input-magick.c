/*
 * Copyright (C) 1999, 2000, 2001 Martin Weber
 * SPDX-FileCopyrightText: © 2000 Johannes.Schindelin
 * SPDX-FileCopyrightText: © 2000-2002 Martin Weber
 * SPDX-FileCopyrightText: © 2000-2003 Masatake YAMATO
 * SPDX-FileCopyrightText: © 2009 Caolan McNamara
 * SPDX-FileCopyrightText: © 2017-2025 Peter Lemenkov
 * SPDX-FileCopyrightText: © 2020 Sacha Chua
 * SPDX-FileCopyrightText: © 2023 Mamoru TASAKA
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* Def: HAVE_CONFIG_H */

#include "input.h"
#include "input-magick.h"

#include <string.h>
#ifdef HAVE_IMAGEMAGICK7
#include <MagickCore/MagickCore.h>
#else
#include <magick/api.h>
#endif

static at_bitmap input_magick_reader(gchar *filename, at_input_opts_type *opts,
                                     at_msg_func msg_func, gpointer msg_data, gpointer user_data)
{
  Image *image = NULL;
  ImageInfo *image_info;
  ImageType image_type;
  unsigned int i, j, point, np;
  unsigned char red, green, blue;
  at_bitmap bitmap = at_bitmap_init(NULL, 0, 0, 0);
#if defined(HAVE_IMAGEMAGICK7)
  ExceptionInfo *exception_ptr = AcquireExceptionInfo();
  MagickCoreGenesis("", MagickFalse);
#else
  ExceptionInfo exception;
  ExceptionInfo *exception_ptr = &exception;
  InitializeMagick("");
  GetExceptionInfo(exception_ptr);
#endif
  image_info = CloneImageInfo((ImageInfo *)NULL);
  if (g_strlcpy(image_info->filename, filename, sizeof(image_info->filename)) >=
      sizeof(image_info->filename)) {
    if (msg_func)
      msg_func("input file name is too long", AT_MSG_FATAL, msg_data);
    goto cleanup;
  }
  image_info->antialias = 0;

  image = ReadImage(image_info, exception_ptr);
  if (image == (Image *)NULL) {
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

  /* Fetch one row at a time.  The samples are Quantum-sized (usually 16 bits,
     or a float in HDRI builds), so scale every one of them down to 8 bits.  */
  for (j = 0, point = 0; j < image->rows; j++) {
#if defined(HAVE_IMAGEMAGICK7)
    const Quantum *p = GetVirtualPixels(image, 0, j, image->columns, 1, exception_ptr);
#elif defined(HAVE_GRAPHICSMAGICK)
    const PixelPacket *p = AcquireImagePixels(image, 0, j, image->columns, 1, exception_ptr);
#else
    const PixelPacket *p = GetVirtualPixels(image, 0, j, image->columns, 1, exception_ptr);
#endif

    if (p == NULL) {
      if (msg_func)
        msg_func(exception_ptr->reason ? exception_ptr->reason : "cannot read the image pixels",
                 AT_MSG_FATAL, msg_data);
      g_free(AT_BITMAP_BITS(&bitmap));
      bitmap = at_bitmap_init(NULL, 0, 0, 0);
      break;
    }

    for (i = 0; i < image->columns; i++) {
#if defined(HAVE_IMAGEMAGICK7)
      red = ScaleQuantumToChar(GetPixelRed(image, p));
      green = ScaleQuantumToChar(GetPixelGreen(image, p));
      blue = ScaleQuantumToChar(GetPixelBlue(image, p));
      p += GetPixelChannels(image);
#else
      red = ScaleQuantumToChar(p->red);
      green = ScaleQuantumToChar(p->green);
      blue = ScaleQuantumToChar(p->blue);
      p++;
#endif
      AT_BITMAP_BITS(&bitmap)[point++] = red; /* if gray: red=green=blue */
      if (np == 3) {
        AT_BITMAP_BITS(&bitmap)[point++] = green;
        AT_BITMAP_BITS(&bitmap)[point++] = blue;
      }
    }
  }

  DestroyImage(image);
cleanup:
  DestroyImageInfo(image_info);
  DestroyExceptionInfo(exception_ptr);
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
  const MagickInfo *info;
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
    if (info->name && info->description) {
      at_input_add_handler_full(info->name, info->description, input_magick_reader, 0, info->name,
                                NULL);
    }
    info = info->next;
  }
#else
  infos = GetMagickInfoList("*", &n, exception_ptr);
  for (int i = 0; i < n; i++) {
    info = infos[i];
    if (info->name && info->description)
      at_input_add_handler_full(info->name, info->description, input_magick_reader, 0, info->name,
                                NULL);
  }
  RelinquishMagickMemory(infos);
#endif // HAVE_GRAPHICSMAGICK

  DestroyExceptionInfo(exception_ptr);
  return 0;
}
