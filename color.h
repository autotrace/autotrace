/* color.h: declarations for color handling. */

#ifndef COLOR_H
#define COLOR_H

#include "autotrace.h"
#include "bitmap.h"

typedef at_color_type color_type;

/* RGB to grayscale */
#define COLOR_LUMINANCE(c) ((unsigned char)(((c).r) * 0.30 + ((c).g) * 0.59 + ((c).b) * 0.11 + 0.5))

#define COLOR_EQUAL(c1,c2) (((c1).r == (c2).r) && ((c1).g == (c2).g) && ((c1).b == (c2).b))

color_type GET_COLOR (at_bitmap_type, unsigned int, unsigned int);

#endif /* not COLOR_H */
