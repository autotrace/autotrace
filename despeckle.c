/* despeckle.c: Bitmap despeckler for AutoTrace

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

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <time.h>
#include "xstd.h"
#include "types.h"
#include "bitmap.h"


/* Calculate Error - compute the error between two colors
 *
 *   Input parameters:
 *     Two 24 bit RGB colors
 *
 *   Returns:
 *     The squared error between the two colors
 */

static int
calc_error (unsigned char *color1,
            unsigned char *color2)
{
  int the_error;
  int temp;

  temp = color1[0] - color2[0];
  the_error = temp * temp;
  temp = color1[1] - color2[1];
  the_error += temp * temp;
  temp = color1[2] - color2[2];
  the_error += temp * temp;

  return the_error;
}


/* Is equal - Are two colors the same?
 *
 *   Input parameters:
 *     Two 24 bit RGB colors
 *
 *   Returns:
 *     TRUE - yes, FALSE - no
 */

static bool
is_equal (unsigned char *color1,
          unsigned char *color2)
{
  return color1[0] == color2[0] &&
         color1[1] == color2[1] &&
         color1[2] == color2[2];
}


/* Assign - copy one color to another
 *
 *   Input parameter:
 *     Input RGB color
 *
 *   Output parameter:
 *     Output RGB color
 */

static void
assign (unsigned char *out,
        unsigned char *in)
{
  out[0] = in[0];
  out[1] = in[1];
  out[2] = in[2];
}


/* Find Size 8 - Find the number of adjacent pixels of the same color and
 * return closest color of all neighboring pixels
 *
 * Input Parameters:
 *   An 8 bit image, the current location inside the image, and the color
 *   we are looking for
 *
 * Modified Parameters:
 *   A mask array used to prevent backtracking over already counted pixels
 *
 * Returns:
 *   Number of adjacent pixels found having the same color, the closest
 *   color of all neighboring pixels, and the color error amount
 */

static int
find_size_8 (/* in */     unsigned char  color,
             /* out */    unsigned char *closest_color,
             /* in/out */ int           *error_amount,
             /* in */     int            x,
             /* in */     int            y,
             /* in */     bitmap_type   *bitmap,
             /* in/out */ unsigned char *mask)
{
  unsigned char *data;
  int            width, height;
  int            count;
  int            x1, x2;
  int            temp_error;

  data   = bitmap->bitmap;
  width  = BITMAP_WIDTH (*bitmap);
  height = BITMAP_HEIGHT (*bitmap);

  if (y < 0 || y >= height || mask[y * width + x] == 1)
    return 0;

  if (data[y * width + x] != color)
    {
      temp_error = abs (color - data[y * width + x]);
      if (temp_error < *error_amount)
        {
          *closest_color = data[y * width + x];
          *error_amount = temp_error;
        }

      return 0;
    }

  for (x1 = x; x1 >= 0 && data[y * width + x1] == color; x1--) ;
  x1++;
  for (x2 = x; x2 < width && data[y * width + x2] == color; x2++) ;

  if (x1 > 0)
    {
      temp_error = abs (color - data[y * width + x1 - 1]);
      if (temp_error < *error_amount)
        {
          *closest_color = data[y * width + x1 - 1];
          *error_amount = temp_error;
        }
    }

  if (x2 < width)
    {
      temp_error = abs (color - data[y * width + x2]);
      if (temp_error < *error_amount)
        {
          *closest_color = data[y * width + x2];
          *error_amount = temp_error;
        }
    }

  count = x2 - x1;
  for (x = x1; x < x2; x++)
    mask[y * width + x] = 1;

  for (x = x1; x < x2; x++)
    {
      count += find_size_8 (color, closest_color, error_amount,
                            x, y - 1, bitmap, mask);
      count += find_size_8 (color, closest_color, error_amount,
                            x, y + 1, bitmap, mask);
    }

  return count;
}


/* Find Size 24 - Find the number of adjacent pixels of the same color and
 * return closest color of all neighboring pixels
 *
 * Input Parameters:
 *   A 24 bit image, the current location inside the image, and the color
 *   we are looking for
 *
 * Modified Parameters:
 *   A mask array used to prevent backtracking over already counted pixels
 *
 * Returns:
 *   Number of adjacent pixels found having the same color, the closest
 *   color of all neighboring pixels, and the color error amount
 */

static int
find_size_24 (/* in */     unsigned char *color,
              /* out */    unsigned char *closest_color,
              /* in/out */ int           *error_amount,
              /* in */     int            x,
              /* in */     int            y,
              /* in */     bitmap_type   *bitmap,
              /* in/out */ unsigned char *mask)
{
  unsigned char *data;
  int            width, height;
  int            count;
  int            x1, x2;
  int            temp_error;

  data   = bitmap->bitmap;
  width  = BITMAP_WIDTH (*bitmap);
  height = BITMAP_HEIGHT (*bitmap);

  if (y < 0 || y >= height || mask[y * width + x] == 1)
    return 0;

  if (!is_equal (&data[(y * width + x) * 3], color))
    {
      unsigned char *value = &data[(y * width + x) * 3];

      temp_error = calc_error (value, color);
      if (temp_error < *error_amount)
        {
          assign (closest_color, value);
          *error_amount = temp_error;
        }

      return 0;
    }

  for (x1 = x; x1 >= 0 && is_equal (&data[(y * width + x1) * 3], color); x1--) ;
  x1++;
  for (x2 = x; x2 < width && is_equal (&data[(y * width + x2) * 3], color); x2++) ;

  if (x1 > 0)
    {
      unsigned char *value = &data[(y * width + x1 - 1) * 3];

      temp_error = calc_error (value, color);
      if (temp_error < *error_amount)
        {
          assign (closest_color, value);
          *error_amount = temp_error;
        }
    }

  if (x2 < width)
    {
      unsigned char *value = &data[(y * width + x2) * 3];

      temp_error = calc_error (value, color);
      if (temp_error < *error_amount)
        {
          assign (closest_color, value);
          *error_amount = temp_error;
        }
    }

  count = x2 - x1;
  for (x = x1; x < x2; x++)
    mask[y * width + x] = 1;

  for (x = x1; x < x2; x++)
    {
      count += find_size_24 (color, closest_color, error_amount,
                             x, y - 1, bitmap, mask);
      count += find_size_24 (color, closest_color, error_amount,
                             x, y + 1, bitmap, mask);
    }

  return count;
}


/* Fill 8 - change the color of a blob
 *
 * Input parameters:
 *   The new color
 *
 * Modified parameters:
 *   8 bit bitmap and its mask (used to prevent backtracking)
 */

static void
fill_8 (/* in */     unsigned char  to_color,
        /* in */     int            x,
        /* in */     int            y,
        /* in/out */ bitmap_type   *bitmap,
        /* in/out */ unsigned char *mask)
{
  unsigned char *data;
  int            x1, x2;
  int            width, height;

  data   = bitmap->bitmap;
  width  = BITMAP_WIDTH (*bitmap);
  height = BITMAP_HEIGHT (*bitmap);

  if (y < 0 || y >= height || mask[y * width + x] != 1)
    return;

  for (x1 = x; x1 >= 0 && mask[y * width + x1] == 1; x1--) ;
  x1++;
  for (x2 = x; x2 < width && mask[y * width + x2] == 1; x2++) ;

  for (x = x1; x < x2; x++)
    mask[y * width + x] = 2;

  for (x = x1; x < x2; x++)
    {
      data[y * width + x] = to_color;

      fill_8 (to_color, x, y - 1, bitmap, mask);
      fill_8 (to_color, x, y + 1, bitmap, mask);
    }
}


/* Fill 24 - change the color of a blob
 *
 * Input parameters:
 *   The new color
 *
 * Modified parameters:
 *   24 bit bitmap and its mask (used to prevent backtracking)
 */

static void
fill_24 (/* in */     unsigned char *to_color,
         /* in */     int            x,
         /* in */     int            y,
         /* in/out */ bitmap_type   *bitmap,
         /* in/out */ unsigned char *mask)
{
  unsigned char *data;
  int            x1, x2;
  int            width, height;

  data   = bitmap->bitmap;
  width  = BITMAP_WIDTH (*bitmap);
  height = BITMAP_HEIGHT (*bitmap);

  if (y < 0 || y >= height || mask[y * width + x] != 1)
    return;

  for (x1 = x; x1 >= 0 && mask[y * width + x1] == 1; x1--) ;
  x1++;
  for (x2 = x; x2 < width && mask[y * width + x2] == 1; x2++) ;

  for (x = x1; x < x2; x++)
    mask[y * width + x] = 2;

  for (x = x1; x < x2; x++)
    {
      if (to_color != NULL)
        assign (&data[(y * width + x) * 3], to_color);

      fill_24 (to_color, x, y - 1, bitmap, mask);
      fill_24 (to_color, x, y + 1, bitmap, mask);
    }
}


/* Recolor 8 - conditionally change a feature's color to the closest color
 * of all neighboring pixels
 *
 * Input parameters:
 *   Current blob size, and adaptive tightness
 *
 *   Adaptive Tightness: (integer 1 to 256)
 *     1   = really tight
 *     256 = turn off the feature
 *
 * Modified parameters:
 *   8 bit bitmap and its mask (used to prevent backtracking)
 */

static void
recolor_8 (/* in */     int            current_size,
           /* in */     int            adaptive_tightness,
           /* in */     int            x,
           /* in */     int            y,
           /* in/out */ bitmap_type   *bitmap,
           /* in/out */ unsigned char *mask)
{
  unsigned char *data;
  unsigned char  color, to_color;
  int            error_amount;
  int            size;
  int            width, height;

  data   = bitmap->bitmap;
  width  = BITMAP_WIDTH (*bitmap);
  height = BITMAP_HEIGHT (*bitmap);

  color = data[y * width + x];
  error_amount = 0xffffff;

  size = find_size_8 (color, &to_color, &error_amount, x, y, bitmap, mask);

  /* Feature big enough, ignore. */
  if (size > current_size)
    fill_8 (color, x, y, bitmap, mask);

  /* This condition only fails if the bitmap is all the same color */
  if (error_amount != 0xffffff)
    {
      /*
       * If the difference between the two colors is too great,
       * don't coalesce the feature with its neighbor(s).  This prevents a
       * color from turning into its complement.
       */

      if (error_amount > adaptive_tightness)
        fill_8 (color, x, y, bitmap, mask);
      else
        fill_8 (to_color, x, y, bitmap, mask);
    }
}


/* Recolor 24 - conditionally change a feature's color to the closest color
 * of all neighboring pixels
 *
 * Input parameters:
 *   Current blob size, and adaptive tightness
 *
 *   Adaptive Tightness: (integer 1 to 256)
 *     1   = really tight
 *     256 = turn off the feature
 *
 * Modified parameters:
 *   24 bit bitmap and its mask (used to prevent backtracking)
 */

static void
recolor_24 (/* in */     int            current_size,
            /* in */     int            adaptive_tightness,
            /* in */     int            x,
            /* in */     int            y,
            /* in/out */ bitmap_type   *bitmap,
            /* in/out */ unsigned char *mask)
{
  unsigned char *data;
  unsigned char  color[3], to_color[3];
  int            error_amount;
  int            size;
  int            width, height;

  data   = bitmap->bitmap;
  width  = BITMAP_WIDTH (*bitmap);
  height = BITMAP_HEIGHT (*bitmap);

  assign (color, &data[(y * width + x) * 3]);
  error_amount = 0xffffff;

  size = find_size_24 (color, to_color, &error_amount, x, y, bitmap, mask);

  /* Feature big enough, ignore. */
  if (size > current_size)
    fill_24 (NULL, x, y, bitmap, mask);

  /* This condition only fails if the bitmap is all the same color */
  if (error_amount != 0xffffff)
    {
      error_amount = (int) sqrt (error_amount / 3.0);

      /*
       * If the difference between the two colors is too great,
       * don't coalesce the feature with its neighbor(s).  This prevents a
       * color from turning into its complement.
       */

      if (error_amount > adaptive_tightness)
        fill_24 (NULL, x, y, bitmap, mask);
      else
        fill_24 (to_color, x, y, bitmap, mask);
    }
}


/* Despeckle Iteration - Despeckle all regions smaller than cur_size pixels
 *
 * Input Parameters:
 *   Current blob size and adaptive tightness
 *
 * Modified Parameters:
 *   The bitmap is despeckled one iteration
 */

static void
despeckle_iteration (/* in/out */ bitmap_type *bitmap,
                     /* in */     int          level,
                     /* in */     real         adaptive_tightness)
{
  unsigned char *mask;
  int            i;
  int            planes;
  int            width, height;
  int            x, y;
  int            current_size;
  int            tightness;

  planes = BITMAP_PLANES (*bitmap);
  width  = BITMAP_WIDTH (*bitmap);
  height = BITMAP_HEIGHT (*bitmap);

  for (i = 0, current_size = 1; i < level; i++, current_size *= 2);
  tightness = 256 / (int) (1.0 + adaptive_tightness * level);

  XMALLOC (mask, width * height);

  if (planes == 1)
    {
      /* Despeckle the greyscale bitmap */

      for (y = 0; y < height; y++)
        for (x = 0; x < width; x++)
          if (mask[y * width + x] == 0)
            recolor_8 (current_size, tightness, x, y, bitmap, mask);
    }
  else
    {
      /* Despeckle the truecolor bitmap */

      for (y = 0; y < height; y++)
        for (x = 0; x < width; x++)
          if (mask[y * width + x] == 0)
            recolor_24 (current_size, tightness, x, y, bitmap, mask);
    }

  free (mask);
}


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

void
despeckle (/* in/out */ bitmap_type *bitmap,
           /* in */     int          level,
           /* in */     real         tightness)
{
  int i;
  int planes;

  planes = BITMAP_PLANES (*bitmap);

  assert (tightness >= (real) 0.0 && tightness <= (real) 8.0);
  assert (level >= 0 && level <= 20);

  if (planes != 1 && planes != 3)
    {
      WARNING1 ("despeckle: %u-plane images are not supported", planes);
      return;
    }

  for (i = 0; i < level; i++)
    despeckle_iteration (bitmap, i, tightness);
}
