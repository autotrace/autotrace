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

#include "filename.h"
#include <string.h>
#include <glib.h>

gchar *find_suffix(const gchar *filename)
{
  g_autofree gchar *basename = g_path_get_basename(filename);
  gchar *dot_pos = strrchr(basename, '.');

  if (dot_pos == NULL || dot_pos == basename) {
    g_free(basename);
    return NULL;
  }

  gchar *suffix = g_strdup(dot_pos + 1);
  return suffix;
}

gchar *remove_suffix(const gchar *filename)
{
  gchar *basename = g_path_get_basename(filename);
  gchar *dot_pos = strrchr(basename, '.');

  if (dot_pos == NULL || dot_pos == basename) {
    // No extension or file starts with dot (like .bashrc)
    return basename;
  }

  *dot_pos = '\0'; // Truncate at the dot
  return basename;
}
