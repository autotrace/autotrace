/* thin-image.h: thin binary image

   Copyright (C) 2001, 2002 Martin Weber

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

#ifndef THIN_IMAGE_H
#define THIN_IMAGE_H

/*
 * C code from the article
 * "Efficient Binary Image Thinning using Neighborhood Maps"
 * by Joseph M. Cychosz, 3ksnn64@ecn.purdue.edu
 * in "Graphics Gems IV", Academic Press, 1994
 */

#include "bitmap.h"
#include "color.h"
#include "exception.h"

void thin_image(at_bitmap * image, const at_color * bg_color, at_exception_type * exp);

#endif /* not THIN_IMAGE_H */
