/* color.c: color handling. */

#include "color.h"

color_type GET_COLOR (bitmap_type bitmap, unsigned int row, unsigned int col)
{
  color_type c;

  if (BITMAP_PLANES (bitmap) >= 3)
  {
    c.r = BITMAP_PIXEL (bitmap, row, col)[0];
    c.g = BITMAP_PIXEL (bitmap, row, col)[1];
    c.b = BITMAP_PIXEL (bitmap, row, col)[2];
  }
  else
    c.g = c.b = c.r = BITMAP_PIXEL (bitmap, row, col)[0];
  return (c);
}

/* version 0.24 */
