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
#include "input-magick.h"
#endif /* HAVE_MAGICK */

#include "bitmap.h"

#include "xstd.h"
#include "filename.h"
#include <string.h>

#define STREQ(s1, s2) (strcmp (s1, s2) == 0)

struct input_format_entry {
  input_read reader;
  const char * descr;
  const char * name;

};

static struct input_format_entry input_formats[] = {
#ifdef HAVE_LIBPNG
  { png_load_image,  "Portable Network Graphics",  "png" },
#endif /* HAVE_LIBPNG */
#if HAVE_MAGICK
  { magick_load_image, "", "magick" },
#endif /* HAVE_MAGICK */
  { ReadTGA,          "", "tga" },
  { pnm_load_image,   "", "pbm" },
  { pnm_load_image,   "", "pnm" },
  { pnm_load_image,   "", "pgm" },
  { pnm_load_image,   "", "ppm" },
  { ReadBMP, "", "bmp" },
  {NULL, NULL, NULL}
};

input_read
input_get_handler (string filename)
{
  char * ext = find_suffix (filename);
  if (ext == NULL)
     ext = "";

  return input_get_handler_by_suffix (ext);
}

input_read
input_get_handler_by_suffix (string suffix)
{
  struct input_format_entry * format;
  for (format = input_formats ; format->name; format++)
    {
      if (STREQ (suffix, format->name))
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
  int count = 0;
  int i;

  struct input_format_entry * entry;
  for (entry = input_formats; entry->name; entry++)
    count++;
  
  XMALLOC(list, sizeof(char*)*((2*count)+1));

  entry = input_formats;
  for (i = 0; i < count; i++)
    {
      list[2*i] = (char *)entry[i].name;
      list[2*i+1] = (char *)entry[i].descr;
    }
  list[2*i] = NULL;
  return list;
}
