/* despeckle.h: Bitmap despeckler for AutoTrace

   Copyright (C) 2001 David A. Bartold

   This program is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by the
   Free Software Foundation; either version 2, or (at your option) any
   later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#ifndef DESPECKLE_H
#define DESPECKLE_H

#include "types.h"
#include "bitmap.h"

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
despeckle (bitmap_type *bitmap, int level, real tightness);

#endif /* not DESPECKLE_H */
