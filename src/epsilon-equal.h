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

/* Numerical errors sometimes make a floating point number just slightly
   larger or smaller than its TRUE value.  When it matters, we need to
   compare with some tolerance, REAL_EPSILON.  Fixed-point arithmetic
   would be better, to guarantee machine independence, but it's so much
   more painful to work with.  The value here is smaller than can be
   represented in either a `fix_word' or a `scaled_num', so more
   precision than this will be lost when we output, anyway.  */
#define REAL_EPSILON 0.00001f

/* Says whether V1 and V2 are within REAL_EPSILON of each other.  The
   comparison is done in float, as the function this replaces did.  */
#define epsilon_equal(v1, v2) G_APPROX_VALUE((float)(v1), (float)(v2), REAL_EPSILON)

#endif /* not EPSILON_EQUAL_H */
