/* despeckle.c: Bitmap despeckler

   Copyright (C) 2001 David A. Bartold / Martin Weber

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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* Def: HAVE_CONFIG_H */

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <time.h>
#include "xstd.h"
#include "logreport.h"
#include "types.h"
#include "bitmap.h"
#include "despeckle.h"

/* Calculate Error - compute the error between two colors
 *
 *   Input parameters:
 *     Two 24 bit RGB colors
 *
 *   Returns:
 *     The squared error between the two colors
 */

static int calc_error(unsigned char *color1, unsigned char *color2)
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

/* Calculate Error - compute the error between two colors
 *
 *   Input parameters:
 *     Two 8 bit gray scale colors
 *
 *   Returns:
 *     The squared error between the two colors
 */

static int calc_error_8(unsigned char *color1, unsigned char *color2)
{
  int the_error;

  the_error = abs(color1[0] - color2[0]);

  return the_error;
}

/* Find Size - Find the number of adjacent pixels of the same color
 *
 * Input Parameters:
 *   An 24 bit image, the current location inside the image, and the palette
 *   index of the color we are looking for
 *
 * Modified Parameters:
 *   A mask array used to prevent backtracking over already counted pixels
 *
 * Returns:
 *   Number of adjacent pixels found having the same color
 */

static int find_size( /* in */ unsigned char *index,
                     /* in */ int x,
                     /* in */ int y,
                     /* in */ int width,
                     /* in */ int height,
                     /* in */ unsigned char *bitmap,
                     /* in/out */ unsigned char *mask)
{
  int count;
  int x1, x2;

  if (y < 0 || y >= height || mask[y * width + x] == 1 || bitmap[3 * (y * width + x)] != index[0] || bitmap[3 * (y * width + x) + 1] != index[1] || bitmap[3 * (y * width + x) + 2] != index[2])
    return 0;

  for (x1 = x; x1 >= 0 && bitmap[3 * (y * width + x1)] == index[0] && bitmap[3 * (y * width + x1) + 1] == index[1] && bitmap[3 * (y * width + x1) + 2] == index[2] && mask[y * width + x] != 1; x1--) ;
  x1++;

  for (x2 = x; x2 < width && bitmap[3 * (y * width + x2)] == index[0] && bitmap[3 * (y * width + x2) + 1] == index[1] && bitmap[3 * (y * width + x2) + 2] == index[2] && mask[y * width + x] != 1; x2++) ;
  x2--;

  count = x2 - x1 + 1;
  for (x = x1; x <= x2; x++)
    mask[y * width + x] = 1;

  for (x = x1; x <= x2; x++) {
    count += find_size(index, x, y - 1, width, height, bitmap, mask);
    count += find_size(index, x, y + 1, width, height, bitmap, mask);
  }

  return count;
}

/* Find Size - Find the number of adjacent pixels of the same color
 *
 * Input Parameters:
 *   An 8 bit image, the current location inside the image, and the palette
 *   index of the color we are looking for
 *
 * Modified Parameters:
 *   A mask array used to prevent backtracking over already counted pixels
 *
 * Returns:
 *   Number of adjacent pixels found having the same color
 */

static int find_size_8( /* in */ unsigned char *index,
                       /* in */ int x,
                       /* in */ int y,
                       /* in */ int width,
                       /* in */ int height,
                       /* in */ unsigned char *bitmap,
                       /* in/out */ unsigned char *mask)
{
  int count;
  int x1, x2;

  if (y < 0 || y >= height || mask[y * width + x] == 1 || bitmap[(y * width + x)] != index[0])
    return 0;

  for (x1 = x; x1 >= 0 && bitmap[(y * width + x1)] == index[0] && mask[y * width + x] != 1; x1--) ;
  x1++;

  for (x2 = x; x2 < width && bitmap[(y * width + x2)] == index[0] && mask[y * width + x] != 1; x2++) ;
  x2--;

  count = x2 - x1 + 1;
  for (x = x1; x <= x2; x++)
    mask[y * width + x] = 1;

  for (x = x1; x <= x2; x++) {
    count += find_size_8(index, x, y - 1, width, height, bitmap, mask);
    count += find_size_8(index, x, y + 1, width, height, bitmap, mask);
  }

  return count;
}

/* Find Most Similar Neighbor - Given a position in an 24 bit bitmap and a color
 * index, traverse over a blob of adjacent pixels having the same value.
 * Return the color index of the neighbor pixel that has the most similar
 * color.
 *
 * Input parameters:
 *   24 bit bitmap, the current location inside the image,
 *   and the color index of the blob
 *
 * Modified parameters:
 *   Mask used to prevent backtracking
 *
 * Output parameters:
 *   Closest index != index and the error between the two colors squared
 */

static void find_most_similar_neighbor( /* in */ unsigned char *index,
                                       /* in/out */ unsigned char **closest_index,
                                       /* in/out */ int *error_amt,
                                       /* in */ int x,
                                       /* in */ int y,
                                       /* in */ int width,
                                       /* in */ int height,
                                       /* in */ unsigned char *bitmap,
                                       /* in/out */ unsigned char *mask)
{
  int x1, x2;
  int temp_error;
  unsigned char *value, *temp;

  if (y < 0 || y >= height || mask[y * width + x] == 2)
    return;

  temp = &bitmap[3 * (y * width + x)];

  assert(closest_index != NULL);

  if (temp[0] != index[0] || temp[1] != index[1] || temp[2] != index[2]) {
    value = temp;

    temp_error = calc_error(index, value);

    if (*closest_index == NULL || temp_error < *error_amt)
      *closest_index = value, *error_amt = temp_error;

    return;
  }

  for (x1 = x; x1 >= 0 && bitmap[3 * (y * width + x1)] == index[0] && bitmap[3 * (y * width + x1) + 1] == index[1] && bitmap[3 * (y * width + x1) + 2] == index[2]; x1--) ;
  x1++;

  for (x2 = x; x2 < width && bitmap[3 * (y * width + x2)] == index[0] && bitmap[3 * (y * width + x2) + 1] == index[1] && bitmap[3 * (y * width + x2) + 2] == index[2]; x2++) ;
  x2--;

  if (x1 > 0) {
    value = &bitmap[3 * (y * width + x1 - 1)];

    temp_error = calc_error(index, value);

    if (*closest_index == NULL || temp_error < *error_amt)
      *closest_index = value, *error_amt = temp_error;
  }

  if (x2 < width - 1) {
    value = &bitmap[3 * (y * width + x2 + 1)];

    temp_error = calc_error(index, value);

    if (*closest_index == NULL || temp_error < *error_amt)
      *closest_index = value, *error_amt = temp_error;
  }

  for (x = x1; x <= x2; x++)
    mask[y * width + x] = 2;

  for (x = x1; x <= x2; x++) {
    find_most_similar_neighbor(index, closest_index, error_amt, x, y - 1, width, height, bitmap, mask);
    find_most_similar_neighbor(index, closest_index, error_amt, x, y + 1, width, height, bitmap, mask);
  }
}

/* Find Most Similar Neighbor - Given a position in an 8 bit bitmap and a color
 * index, traverse over a blob of adjacent pixels having the same value.
 * Return the color index of the neighbor pixel that has the most similar
 * color.
 *
 * Input parameters:
 *   8 bit bitmap, the current location inside the image,
 *   and the color index of the blob
 *
 * Modified parameters:
 *   Mask used to prevent backtracking
 *
 * Output parameters:
 *   Closest index != index and the error between the two colors squared
 */

static void find_most_similar_neighbor_8( /* in */ unsigned char *index,
                                         /* in/out */ unsigned char **closest_index,
                                         /* in/out */ int *error_amt,
                                         /* in */ int x,
                                         /* in */ int y,
                                         /* in */ int width,
                                         /* in */ int height,
                                         /* in */ unsigned char *bitmap,
                                         /* in/out */ unsigned char *mask)
{
  int x1, x2;
  int temp_error;
  unsigned char *value, *temp;

  if (y < 0 || y >= height || mask[y * width + x] == 2)
    return;

  temp = &bitmap[(y * width + x)];

  assert(closest_index != NULL);

  if (temp[0] != index[0]) {
    value = temp;

    temp_error = calc_error_8(index, value);

    if (*closest_index == NULL || temp_error < *error_amt)
      *closest_index = value, *error_amt = temp_error;

    return;
  }

  for (x1 = x; x1 >= 0 && bitmap[(y * width + x1)] == index[0]; x1--) ;
  x1++;

  for (x2 = x; x2 < width && bitmap[(y * width + x2)] == index[0]; x2++) ;
  x2--;

  if (x1 > 0) {
    value = &bitmap[(y * width + x1 - 1)];

    temp_error = calc_error_8(index, value);

    if (*closest_index == NULL || temp_error < *error_amt)
      *closest_index = value, *error_amt = temp_error;
  }

  if (x2 < width - 1) {
    value = &bitmap[(y * width + x2 + 1)];

    temp_error = calc_error_8(index, value);

    if (*closest_index == NULL || temp_error < *error_amt)
      *closest_index = value, *error_amt = temp_error;
  }

  for (x = x1; x <= x2; x++)
    mask[y * width + x] = 2;

  for (x = x1; x <= x2; x++) {
    find_most_similar_neighbor_8(index, closest_index, error_amt, x, y - 1, width, height, bitmap, mask);
    find_most_similar_neighbor_8(index, closest_index, error_amt, x, y + 1, width, height, bitmap, mask);
  }
}

/* Fill - change the color of a blob
 *
 * Input parameters:
 *   The new color
 *
 * Modified parameters:
 *   24 bit pixbuf and its mask (used to prevent backtracking)
 */

static void fill( /* in */ unsigned char *to_index,
                 /* in */ int x,
                 /* in */ int y,
                 /* in */ int width,
                 /* in */ int height,
                 /* in/out */ unsigned char *bitmap,
                 /* in/out */ unsigned char *mask)
{
  int x1, x2;

  if (y < 0 || y >= height || mask[y * width + x] != 2)
    return;

  for (x1 = x; x1 >= 0 && mask[y * width + x1] == 2; x1--) ;
  x1++;
  for (x2 = x; x2 < width && mask[y * width + x2] == 2; x2++) ;
  x2--;

  assert(x1 >= 0 && x2 < width);

  for (x = x1; x <= x2; x++) {
    bitmap[3 * (y * width + x)] = to_index[0];
    bitmap[3 * (y * width + x) + 1] = to_index[1];
    bitmap[3 * (y * width + x) + 2] = to_index[2];
    mask[y * width + x] = 3;
  }

  for (x = x1; x <= x2; x++) {
    fill(to_index, x, y - 1, width, height, bitmap, mask);
    fill(to_index, x, y + 1, width, height, bitmap, mask);
  }
}

/* Fill - change the color of a blob
 *
 * Input parameters:
 *   The new color
 *
 * Modified parameters:
 *   8 bit pixbuf and its mask (used to prevent backtracking)
 */

static void fill_8( /* in */ unsigned char *to_index,
                   /* in */ int x,
                   /* in */ int y,
                   /* in */ int width,
                   /* in */ int height,
                   /* in/out */ unsigned char *bitmap,
                   /* in/out */ unsigned char *mask)
{
  int x1, x2;

  if (y < 0 || y >= height || mask[y * width + x] != 2)
    return;

  for (x1 = x; x1 >= 0 && mask[y * width + x1] == 2; x1--) ;
  x1++;
  for (x2 = x; x2 < width && mask[y * width + x2] == 2; x2++) ;
  x2--;

  assert(x1 >= 0 && x2 < width);

  for (x = x1; x <= x2; x++) {
    bitmap[(y * width + x)] = to_index[0];
    mask[y * width + x] = 3;
  }

  for (x = x1; x <= x2; x++) {
    fill_8(to_index, x, y - 1, width, height, bitmap, mask);
    fill_8(to_index, x, y + 1, width, height, bitmap, mask);
  }
}

/* Ignore - blob is big enough, mask it off
 *
 * Modified parameters:
 *   its mask (used to prevent backtracking)
 */

static void ignore( /* in */ int x,
                   /* in */ int y,
                   /* in */ int width,
                   /* in */ int height,
                   /* in/out */ unsigned char *mask)
{
  int x1, x2;

  if (y < 0 || y >= height || mask[y * width + x] != 1)
    return;

  for (x1 = x; x1 >= 0 && mask[y * width + x1] == 1; x1--) ;
  x1++;
  for (x2 = x; x2 < width && mask[y * width + x2] == 1; x2++) ;
  x2--;

  assert(x1 >= 0 && x2 < width);

  for (x = x1; x <= x2; x++)
    mask[y * width + x] = 3;

  for (x = x1; x <= x2; x++) {
    ignore(x, y - 1, width, height, mask);
    ignore(x, y + 1, width, height, mask);
  }
}

/* Recolor - conditionally change a feature's color to the closest color of all
 * neighboring pixels
 *
 * Input parameters:
 *   The color palette, current blob size, and adaptive tightness
 *
 *   Adaptive Tightness: (integer 1 to 256)
 *     1   = really tight
 *     256 = turn off the feature
 *
 * Modified parameters:
 *   24 bit pixbuf and its mask (used to prevent backtracking)
 *
 * Returns:
 *   TRUE  - feature was recolored, thus coalesced
 *   FALSE - feature wasn't recolored
 */

static gboolean recolor( /* in */ double adaptive_tightness,
                        /* in */ int x,
                        /* in */ int y,
                        /* in */ int width,
                        /* in */ int height,
                        /* in/out */ unsigned char *bitmap,
                        /* in/out */ unsigned char *mask)
{
  unsigned char *index, *to_index;
  int error_amt, max_error;

  index = &bitmap[3 * (y * width + x)];
  to_index = NULL;
  error_amt = 0;
  max_error = (int)(3.0 * adaptive_tightness * adaptive_tightness);

  find_most_similar_neighbor(index, &to_index, &error_amt, x, y, width, height, bitmap, mask);

  /* This condition only fails if the bitmap is all the same color */
  if (to_index != NULL) {
    /*
     * If the difference between the two colors is too great,
     * don't coalesce the feature with its neighbor(s).  This prevents a
     * color from turning into its complement.
     */

    if (calc_error(index, to_index) > max_error)
      fill(index, x, y, width, height, bitmap, mask);
    else {
      fill(to_index, x, y, width, height, bitmap, mask);

      return TRUE;
    }
  }

  return FALSE;
}

/* Recolor - conditionally change a feature's color to the closest color of all
 * neighboring pixels
 *
 * Input parameters:
 *   The color palette, current blob size, and adaptive tightness
 *
 *   Adaptive Tightness: (integer 1 to 256)
 *     1   = really tight
 *     256 = turn off the feature
 *
 * Modified parameters:
 *   8 bit pixbuf and its mask (used to prevent backtracking)
 *
 * Returns:
 *   TRUE  - feature was recolored, thus coalesced
 *   FALSE - feature wasn't recolored
 */

static gboolean recolor_8( /* in */ double adaptive_tightness,
                          /* in */ int x,
                          /* in */ int y,
                          /* in */ int width,
                          /* in */ int height,
                          /* in/out */ unsigned char *bitmap,
                          /* in/out */ unsigned char *mask)
{
  unsigned char *index, *to_index;
  int error_amt;

  index = &bitmap[(y * width + x)];
  to_index = NULL;
  error_amt = 0;

  find_most_similar_neighbor_8(index, &to_index, &error_amt, x, y, width, height, bitmap, mask);

  /* This condition only fails if the bitmap is all the same color */
  if (to_index != NULL) {
    /*
     * If the difference between the two colors is too great,
     * don't coalesce the feature with its neighbor(s).  This prevents a
     * color from turning into its complement.
     */

    if (calc_error_8(index, to_index) > adaptive_tightness)
      fill_8(index, x, y, width, height, bitmap, mask);
    else {
      fill_8(to_index, x, y, width, height, bitmap, mask);

      return TRUE;
    }
  }

  return FALSE;
}

/* Despeckle Iteration - Despeckle all regions smaller than cur_size pixels
 *
 * Input Parameters:
 *   Current blob size, maximum blob size
 *   for all iterations (used to selectively recolor blobs), adaptive
 *   tightness and noise removal
 *
 * Modified Parameters:
 *   The 24 bit pixbuf is despeckled
 */

static void despeckle_iteration( /* in */ int level,
                                /* in */ double adaptive_tightness,
                                /* in */ double noise_max,
                                /* in */ int width,
                                /* in */ int height,
                                /* in/out */ unsigned char *bitmap)
{
  unsigned char *mask;
  int x, y;
  int current_size;
  int tightness;

  /* Size doubles each iteration level, so current_size = 2^level */
  current_size = 1 << level;
  tightness = (int)(noise_max / (1.0 + adaptive_tightness * level));

  mask = (unsigned char *)calloc(width * height, sizeof(unsigned char));
  for (y = 0; y < height; y++) {
    for (x = 0; x < width; x++) {
      if (mask[y * width + x] == 0) {
        int size;

        size = find_size(&bitmap[3 * (y * width + x)], x, y, width, height, bitmap, mask);

        assert(size > 0);

        if (size < current_size) {
          if (recolor(tightness, x, y, width, height, bitmap, mask))
            x--;
        } else
          ignore(x, y, width, height, mask);
      }
    }
  }

  free(mask);
}

/* Despeckle Iteration - Despeckle all regions smaller than cur_size pixels
 *
 * Input Parameters:
 *   Current blob size, maximum blob size
 *   for all iterations (used to selectively recolor blobs), adaptive
 *   tightness and noise removal
 *
 * Modified Parameters:
 *   The 8 bit pixbuf is despeckled
 */

static void despeckle_iteration_8( /* in */ int level,
                                  /* in */ double adaptive_tightness,
                                  /* in */ double noise_max,
                                  /* in */ int width,
                                  /* in */ int height,
                                  /* in/out */ unsigned char *bitmap)
{
  unsigned char *mask;
  int x, y;
  int current_size;
  int tightness;

  /* Size doubles each iteration level, so current_size = 2^level */
  current_size = 1 << level;
  tightness = (int)(noise_max / (1.0 + adaptive_tightness * level));

  mask = (unsigned char *)calloc(width * height, sizeof(unsigned char));
  for (y = 0; y < height; y++) {
    for (x = 0; x < width; x++) {
      if (mask[y * width + x] == 0) {
        int size;

        size = find_size_8(&bitmap[(y * width + x)], x, y, width, height, bitmap, mask);

        assert(size > 0);

        if (size < current_size) {
          if (recolor_8(tightness, x, y, width, height, bitmap, mask))
            x--;
        } else
          ignore(x, y, width, height, mask);
      }
    }
  }

  free(mask);
}

/* Despeckle - Despeckle a 8 or 24 bit image
 *
 * Input Parameters:
 *   Adaptive feature coalescing value, the despeckling level and noise removal
 *
 *   Despeckling level (level): Integer from 0 to ~20
 *     0 = perform no despeckling
 *     An increase of the despeckling level by one doubles the size of features.
 *     The Maximum value must be smaller then the logarithm base two of the number
 *     of pixels.
 *
 *   Feature coalescing (tightness): Real from 0.0 to ~8.0
 *     0 = Turn it off (whites may turn black and vice versa, etc)
 *     3 = Good middle value
 *     8 = Really tight
 *
 *   Noise removal (noise_removal): Real from 1.0 to 0.0
 *     1 = Maximum noise removal
 *     You should always use the highest value, only if certain parts of the image
 *     disappear you should lower it.
 *
 * Modified Parameters:
 *   The bitmap is despeckled.
 */

void despeckle( /* in/out */ at_bitmap * bitmap,
               /* in */ int level,
               /* in */ gfloat tightness,
               /* in */ gfloat noise_removal,
               /* exception handling */ at_exception_type * excep)
{
  int i, planes, max_level;
  short width, height;
  unsigned char *bits;
  double noise_max, adaptive_tightness;

  planes = AT_BITMAP_PLANES(bitmap);
  noise_max = noise_removal * 255.0;
  width = AT_BITMAP_WIDTH(bitmap);
  height = AT_BITMAP_HEIGHT(bitmap);
  bits = AT_BITMAP_BITS(bitmap);
  max_level = (int)(log(width * height) / log(2.0) - 0.5);
  if (level > max_level)
    level = max_level;
  adaptive_tightness = (noise_removal * (1.0 + tightness * level) - 1.0) / level;

  if (planes == 3) {
    for (i = 0; i < level; i++)
      despeckle_iteration(i, adaptive_tightness, noise_max, width, height, bits);
  } else if (planes == 1) {
    for (i = 0; i < level; i++)
      despeckle_iteration_8(i, adaptive_tightness, noise_max, width, height, bits);
  } else {
    LOG("despeckle: %u-plane images are not supported", planes);
    at_exception_fatal(excep, "despeckle: wrong plane images are passed");
    return;
  }

}
