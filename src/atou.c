/*
 * SPDX-FileCopyrightText: © 2000-2001 Martin Weber
 * SPDX-FileCopyrightText: © 2002-2003 Masatake YAMATO
 * SPDX-FileCopyrightText: © 2017-2025 Peter Lemenkov
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

/* atou.c: like atoi, but if the number is negative, abort. */

#include "atou.h"
#include "logreport.h"
#include <glib.h>

unsigned atou(const gchar *s)
{
  guint64 val;

  if (!g_ascii_string_to_unsigned(s, 10, 0, UINT_MAX, &val, NULL))
    FATAL("Invalid unsigned integer: '%s'", s);

  return (unsigned)val;
}
