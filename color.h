/* color.h: declarations for color handling. */

#ifndef COLOR_H
#define COLOR_H

#include "bitmap.h"

/* Color in RGB */
typedef struct
{
  unsigned char r;
  unsigned char g;
  unsigned char b;
} color_type;

#define COLOR_EQUAL(c1,c2) ((c1.r == c2.r) && (c1.g == c2.g) && (c1.b == c2.b))

color_type GET_COLOR (bitmap_type, unsigned int, unsigned int);

#endif /* not COLOR_H */

/* version 0.24 */
