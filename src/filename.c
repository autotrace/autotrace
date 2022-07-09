/* filename.c: Function manipulate file names
   Was: find-suffix, remove-suffix */

/*
 * Copyright (C) 1999 Free Software Foundation, Inc.
 * SPDX-FileCopyrightText: © 2000-2003 Masatake YAMATO
 * SPDX-FileCopyrightText: © 2001-2002 Martin Weber
 * SPDX-FileCopyrightText: © 2017-2025 Peter Lemenkov
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

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
