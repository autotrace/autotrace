/* filename.h: Function manipulate file names
   Was: find-suffix, remove-suffix */

/*
 * Copyright (C) 1992 Free Software Foundation, Inc.
 * SPDX-FileCopyrightText: © 2000-2001 Martin Weber
 * SPDX-FileCopyrightText: © 2000-2003 Masatake YAMATO
 * SPDX-FileCopyrightText: © 2017-2025 Peter Lemenkov
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef FILENAME_H
#define FILENAME_H
#include <glib.h>

/* If NAME has a suffix, return a pointer to its first character (i.e.,
   the one after the `.'); otherwise, return NULL.  */
gchar *find_suffix(const gchar *name);

/* Return NAME with any suffix removed.  */
gchar *remove_suffix(const gchar *name);

#endif /* Not def: FILENAME_H */
