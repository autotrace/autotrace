/*
 * SPDX-FileCopyrightText: © 2002-2003 Masatake YAMATO
 * SPDX-FileCopyrightText: © 2017-2025 Peter Lemenkov
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

/* image-proc.h: image processing routines */

#ifndef IMAGE_PROC_H
#define IMAGE_PROC_H

#include "autotrace.h"
#include "exception.h"
#include "input.h"
#include "progress.h"
#include <glib.h>

typedef struct {
  unsigned height, width;
  float **weight;
  float **d;
} at_distance_map;

/* Allocate and compute a new distance map.  CANCEL is asked about once per
   million pixels; when it says to cancel, the map is returned unfinished,
   to be freed with free_distance_map().  */
extern at_distance_map new_distance_map(at_bitmap *, unsigned char target_value, gboolean padded,
                                        cancel_hook *cancel, at_exception_type *exp);

/* Free the dynamically-allocated storage associated with a distance map. */
extern void free_distance_map(at_distance_map *);

#endif /* not IMAGE_PROC_H */
