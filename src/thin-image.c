/* thin-image.c: thin binary image

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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* Def: HAVE_CONFIG_H */

#include <stdlib.h>
#include <stdio.h>
#include "thin-image.h"
#include "logreport.h"
#include "types.h"
#include "bitmap.h"
#include "xstd.h"
#include <string.h>

#define PIXEL_SET(p, new)  ((void)memcpy((p), (new), sizeof(Pixel)))
#define PIXEL_EQUAL(p1, p2) \
    ((p1)[0] == (p2)[0] && (p1)[1] == (p2)[1] && (p1)[2] == (p2)[2])

typedef unsigned char Pixel[3]; /* RGB pixel data type */

void thin3(at_bitmap * image, Pixel colour);
void thin1(at_bitmap * image, unsigned char colour);

/* -------------------------------- ThinImage - Thin binary image. --------------------------- *
 *
 *    Description:
 *        Thins the supplied binary image using Rosenfeld's parallel
 *        thinning algorithm.
 *
 *    On Entry:
 *        image = Image to thin.
 *
 * -------------------------------------------------------------------------------------------- */

/* Direction masks:                  */
/*   N     S     W        E            */
static unsigned int masks[] = { 0200, 0002, 0040, 0010 };

/*    True if pixel neighbor map indicates the pixel is 8-simple and  */
/*    not an end point and thus can be deleted.  The neighborhood     */
/*    map is defined as an integer of bits abcdefghi with a non-zero  */
/*    bit representing a non-zero pixel.  The bit assignment for the  */
/*    neighborhood is:                                                */
/*                                                                    */
/*                            a b c                                   */
/*                            d e f                                   */
/*                            g h i                                   */

static unsigned char todelete[512] = {
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 1, 0, 0, 1, 1, 0, 1, 1, 1, 0, 0, 1, 1,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 1, 1, 1, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 1, 1,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 1, 1,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 1, 1,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  1, 0, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  1, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 1, 1,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  1, 0, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  1, 0, 1, 1, 1, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 1, 1,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  1, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 1, 1,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  1, 0, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  1, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 1, 1,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  1, 0, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1
};

static at_color background = { 0xff, 0xff, 0xff };

void thin_image(at_bitmap * image, const at_color * bg, at_exception_type * exp)
{
  /* This is nasty as we need to call thin once for each
   * colour in the image the way I do this is to keep a second
   * copy of the bitmap and to use this to keep
   * track of which colours have not yet been processed,
   * trades time for pathological case memory.....*/
  long m, n, num_pixels;
  at_bitmap bm;
  unsigned int spp = AT_BITMAP_PLANES(image), width = AT_BITMAP_WIDTH(image), height = AT_BITMAP_HEIGHT(image);

  if (bg)
    background = *bg;

  bm.height = image->height;
  bm.width = image->width;
  bm.np = image->np;
  XMALLOC(bm.bitmap, height * width * spp);
  memcpy(bm.bitmap, image->bitmap, height * width * spp);
  /* that clones the image */

  num_pixels = height * width;
  switch (spp) {
  case 3:
    {
      Pixel *ptr = (Pixel *) AT_BITMAP_BITS(&bm);
      Pixel bg_color;
      bg_color[0] = background.r;
      bg_color[1] = background.g;
      bg_color[2] = background.b;

      for (n = num_pixels - 1; n >= 0L; --n) {
        Pixel p;

        PIXEL_SET(p, ptr[n]);
        if (!PIXEL_EQUAL(p, bg_color)) {
          /* we have a new colour in the image */
          LOG("Thinning colour (%x, %x, %x)\n", p[0], p[1], p[2]);
          for (m = n - 1; m >= 0L; --m) {
            if (PIXEL_EQUAL(ptr[m], p))
              PIXEL_SET(ptr[m], bg_color);
          }
          thin3(image, p);
        }
      }
      break;
    }

  case 1:
    {
      unsigned char *ptr = AT_BITMAP_BITS(&bm);
      unsigned char bg_color;

      if (background.r == background.g && background.g == background.b)
        bg_color = background.r;
      else
        bg_color = at_color_luminance(&background);

      for (n = num_pixels - 1; n >= 0L; --n) {
        unsigned char c = ptr[n];
        if (c != bg_color) {
          LOG("Thinning colour %x\n", c);
          for (m = n - 1; m >= 0L; --m)
            if (ptr[m] == c)
              ptr[m] = bg_color;
          thin1(image, c);
        }
      }
      break;
    }

  default:
    {
      LOG("thin_image: %u-plane images are not supported", spp);
      at_exception_fatal(exp, "thin_image: wrong plane images are passed");
      goto cleanup;
    }
  }
cleanup:
  free(bm.bitmap);
}

void thin3(at_bitmap * image, Pixel colour)
{
  Pixel *ptr, *y_ptr, *y1_ptr;
  Pixel bg_color;
  unsigned int xsize, ysize;    /* Image resolution             */
  unsigned int x, y;            /* Pixel location               */
  unsigned int i;               /* Pass index           */
  unsigned int pc = 0;          /* Pass count           */
  unsigned int count = 1;       /* Deleted pixel count          */
  unsigned int p, q;            /* Neighborhood maps of adjacent */
  /* cells                        */
  unsigned char *qb;            /* Neighborhood maps of previous */
  /* scanline                     */
  unsigned int m;               /* Deletion direction mask      */

  bg_color[0] = background.r;
  bg_color[1] = background.g;
  bg_color[2] = background.b;

  LOG(" Thinning image.....\n ");
  xsize = AT_BITMAP_WIDTH(image);
  ysize = AT_BITMAP_HEIGHT(image);
  XMALLOC(qb, xsize * sizeof(unsigned char));
  qb[xsize - 1] = 0;            /* Used for lower-right pixel   */
  ptr = (Pixel *) AT_BITMAP_BITS(image);

  while (count) {               /* Scan image while deletions   */
    pc++;
    count = 0;

    for (i = 0; i < 4; i++) {

      m = masks[i];

      /* Build initial previous scan buffer.                  */
      p = PIXEL_EQUAL(ptr[0], colour);
      for (x = 0; x < xsize - 1; x++)
        qb[x] = (unsigned char)(p = ((p << 1) & 0006) | (unsigned int)PIXEL_EQUAL(ptr[x + 1], colour));

      /* Scan image for pixel deletion candidates.            */
      y_ptr = ptr;
      y1_ptr = ptr + xsize;
      for (y = 0; y < ysize - 1; y++, y_ptr += xsize, y1_ptr += xsize) {
        q = qb[0];
        p = ((q << 2) & 0330) | (unsigned int)PIXEL_EQUAL(y1_ptr[0], colour);

        for (x = 0; x < xsize - 1; x++) {
          q = qb[x];
          p = ((p << 1) & 0666) | ((q << 3) & 0110) | (unsigned int)PIXEL_EQUAL(y1_ptr[x + 1], colour);
          qb[x] = (unsigned char)p;
          if ((i != 2 || x != 0) && ((p & m) == 0) && todelete[p]) {
            count++;            /* delete the pixel */
            PIXEL_SET(y_ptr[x], bg_color);
          }
        }

        /* Process right edge pixel.                        */
        p = (p << 1) & 0666;
        if (i != 3 && (p & m) == 0 && todelete[p]) {
          count++;
          PIXEL_SET(y_ptr[xsize - 1], bg_color);
        }
      }

      if (i != 1) {
        /* Process bottom scan line.                            */
        q = qb[0];
        p = ((q << 2) & 0330);

        y_ptr = ptr + xsize * (ysize - 1);
        for (x = 0; x < xsize; x++) {
          q = qb[x];
          p = ((p << 1) & 0666) | ((q << 3) & 0110);
          if ((i != 2 || x != 0) && (p & m) == 0 && todelete[p]) {
            count++;
            PIXEL_SET(y_ptr[x], bg_color);
          }
        }
      }
    }
    LOG("ThinImage: pass %d, %d pixels deleted\n", pc, count);
  }
  free(qb);
}

void thin1(at_bitmap * image, unsigned char colour)
{
  unsigned char *ptr, *y_ptr, *y1_ptr;
  unsigned char bg_color;
  unsigned int xsize, ysize;    /* Image resolution             */
  unsigned int x, y;            /* Pixel location               */
  unsigned int i;               /* Pass index           */
  unsigned int pc = 0;          /* Pass count           */
  unsigned int count = 1;       /* Deleted pixel count          */
  unsigned int p, q;            /* Neighborhood maps of adjacent */
  /* cells                        */
  unsigned char *qb;            /* Neighborhood maps of previous */
  /* scanline                     */
  unsigned int m;               /* Deletion direction mask      */

  if (background.r == background.g && background.g == background.b)
    bg_color = background.r;
  else
    bg_color = at_color_luminance(&background);

  LOG(" Thinning image.....\n ");
  xsize = AT_BITMAP_WIDTH(image);
  ysize = AT_BITMAP_HEIGHT(image);
  XMALLOC(qb, xsize * sizeof(unsigned char));
  qb[xsize - 1] = 0;            /* Used for lower-right pixel   */
  ptr = AT_BITMAP_BITS(image);

  while (count) {               /* Scan image while deletions   */
    pc++;
    count = 0;

    for (i = 0; i < 4; i++) {

      m = masks[i];

      /* Build initial previous scan buffer.                  */
      p = (ptr[0] == colour);
      for (x = 0; x < xsize - 1; x++)
        qb[x] = (unsigned char)(p = ((p << 1) & 0006) | (unsigned int)(ptr[x + 1] == colour));

      /* Scan image for pixel deletion candidates.            */
      y_ptr = ptr;
      y1_ptr = ptr + xsize;
      for (y = 0; y < ysize - 1; y++, y_ptr += xsize, y1_ptr += xsize) {
        q = qb[0];
        p = ((q << 2) & 0330) | (y1_ptr[0] == colour);

        for (x = 0; x < xsize - 1; x++) {
          q = qb[x];
          p = ((p << 1) & 0666) | ((q << 3) & 0110) | (unsigned int)(y1_ptr[x + 1] == colour);
          qb[x] = (unsigned char)p;
          if (((p & m) == 0) && todelete[p]) {
            count++;
            y_ptr[x] = bg_color;  /* delete the pixel */
          }
        }

        /* Process right edge pixel.                        */
        p = (p << 1) & 0666;
        if ((p & m) == 0 && todelete[p]) {
          count++;
          y_ptr[xsize - 1] = bg_color;
        }
      }

      /* Process bottom scan line.                            */
      q = qb[0];
      p = ((q << 2) & 0330);

      y_ptr = ptr + xsize * (ysize - 1);
      for (x = 0; x < xsize; x++) {
        q = qb[x];
        p = ((p << 1) & 0666) | ((q << 3) & 0110);
        if ((p & m) == 0 && todelete[p]) {
          count++;
          y_ptr[x] = bg_color;
        }
      }
    }
    LOG("thin1: pass %d, %d pixels deleted\n", pc, count);
  }
  free(qb);
}
