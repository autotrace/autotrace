/* color.c: color handling. */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* Def: HAVE_CONFIG_H */

#include "color.h"

color_type GET_COLOR (bitmap_type bitmap, unsigned int row, unsigned int col)
{
  color_type c;
  unsigned char *p = BITMAP_PIXEL (bitmap, row, col);

  if (BITMAP_PLANES (bitmap) >= 3)
  {
    c.r = p[0];
    c.g = p[1];
    c.b = p[2];
  }
  else
    c.g = c.b = c.r = p[0];
  return (c);
}

