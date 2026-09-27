/*
 * SPDX-FileCopyrightText: © 2002-2003 Masatake YAMATO
 * SPDX-FileCopyrightText: © 2017-2025 Peter Lemenkov
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

/* image-proc.c: image processing routines */

#include <assert.h>
#include <glib.h>
#include "logreport.h"
#include "image-proc.h"

#ifndef M_SQRT2
#define M_SQRT2 1.41421356237
#endif

/* RGB to grayscale */
#define LUMINANCE(r, g, b) ((r) * 0.30 + (g) * 0.59 + (b) * 0.11 + 0.5)

/* Allocate storage for a new distance map with the same dimensions
   as BITMAP and initialize it so that pixels in BITMAP with value
   TARGET_VALUE are at distance zero and all other pixels are at
   distance infinity.  Then compute the gray-weighted distance from
   every non-target point to the nearest target point. */

at_distance_map new_distance_map(at_bitmap *bitmap, unsigned char target_value, gboolean padded,
                                 at_exception_type *exp)
{
  signed x, y;
  float d, min;
  at_distance_map dist;
  unsigned char *b = AT_BITMAP_BITS(bitmap);
  unsigned w = AT_BITMAP_WIDTH(bitmap);
  unsigned h = AT_BITMAP_HEIGHT(bitmap);
  unsigned spp = AT_BITMAP_PLANES(bitmap);

  dist.height = h;
  dist.width = w;
  dist.d = g_new(float *, h);
  dist.weight = g_new(float *, h);
  for (y = 0; y < (signed)h; y++) {
    dist.d[y] = g_new0(float, w);
    dist.weight[y] = g_new(float, w);
  }

  if (spp == 3) {
    for (y = 0; y < (signed)h; y++) {
      for (x = 0; x < (signed)w; x++, b += spp) {
        int gray;
        float fgray;
        gray = (int)LUMINANCE(b[0], b[1], b[2]);
        dist.d[y][x] = (gray == target_value ? 0.0F : 1.0e10F);
        fgray = gray * 0.0039215686F; /* = gray / 255.0F */
        dist.weight[y][x] = 1.0F - fgray;
        /*        dist.weight[y][x] = 1.0F - (fgray * fgray);*/
        /*        dist.weight[y][x] = (fgray < 0.5F ? 1.0F - fgray : -2.0F * fgray * (fgray
         * - 1.0F));*/
      }
    }
  } else {
    for (y = 0; y < (signed)h; y++) {
      for (x = 0; x < (signed)w; x++, b += spp) {
        int gray;
        float fgray;
        gray = b[0];
        dist.d[y][x] = (gray == target_value ? 0.0F : 1.0e10F);
        fgray = gray * 0.0039215686F; /* = gray / 255.0F */
        dist.weight[y][x] = 1.0F - fgray;
        /*        dist.weight[y][x] = 1.0F - (fgray * fgray);*/
        /*        dist.weight[y][x] = (fgray < 0.5F ? 1.0F - fgray : -2.0F * fgray * (fgray
         * - 1.0F)); */
      }
    }
  }

  /* If the image is padded then border points are all at most
     one unit away from the nearest target point. */
  if (padded) {
    for (y = 0; y < (signed)h; y++) {
      if (dist.d[y][0] > dist.weight[y][0])
        dist.d[y][0] = dist.weight[y][0];
      if (dist.d[y][w - 1] > dist.weight[y][w - 1])
        dist.d[y][w - 1] = dist.weight[y][w - 1];
    }
    for (x = 0; x < (signed)w; x++) {
      if (dist.d[0][x] > dist.weight[0][x])
        dist.d[0][x] = dist.weight[0][x];
      if (dist.d[h - 1][x] > dist.weight[h - 1][x])
        dist.d[h - 1][x] = dist.weight[h - 1][x];
    }
  }

  /* Scan the image from left to right, top to bottom.
     Examine the already-visited neighbors of each point (those
     situated above or to the left of it).  Each neighbor knows
     the distance to its nearest target point; add to this distance
     the distance from the central point to the neighbor (either
     sqrt(2) or one) multiplied by the central point's weight
     (derived from its gray level).  Replace the distance already
     stored at the central point if the new distance is smaller. */
  for (y = 1; y < (signed)h; y++) {
    for (x = 1; x < (signed)w; x++) {
      if (dist.d[y][x] == 0.0F)
        continue;

      min = dist.d[y][x];

      /* upper-left neighbor */
      d = dist.d[y - 1][x - 1] + (float)M_SQRT2 * dist.weight[y][x];
      if (d < min)
        min = dist.d[y][x] = d;

      /* upper neighbor */
      d = dist.d[y - 1][x] + dist.weight[y][x];
      if (d < min)
        min = dist.d[y][x] = d;

      /* left neighbor */
      d = dist.d[y][x - 1] + dist.weight[y][x];
      if (d < min)
        min = dist.d[y][x] = d;

      /* upper-right neighbor (except at the last column) */
      if (x + 1 < (signed)w) {
        d = dist.d[y - 1][x + 1] + (float)M_SQRT2 * dist.weight[y][x];
        if (d < min)
          min = dist.d[y][x] = d;
      }
    }
  }

  /* Same as above, but now scanning right to left, bottom to top. */
  for (y = h - 2; y >= 0; y--) {
    for (x = w - 2; x >= 0; x--) {
      min = dist.d[y][x];

      /* lower-right neighbor */
      d = dist.d[y + 1][x + 1] + (float)M_SQRT2 * dist.weight[y][x];
      if (d < min)
        min = dist.d[y][x] = d;

      /* lower neighbor */
      d = dist.d[y + 1][x] + dist.weight[y][x];
      if (d < min)
        min = dist.d[y][x] = d;

      /* right neighbor */
      d = dist.d[y][x + 1] + dist.weight[y][x];
      if (d < min)
        min = dist.d[y][x] = d;

      /* lower-left neighbor (except at the first column) */
      if (x - 1 >= 0) {
        d = dist.d[y + 1][x - 1] + (float)M_SQRT2 * dist.weight[y][x];
        if (d < min)
          min = dist.d[y][x] = d;
      }
    }
  }
  return dist;
}

/* Free the dynamically-allocated storage associated with a distance map. */

void free_distance_map(at_distance_map *dist)
{
  unsigned y, h;

  if (!dist)
    return;

  h = dist->height;

  if (dist->d != NULL) {
    for (y = 0; y < h; y++)
      g_free((gpointer *)dist->d[y]);
    g_free((gpointer *)dist->d);
  }
  if (dist->weight != NULL) {
    for (y = 0; y < h; y++)
      g_free((gpointer *)dist->weight[y]);
    g_free((gpointer *)dist->weight);
  }
}
