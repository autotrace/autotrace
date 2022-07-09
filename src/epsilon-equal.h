/*
 * SPDX-FileCopyrightText: © 2000-2001 Martin Weber
 * SPDX-FileCopyrightText: © 2001-2003 Masatake YAMATO
 * SPDX-FileCopyrightText: © 2017-2025 Peter Lemenkov
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

/* epsilon-equal.h: define an error resist compare. */

#ifndef EPSILON_EQUAL_H
#define EPSILON_EQUAL_H

#include <glib.h>

/* Says whether V1 and V2 are within REAL_EPSILON of each other.
   Fixed-point arithmetic would be better, to guarantee machine
   independence, but it's so much more painful to work with.  The value
   here is smaller than can be represented in either a `fix_word' or a
   `scaled_num', so more precision than this will be lost when we
   output, anyway.  */
gboolean epsilon_equal(float v1, float v2);

#endif /* not EPSILON_EQUAL_H */
