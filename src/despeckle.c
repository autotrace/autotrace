/*
 * Copyright (C) 2001 David A. Bartold / Martin Weber
 * SPDX-FileCopyrightText: © 2001 David A. Bartold
 * SPDX-FileCopyrightText: © 2001-2003 Martin Weber
 * SPDX-FileCopyrightText: © 2001-2003 Masatake YAMATO
 * SPDX-FileCopyrightText: © 2017-2025 Peter Lemenkov
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "logreport.h"
#include "despeckle.h"
#include <glib.h>

/* The flood fills below walk a blob one horizontal scanline segment at a
 * time and then visit the rows above and below every pixel of the segment.
 * Doing that recursively needs one stack frame per segment, which overflows
 * the stack on large blobs (github #154).  Instead the pending segments are
 * kept on an explicit stack: a span_frame remembers a segment [x1, x2] on
 * row y and which neighbour (x, y + dy) is due next.  span_stack_next()
 * hands out the neighbours in exactly the order the recursive calls were
 * made, so the traversal, and thus the result, is unchanged.
 */
typedef struct {
  int x1, x2, y;
  int x;  /* column of the next neighbour to visit */
  int dy; /* -1: the row above is next, +1: the row below is next */
} span_frame;

static void span_stack_push(GArray *stack, int x1, int x2, int y)
{
  span_frame frame = {x1, x2, y, x1, -1};

  g_array_append_val(stack, frame);
}

/* Fetch the next (x, y) to visit.  Returns FALSE once every pending segment
 * has been fully processed.
 */
static gboolean span_stack_next(GArray *stack, int *x, int *y)
{
  while (stack->len > 0) {
    span_frame *frame = &g_array_index(stack, span_frame, stack->len - 1);

    if (frame->x > frame->x2) {
      g_array_set_size(stack, stack->len - 1);
      continue;
    }

    *x = frame->x;
    *y = frame->y + frame->dy;

    if (frame->dy < 0)
      frame->dy = 1;
    else {
      frame->dy = -1;
      frame->x++;
    }

    return TRUE;
  }

  return FALSE;
}

/* The same code handles 8 bit gray and 24 bit RGB images: PLANES is 1 or 3,
 * a pixel is PLANES consecutive bytes, and a "color index" is a pointer to
 * one such pixel.
 */
#define PIXEL(bitmap, planes, width, x, y) ((bitmap) + (planes) * ((y) * (width) + (x)))

static gboolean same_color(int planes, const unsigned char *a, const unsigned char *b)
{
  return memcmp(a, b, planes) == 0;
}

/* Copy a color, byte by byte because source and destination may be the
 * same pixel.
 */
static void set_color(int planes, unsigned char *pixel, const unsigned char *color)
{
  int i;

  for (i = 0; i < planes; i++)
    pixel[i] = color[i];
}

/* Calculate Error - compute the error between two colors
 *
 *   Input parameters:
 *     Two 24 bit RGB colors, or two 8 bit gray levels
 *
 *   Returns:
 *     For RGB the squared distance between the two colors; for gray the
 *     absolute difference.  (These are the two metrics the separate RGB
 *     and gray implementations always used.)
 */
static int calc_error(int planes, const unsigned char *color1, const unsigned char *color2)
{
  int the_error = 0;
  int i;

  if (planes == 1)
    return abs(color1[0] - color2[0]);

  for (i = 0; i < planes; i++) {
    int temp = color1[i] - color2[i];

    the_error += temp * temp;
  }

  return the_error;
}

/* Find Size - Find the number of adjacent pixels of the same color
 *
 * Input Parameters:
 *   The image, the current location inside the image, and the palette
 *   index of the color we are looking for
 *
 * Modified Parameters:
 *   A mask array used to prevent backtracking over already counted pixels
 *
 * Returns:
 *   Number of adjacent pixels found having the same color
 */
static int find_size(/* in */ int planes,
                     /* in */ unsigned char *index,
                     /* in */ int x,
                     /* in */ int y,
                     /* in */ int width,
                     /* in */ int height,
                     /* in */ unsigned char *bitmap,
                     /* in/out */ unsigned char *mask)
{
  int count = 0;
  int x1, x2;
  g_autoptr(GArray) stack = g_array_new(FALSE, FALSE, sizeof(span_frame));

  do {
    if (y < 0 || y >= height || mask[y * width + x] == 1 ||
        !same_color(planes, PIXEL(bitmap, planes, width, x, y), index))
      continue;

    for (x1 = x; x1 >= 0 && same_color(planes, PIXEL(bitmap, planes, width, x1, y), index) &&
                 mask[y * width + x1] != 1;
         x1--)
      ;
    x1++;

    for (x2 = x; x2 < width && same_color(planes, PIXEL(bitmap, planes, width, x2, y), index) &&
                 mask[y * width + x2] != 1;
         x2++)
      ;
    x2--;

    count += x2 - x1 + 1;
    for (x = x1; x <= x2; x++)
      mask[y * width + x] = 1;

    span_stack_push(stack, x1, x2, y);
  } while (span_stack_next(stack, &x, &y));

  return count;
}

/* Find Most Similar Neighbor - Given a position in a bitmap and a color
 * index, traverse over a blob of adjacent pixels having the same value.
 * Return the color index of the neighbor pixel that has the most similar
 * color.
 *
 * Input parameters:
 *   The bitmap, the current location inside the image,
 *   and the color index of the blob
 *
 * Modified parameters:
 *   Mask used to prevent backtracking
 *
 * Output parameters:
 *   Closest index != index and the error between the two colors
 */
static void find_most_similar_neighbor(/* in */ int planes,
                                       /* in */ unsigned char *index,
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
  g_autoptr(GArray) stack = g_array_new(FALSE, FALSE, sizeof(span_frame));

  assert(closest_index != NULL);

  do {
    if (y < 0 || y >= height || mask[y * width + x] == 2)
      continue;

    temp = PIXEL(bitmap, planes, width, x, y);

    if (!same_color(planes, temp, index)) {
      value = temp;

      temp_error = calc_error(planes, index, value);

      if (*closest_index == NULL || temp_error < *error_amt)
        *closest_index = value, *error_amt = temp_error;

      continue;
    }

    for (x1 = x; x1 >= 0 && same_color(planes, PIXEL(bitmap, planes, width, x1, y), index); x1--)
      ;
    x1++;

    for (x2 = x; x2 < width && same_color(planes, PIXEL(bitmap, planes, width, x2, y), index); x2++)
      ;
    x2--;

    if (x1 > 0) {
      value = PIXEL(bitmap, planes, width, x1 - 1, y);

      temp_error = calc_error(planes, index, value);

      if (*closest_index == NULL || temp_error < *error_amt)
        *closest_index = value, *error_amt = temp_error;
    }

    if (x2 < width - 1) {
      value = PIXEL(bitmap, planes, width, x2 + 1, y);

      temp_error = calc_error(planes, index, value);

      if (*closest_index == NULL || temp_error < *error_amt)
        *closest_index = value, *error_amt = temp_error;
    }

    for (x = x1; x <= x2; x++)
      mask[y * width + x] = 2;

    span_stack_push(stack, x1, x2, y);
  } while (span_stack_next(stack, &x, &y));
}

/* Fill - change the color of a blob
 *
 * Input parameters:
 *   The new color
 *
 * Modified parameters:
 *   The pixbuf and its mask (used to prevent backtracking)
 */
static void fill(/* in */ int planes,
                 /* in */ unsigned char *to_index,
                 /* in */ int x,
                 /* in */ int y,
                 /* in */ int width,
                 /* in */ int height,
                 /* in/out */ unsigned char *bitmap,
                 /* in/out */ unsigned char *mask)
{
  int x1, x2;
  g_autoptr(GArray) stack = g_array_new(FALSE, FALSE, sizeof(span_frame));

  do {
    if (y < 0 || y >= height || mask[y * width + x] != 2)
      continue;

    for (x1 = x; x1 >= 0 && mask[y * width + x1] == 2; x1--)
      ;
    x1++;
    for (x2 = x; x2 < width && mask[y * width + x2] == 2; x2++)
      ;
    x2--;

    assert(x1 >= 0 && x2 < width);

    for (x = x1; x <= x2; x++) {
      set_color(planes, PIXEL(bitmap, planes, width, x, y), to_index);
      mask[y * width + x] = 3;
    }

    span_stack_push(stack, x1, x2, y);
  } while (span_stack_next(stack, &x, &y));
}

/* Ignore - blob is big enough, mask it off
 *
 * Modified parameters:
 *   its mask (used to prevent backtracking)
 */
static void ignore(/* in */ int x,
                   /* in */ int y,
                   /* in */ int width,
                   /* in */ int height,
                   /* in/out */ unsigned char *mask)
{
  int x1, x2;
  g_autoptr(GArray) stack = g_array_new(FALSE, FALSE, sizeof(span_frame));

  do {
    if (y < 0 || y >= height || mask[y * width + x] != 1)
      continue;

    for (x1 = x; x1 >= 0 && mask[y * width + x1] == 1; x1--)
      ;
    x1++;
    for (x2 = x; x2 < width && mask[y * width + x2] == 1; x2++)
      ;
    x2--;

    assert(x1 >= 0 && x2 < width);

    for (x = x1; x <= x2; x++)
      mask[y * width + x] = 3;

    span_stack_push(stack, x1, x2, y);
  } while (span_stack_next(stack, &x, &y));
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
 *   The pixbuf and its mask (used to prevent backtracking)
 *
 * Returns:
 *   TRUE  - feature was recolored, thus coalesced
 *   FALSE - feature wasn't recolored
 */
static gboolean recolor(/* in */ int planes,
                        /* in */ double adaptive_tightness,
                        /* in */ int x,
                        /* in */ int y,
                        /* in */ int width,
                        /* in */ int height,
                        /* in/out */ unsigned char *bitmap,
                        /* in/out */ unsigned char *mask)
{
  unsigned char *index, *to_index;
  int error_amt;
  double max_error;

  index = PIXEL(bitmap, planes, width, x, y);
  to_index = NULL;
  error_amt = 0;

  /* The RGB error is a squared distance and the gray error a plain
   * difference, so the thresholds differ accordingly; the RGB one has
   * always been truncated to an integer. */
  if (planes == 1)
    max_error = adaptive_tightness;
  else
    max_error = (int)(3.0 * adaptive_tightness * adaptive_tightness);

  find_most_similar_neighbor(planes, index, &to_index, &error_amt, x, y, width, height, bitmap,
                             mask);

  /* This condition only fails if the bitmap is all the same color */
  if (to_index != NULL) {
    /*
     * If the difference between the two colors is too great,
     * don't coalesce the feature with its neighbor(s).  This prevents a
     * color from turning into its complement.
     */

    if (calc_error(planes, index, to_index) > max_error)
      fill(planes, index, x, y, width, height, bitmap, mask);
    else {
      fill(planes, to_index, x, y, width, height, bitmap, mask);

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
 *   The pixbuf is despeckled
 */
static void despeckle_iteration(/* in */ int planes,
                                /* in */ int level,
                                /* in */ double adaptive_tightness,
                                /* in */ double noise_max,
                                /* in */ int width,
                                /* in */ int height,
                                /* in/out */ unsigned char *bitmap)
{
  int x, y;
  int current_size;
  int tightness;

  /* Size doubles each iteration level, so current_size = 2^level */
  current_size = 1 << level;
  tightness = (int)(noise_max / (1.0 + adaptive_tightness * level));

  g_autofree unsigned char *mask = g_malloc0((gsize)width * height * sizeof(unsigned char));
  for (y = 0; y < height; y++) {
    for (x = 0; x < width; x++) {
      if (mask[y * width + x] == 0) {
        int size;

        size = find_size(planes, PIXEL(bitmap, planes, width, x, y), x, y, width, height, bitmap,
                         mask);

        assert(size > 0);

        if (size < current_size) {
          if (recolor(planes, tightness, x, y, width, height, bitmap, mask))
            x--;
        } else
          ignore(x, y, width, height, mask);
      }
    }
  }
}

/* Despeckle - Despeckle a 8 or 24 bit image
 *
 * Input Parameters:
 *   Adaptive feature coalescing value, the despeckling level and noise removal
 *
 *   Despeckling level (level): Integer from 0 to ~20
 *     0 = perform no despeckling
 *     An increase of the despeckle level by one doubles the size of features.
 *     The Maximum value must be smaller then the logarithm base two of the
 *     number of pixels.
 *
 *   Feature coalescing (tightness): Real from 0.0 to ~8.0
 *     0 = Turn it off (whites may turn black and vice versa, etc)
 *     3 = Good middle value
 *     8 = Really tight
 *
 *   Noise removal (noise_removal): Real from 1.0 to 0.0
 *     1 = Maximum noise removal
 *     You should always use the highest value, only if certain parts of the
 *     image disappear you should lower it.
 *
 * Modified Parameters:
 *   The bitmap is despeckled.
 */
void despeckle(/* in/out */ at_bitmap *bitmap,
               /* in */ int level,
               /* in */ gfloat tightness,
               /* in */ gfloat noise_removal,
               /* exception handling */ at_exception_type *excep)
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

  if (planes != 3 && planes != 1) {
    LOG("despeckle: %u-plane images are not supported", planes);
    at_exception_fatal(excep, "despeckle: wrong plane images are passed");
    return;
  }

  for (i = 0; i < level; i++)
    despeckle_iteration(planes, i, adaptive_tightness, noise_max, width, height, bits);
}
