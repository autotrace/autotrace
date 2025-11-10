/*
 * Copyright (C) 2001 David A. Bartold / Martin Weber
 * SPDX-FileCopyrightText: © 2001 David A. Bartold
 * SPDX-FileCopyrightText: © 2001-2003 Martin Weber
 * SPDX-FileCopyrightText: © 2001-2003 Masatake YAMATO
 * SPDX-FileCopyrightText: © 2017-2025 Peter Lemenkov
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#ifndef DESPECKLE_H
#define DESPECKLE_H

#include "autotrace.h"
#include "exception.h"
#include "input.h"
#include <glib.h>

/* Despeckle - Despeckle a 8 or 24 bit image
 *
 * Input Parameters:
 *   Adaptive feature coalescing value, the despeckling level and noise removal
 *
 *   Despeckling level (level): Integer from 0 to ~20
 *     0 = perform no despeckling
 *     An increase of the despeckling level by one doubles the size of features.
 *     The Maximum value must be smaller then the logarithm base two of the number
 *     of pixels.
 *
 *   Feature coalescing (tightness): Real from 0.0 to ~8.0
 *     0 = Turn it off (whites may turn black and vice versa, etc)
 *     3 = Good middle value
 *     8 = Really tight
 *
 *   Noise removal (noise_removal): Real from 1.0 to 0.0
 *     1 = Maximum noise removal
 *     You should always use the highest value, only if certain parts of the image
 *     disappear you should lower it.
 *
 * Modified Parameters:
 *   The bitmap is despeckled.
 */

extern void despeckle(at_bitmap *bitmap, int level, gfloat tightness, gfloat noise_removal,
                      at_exception_type *exp);

#endif /* not DESPECKLE_H */
