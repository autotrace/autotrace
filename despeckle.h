/* despeckle.h: Bitmap despeckler for AutoTrace

   Copyright (C) 2001 David A. Bartold

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public License
   as published by the Free Software Foundation; either version 2.1 of
   the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with this library; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
   USA. */

#ifndef DESPECKLE_H
#define DESPECKLE_H

#include "types.h"
#include "bitmap.h"
#include "exception.h"

/* Despeckle - Despeckle a 8 or 24 bit image
 *
 * Input Parameters:
 *   Adaptive feature coalescing value and the despeckling level
 *
 *   Despeckling level: Integer from 0 to ~20
 *     0 = perform no despeckling
 *     An increase of the despeckling level by one doubles the size of features
 *
 *   Feature coalescing:
 *     0 = Turn it off (whites may turn black and vice versa, etc)
 *     3 = Good middle value
 *     8 = Really tight
 *
 * Modified Parameters:
 *   The bitmap is despeckled
 */

extern void
despeckle (bitmap_type *bitmap, int level, at_real tightness, at_exception_type * exp);

#endif /* not DESPECKLE_H */
