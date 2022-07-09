/*
 * SPDX-FileCopyrightText: © 2000-2002 Martin Weber
 * SPDX-FileCopyrightText: © 2001-2003 Masatake YAMATO
 * SPDX-FileCopyrightText: © 2017-2025 Peter Lemenkov
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

/* epsilon-equal.c: define a error resist compare. */

#include "epsilon-equal.h"
#include <glib.h>
#include <math.h>

static const float REAL_EPSILON = 0.00001f;

/* Numerical errors sometimes make a floating point number just slightly
   larger or smaller than its TRUE value.  When it matters, we need to
   compare with some tolerance, REAL_EPSILON, defined in kbase.h.  */

gboolean epsilon_equal(float v1, float v2)
{
  return fabs(v1 - v2) <= REAL_EPSILON;
}
