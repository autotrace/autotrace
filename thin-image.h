/* thin-image.h: */
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

void  thin_image      (bitmap_type *image, const color_type *bg_color, at_exception * exp);

#endif /* not THIN_IMAGE_H */
