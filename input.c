/* input.c: input routines
   Copyright (C) 1999 Bernhard Herzog. */

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

#include "bitmap.h"

#include "xstd.h"
#include "filename.h"
#include "strgicmp.h"
#include <string.h>

struct input_format_entry {
  input_read reader;
  const char * descr;
  const char * name;

};

static struct input_format_entry input_formats[] = {
#ifdef HAVE_LIBPNG
  { png_load_image,   "Portable network graphics",      "PNG" },
#endif /* HAVE_LIBPNG */
  { tga_load_image,   "Truevision Targa image",         "TGA" },
  { pnm_load_image,   "Portable bitmap format",         "PBM" },
  { pnm_load_image,   "Portable anymap format",         "PNM" },
  { pnm_load_image,   "Portable graymap format",        "PGM" },
  { pnm_load_image,   "Portable pixmap format",         "PPM" },
  { bmp_load_image,   "Microsoft Windows bitmap image", "BMP" },
  {NULL, NULL, NULL}
};

input_read
input_get_handler (at_string filename)
{
  char * ext = find_suffix (filename);
  if (ext == NULL)
     ext = "";

  return input_get_handler_by_suffix (ext);
}

input_read
input_get_handler_by_suffix (at_string suffix)
{
  struct input_format_entry * format;
  for (format = input_formats ; format->name; format++)
    {
      if (strgicmp (suffix, format->name))
        {
          return format->reader;
        }
    }
#if HAVE_MAGICK
  return (input_read)magick_load_image;
#else
  return NULL;
#endif /* HAVE_MAGICK */
}

char **
input_list (void)
{
  char ** list;
  int count, count_int = 0;
  int i;
#if HAVE_MAGICK
  ExceptionInfo exception;
#if (MagickLibVersion < 0x0540)
  MagickInfo *info, *magickinfo;
#else
  const MagickInfo *info, *magickinfo;
#endif
#endif

  struct input_format_entry * entry;
  for (entry = input_formats; entry->name; entry++)
    count_int++;
#if HAVE_MAGICK
#if (MagickLibVersion < 0x0538)
  MagickIncarnate("");
#else
  InitializeMagick("");
#endif
  GetExceptionInfo(&exception);
#if (MagickLibVersion < 0x0534)
  magickinfo = info = GetMagickInfo(NULL);
#else
  magickinfo = info = GetMagickInfo("", &exception);
#endif
#endif
  count = count_int;
#if HAVE_MAGICK
  while (info)
    {
#if (MagickLibVersion < 0x0537)
      if (info->tag && info->description)
#else
      if (info->name && info->description)
#endif
        count ++;
      info = info->next ;
    }
#endif

  XMALLOC(list, sizeof(char*)*((2*count)+1));

  entry = input_formats;
  for (i = 0; i < count_int; i++)
    {
      list[2*i] = (char *)entry[i].name;
      list[2*i+1] = (char *)entry[i].descr;
    }

#if HAVE_MAGICK
  info = magickinfo;

  while (info)
    {
#if (MagickLibVersion < 0x0537)
      if (info->tag && info->description)
#else
      if (info->name && info->description)
#endif
        {
#if (MagickLibVersion < 0x0537)
          list[2*i] = info->tag;
#else
          list[2*i] = info->name;
#endif
          list[2*i+1] = info->description;
          i++;
        }
      info = info->next ;
    }
#endif
  list[2*i] = NULL;
  return list;
}

char *
input_shortlist (void)
{
  char * list;
  int count_int = 0, count;
  int length = 0;
  int i;
#if HAVE_MAGICK
  ExceptionInfo exception;
#if (MagickLibVersion < 0x0540)
  MagickInfo *info, *magickinfo;
#else
  const MagickInfo *info, *magickinfo;
#endif
#endif

  struct input_format_entry * entry;
  for (entry = input_formats; entry->name; entry++)
    {
      count_int++;
      length += strlen (entry->name) + 2;
  }

#if HAVE_MAGICK
#if (MagickLibVersion < 0x0538)
  MagickIncarnate("");
#else
  InitializeMagick("");
#endif
  GetExceptionInfo(&exception);
#if (MagickLibVersion < 0x0534)
  magickinfo = info = GetMagickInfo(NULL);
#else
  magickinfo = info = GetMagickInfo(NULL, &exception);
#endif
#endif
count = count_int;
#if HAVE_MAGICK
  while (info)
    {
#if (MagickLibVersion < 0x0537)
      if (info->tag && info->description)
#else
      if (info->name && info->description)
#endif
        {
          count ++;
#if (MagickLibVersion < 0x0537)
          length += strlen (info->tag) + 2;
#else
          length += strlen (info->name) + 2;
#endif
        }
      info = info->next ;
    }
#endif

  XMALLOC(list, sizeof (char) * (length + 1 + 2));

  entry = input_formats;
  strcpy (list, (char *) entry[0].name);
  for (i = 1; i < count_int - 1; i++)
    {
      strcat (list, ", ");
      strcat (list, (char *) entry[i].name);
    }
#if HAVE_MAGICK
  info = magickinfo;
  while (info)
    {
#if (MagickLibVersion < 0x0537)
      if (info->tag && info->description)
#else
      if (info->name && info->description)
#endif
        {
          strcat (list, ", ");
#if (MagickLibVersion < 0x0537)
          strcat (list, info->tag);
#else
          strcat (list, info->name);
#endif
        }
      info = info->next ;
    }
#endif
  strcat (list, " or ");
  strcat (list, (char *) entry[i].name);
  return list;
}
