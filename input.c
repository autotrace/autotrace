/* input.c: interface for input handlers

   Copyright (C) 1999, 2000, 2001 Bernhard Herzog.

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

#include "autotrace.h"
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

#include "xstd.h"
#include "filename.h"
#include "strgicmp.h"
#include <string.h>

struct input_format_entry {
  const char * name;
  const char * descr;
  at_input_read_func reader;
};

#define END   {NULL, NULL, NULL}
static struct input_format_entry input_formats[] = {
#ifdef HAVE_LIBPNG
  { "PNG",   "Portable network graphics",      input_png_reader},
#endif /* HAVE_LIBPNG */
  { "TGA",   "Truevision Targa image",         input_tga_reader },
  { "PBM",   "Portable bitmap format",         input_pnm_reader },
  { "PNM",   "Portable anymap format",         input_pnm_reader },
  { "PGM",   "Portable graymap format",        input_pnm_reader },
  { "PPM",   "Portable pixmap format",         input_pnm_reader },
  { "BMP",   "Microsoft Windows bitmap image", input_bmp_reader },
  END
};

at_input_read_func
at_input_get_handler (at_string filename)
{
  char * ext = find_suffix (filename);
  if (ext == NULL)
     ext = "";

  return at_input_get_handler_by_suffix (ext);
}

at_input_read_func
at_input_get_handler_by_suffix (at_string suffix)
{
  struct input_format_entry * format;
  
  if (!suffix || suffix[0] == '\0')
    return NULL;

  for (format = input_formats ; format->name; format++)
    {
      if (strgicmp (suffix, format->name))
        {
          return format->reader;
        }
    }
#if HAVE_MAGICK
  return (at_input_read_func)input_magick_reader;
#else
  return NULL;
#endif /* HAVE_MAGICK */
}

char **
at_input_list_new (void)
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
  info = GetMagickInfo(NULL, &exception);
  if (info && !info->next)
    info = GetMagickInfo("*", &exception);
  magickinfo = info;
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

void
at_input_list_free(char ** list)
{
  free(list);
}

char *
at_input_shortlist (void)
{
  char * list;
  int count_int = 0;
  size_t length = 0;
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
#if HAVE_MAGICK
  while (info)
    {
#if (MagickLibVersion < 0x0537)
      if (info->tag && info->description)
#else
      if (info->name && info->description)
#endif
        {
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

int
at_input_add_handler (at_string suffix, 
		      at_string description,
		      at_input_read_func func)
{
  return 0;
}
