/* filename.c: Function manipulate file names
   Was: find-suffix, extend-fname, make-suffix, remove-suffx
   substring */

/* remove-suffx.c: remove any suffix.

Copyright (C) 1999 Free Software Foundation, Inc.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* Def: HAVE_CONFIG_H */

#include "filename.h"
#include "xstd.h"
#include <string.h>
#include <glib.h>

/* Return a fresh copy of SOURCE[START..LIMIT], or NULL if LIMIT<START.
   If LIMIT>strlen(START), it is reassigned. */
static gchar *substring(gchar * source, const unsigned start, const unsigned limit);

gchar *find_suffix(gchar * name)
{
  gchar *dot_pos = strrchr(name, '.');
#ifdef WIN32
  gchar *slash_pos = strrchr(name, '\\');
#else
  gchar *slash_pos = strrchr(name, '/');
#endif

  /* If the name is `foo' or `/foo.bar/baz', we have no extension.  */
  return dot_pos == NULL || dot_pos < slash_pos ? NULL : dot_pos + 1;
}

gchar *extend_filename(gchar * name, gchar * default_suffix)
{
  gchar *new_s;
  gchar *suffix = find_suffix(name);

  new_s = suffix == NULL ? g_strconcat(name, ".", default_suffix) : name;
  return new_s;
}

gchar *remove_suffix(gchar * s)
{
  gchar *suffix = find_suffix(s);

  return suffix == NULL ? s : suffix - 2 - s < 0 ? NULL : substring(s, 0, (unsigned)(suffix - 2 - s));
}

/* From substring.c */
static gchar *substring(gchar * source, const unsigned start, const unsigned limit)
{
  gchar *result;
  unsigned this_char;
  size_t length = strlen(source);
  size_t lim = limit;

  /* Upper bound out of range? */
  if (lim >= length)
    lim = length - 1;

  /* Null substring? */
  if (start > lim)
    return "";

  /* The `2' here is one for the null and one for limit - start inclusive. */
  XMALLOC(result, lim - start + 2);

  for (this_char = start; this_char <= lim; this_char++)
    result[this_char - start] = source[this_char];

  result[this_char - start] = 0;

  return result;
}
