/*
 * SPDX-FileCopyrightText: © 2000-2001 Martin Weber
 * SPDX-FileCopyrightText: © 2001-2003 Masatake YAMATO
 * SPDX-FileCopyrightText: © 2017-2025 Peter Lemenkov
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

/* atou.h: like atoi, but if the number is negative, abort. */

#ifndef ATOU_H
#define ATOU_H

#include <glib.h>
unsigned atou(const gchar *s);

#endif /* not ATOU_H */
