/* filename.c: Function manipulate file names
   Was: find-suffix, remove-suffix */

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

gchar *find_suffix(gchar * name)
{
  gchar *dot_pos = strrchr(name, '.');
  gchar *slash_pos = strrchr(name, G_DIR_SEPARATOR);

  /* If the name is `foo' or `/foo.bar/baz', we have no extension.  */
  return dot_pos == NULL || dot_pos < slash_pos ? NULL : dot_pos + 1;
}

gchar *remove_suffix(gchar * s)
{
  gchar *suffix = find_suffix(s);

  return suffix == NULL ? s : suffix - 2 - s < 0 ? NULL : g_strndup(s, (unsigned)(suffix - 2 - s));
}
