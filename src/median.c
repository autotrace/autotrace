/* median.c: median cut - reducing a high color bitmap to certain number of colors

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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "logreport.h"
#include "xstd.h"
#include "quantize.h"

#define MAXNUMCOLORS 256

#if 0
#define R_SCALE
#define G_SCALE
#define B_SCALE
#else

/* scale RGB distances by *2,*3,*1 */
#define R_SCALE  <<1
#define G_SCALE  *3
#define B_SCALE
#endif

#define BITS_IN_SAMPLE	8

#define R_SHIFT	(BITS_IN_SAMPLE - PRECISION_R)
#define G_SHIFT	(BITS_IN_SAMPLE - PRECISION_G)
#define B_SHIFT	(BITS_IN_SAMPLE - PRECISION_B)

typedef struct {
  /* The bounds of the box (inclusive); expressed as histogram indexes */
  int Rmin, Rmax;
  int Gmin, Gmax;
  int Bmin, Bmax;
  /* The volume (actually 2-norm) of the box */
  int volume;
  /* The number of nonzero histogram cells within this box */
  long colorcount;
} box, *boxptr;

static void zero_histogram_rgb(Histogram histogram)
{
  int r, g, b;
  for (r = 0; r < HIST_R_ELEMS; r++)
    for (g = 0; g < HIST_G_ELEMS; g++)
      for (b = 0; b < HIST_B_ELEMS; b++)
        histogram[r * MR + g * MG + b] = 0;
}

static void generate_histogram_rgb(Histogram histogram, at_bitmap * image, const at_color * ignoreColor)
{
  unsigned char *src = image->bitmap;
  int num_elems;
  ColorFreq *col;

  num_elems = AT_BITMAP_WIDTH(image) * AT_BITMAP_HEIGHT(image);
  zero_histogram_rgb(histogram);

  switch (AT_BITMAP_PLANES(image)) {
  case 3:
    while (num_elems--) {
      /* If we have an ignorecolor, skip it. */
      if (ignoreColor) {
        if ((src[0] == ignoreColor->r)
            && (src[1] == ignoreColor->g)
            && (src[2] == ignoreColor->b)) {
          src += 3;
          continue;
        }
      }
      col = &histogram[(src[0] >> R_SHIFT) * MR + (src[1] >> G_SHIFT) * MG + (src[2] >> B_SHIFT)];
      (*col)++;
      src += 3;
    }
    break;

  case 1:
    while (--num_elems >= 0) {
      if (ignoreColor && src[num_elems] == ignoreColor->r)
        continue;
      col = &histogram[(src[num_elems] >> R_SHIFT) * MR + (src[num_elems] >> G_SHIFT) * MG + (src[num_elems] >> B_SHIFT)];
      (*col)++;
    }
    break;
  default:
    /* To avoid compiler warning */ ;
  }
}

static boxptr find_biggest_volume(boxptr boxlist, int numboxes)
/* Find the splittable box with the largest (scaled) volume */
/* Returns 0 if no splittable boxes remain */
{
  boxptr boxp;
  int i;
  int maxv = 0;
  boxptr which = 0;

  for (i = 0, boxp = boxlist; i < numboxes; i++, boxp++) {
    if (boxp->volume > maxv) {
      which = boxp;
      maxv = boxp->volume;
    }
  }

  return which;
}

static void update_box_rgb(Histogram histogram, boxptr boxp)
/* Shrink the min/max bounds of a box to enclose only nonzero elements, */
/* and recompute its volume and population */
{
  ColorFreq *histp;
  int R, G, B;
  int Rmin, Rmax, Gmin, Gmax, Bmin, Bmax;
  int dist0, dist1, dist2;
  long ccount;

  Rmin = boxp->Rmin;
  Rmax = boxp->Rmax;
  Gmin = boxp->Gmin;
  Gmax = boxp->Gmax;
  Bmin = boxp->Bmin;
  Bmax = boxp->Bmax;

  if (Rmax > Rmin)
    for (R = Rmin; R <= Rmax; R++)
      for (G = Gmin; G <= Gmax; G++) {
        histp = histogram + R * MR + G * MG + Bmin;
        for (B = Bmin; B <= Bmax; B++)
          if (*histp++ != 0) {
            boxp->Rmin = Rmin = R;
            goto have_Rmin;
          }
      }
have_Rmin:
  if (Rmax > Rmin)
    for (R = Rmax; R >= Rmin; R--)
      for (G = Gmin; G <= Gmax; G++) {
        histp = histogram + R * MR + G * MG + Bmin;
        for (B = Bmin; B <= Bmax; B++)
          if (*histp++ != 0) {
            boxp->Rmax = Rmax = R;
            goto have_Rmax;
          }
      }
have_Rmax:
  if (Gmax > Gmin)
    for (G = Gmin; G <= Gmax; G++)
      for (R = Rmin; R <= Rmax; R++) {
        histp = histogram + R * MR + G * MG + Bmin;
        for (B = Bmin; B <= Bmax; B++)
          if (*histp++ != 0) {
            boxp->Gmin = Gmin = G;
            goto have_Gmin;
          }
      }
have_Gmin:
  if (Gmax > Gmin)
    for (G = Gmax; G >= Gmin; G--)
      for (R = Rmin; R <= Rmax; R++) {
        histp = histogram + R * MR + G * MG + Bmin;
        for (B = Bmin; B <= Bmax; B++)
          if (*histp++ != 0) {
            boxp->Gmax = Gmax = G;
            goto have_Gmax;
          }
      }
have_Gmax:
  if (Bmax > Bmin)
    for (B = Bmin; B <= Bmax; B++)
      for (R = Rmin; R <= Rmax; R++) {
        histp = histogram + R * MR + Gmin * MG + B;
        for (G = Gmin; G <= Gmax; G++, histp += MG)
          if (*histp != 0) {
            boxp->Bmin = Bmin = B;
            goto have_Bmin;
          }
      }
have_Bmin:
  if (Bmax > Bmin)
    for (B = Bmax; B >= Bmin; B--)
      for (R = Rmin; R <= Rmax; R++) {
        histp = histogram + R * MR + Gmin * MG + B;
        for (G = Gmin; G <= Gmax; G++, histp += MG)
          if (*histp != 0) {
            boxp->Bmax = Bmax = B;
            goto have_Bmax;
          }
      }
have_Bmax:

  /* Update box volume.
   * We use 2-norm rather than real volume here; this biases the method
   * against making long narrow boxes, and it has the side benefit that
   * a box is splittable iff norm > 0.
   * Since the differences are expressed in histogram-cell units,
   * we have to shift back to JSAMPLE units to get consistent distances;
   * after which, we scale according to the selected distance scale factors.
   */
  dist0 = Rmax - Rmin;
  dist1 = Gmax - Gmin;
  dist2 = Bmax - Bmin;
  boxp->volume = dist0 * dist0 + dist1 * dist1 + dist2 * dist2;

  /* Now scan remaining volume of box and compute population */
  ccount = 0;
  for (R = Rmin; R <= Rmax; R++)
    for (G = Gmin; G <= Gmax; G++) {
      histp = histogram + R * MR + G * MG + Bmin;
      for (B = Bmin; B <= Bmax; B++, histp++)
        if (*histp != 0) {
          ccount++;
        }
    }

  boxp->colorcount = ccount;
}

static int median_cut_rgb(Histogram histogram, boxptr boxlist, int numboxes, int desired_colors)
/* Repeatedly select and split the largest box until we have enough boxes */
{
  int n, lb;
  int R, G, B, cmax;
  boxptr b1, b2;

  while (numboxes < desired_colors) {
    /* Select box to split.
     * Current algorithm: by population for first half, then by volume.
     */
    b1 = find_biggest_volume(boxlist, numboxes);

    if (b1 == 0)                /* no splittable boxes left! */
      break;
    b2 = boxlist + numboxes;    /* where new box will go */
    /* Copy the color bounds to the new box. */
    b2->Rmax = b1->Rmax;
    b2->Gmax = b1->Gmax;
    b2->Bmax = b1->Bmax;
    b2->Rmin = b1->Rmin;
    b2->Gmin = b1->Gmin;
    b2->Bmin = b1->Bmin;
    /* Choose which axis to split the box on.
     * Current algorithm: longest scaled axis.
     * See notes in update_box about scaling distances.
     */
    R = b1->Rmax - b1->Rmin;
    G = b1->Gmax - b1->Gmin;
    B = b1->Bmax - b1->Bmin;
    /* We want to break any ties in favor of green, then red, blue last.
     */
    cmax = G;
    n = 1;
    if (R > cmax) {
      cmax = R;
      n = 0;
    }
    if (B > cmax) {
      n = 2;
    }
    /* Choose split point along selected axis, and update box bounds.
     * Current algorithm: split at halfway point.
     * (Since the box has been shrunk to minimum volume,
     * any split will produce two nonempty subboxes.)
     * Note that lb value is max for lower box, so must be < old max.
     */
    switch (n) {
    case 0:
      lb = (b1->Rmax + b1->Rmin) / 2;
      b1->Rmax = lb;
      b2->Rmin = lb + 1;
      break;
    case 1:
      lb = (b1->Gmax + b1->Gmin) / 2;
      b1->Gmax = lb;
      b2->Gmin = lb + 1;
      break;
    case 2:
      lb = (b1->Bmax + b1->Bmin) / 2;
      b1->Bmax = lb;
      b2->Bmin = lb + 1;
      break;
    }
    /* Update stats for boxes */
    update_box_rgb(histogram, b1);
    update_box_rgb(histogram, b2);
    numboxes++;
  }
  return numboxes;
}

static void compute_color_rgb(QuantizeObj * quantobj, Histogram histogram, boxptr boxp, int icolor)
/* Compute representative color for a box, put it in colormap[icolor] */
{
  /* Current algorithm: mean weighted by pixels (not colors) */
  /* Note it is important to get the rounding correct! */
  ColorFreq *histp;
  int R, G, B;
  int Rmin, Rmax;
  int Gmin, Gmax;
  int Bmin, Bmax;
  unsigned long count;
  unsigned long total = 0;
  unsigned long Rtotal = 0;
  unsigned long Gtotal = 0;
  unsigned long Btotal = 0;

  Rmin = boxp->Rmin;
  Rmax = boxp->Rmax;
  Gmin = boxp->Gmin;
  Gmax = boxp->Gmax;
  Bmin = boxp->Bmin;
  Bmax = boxp->Bmax;

  for (R = Rmin; R <= Rmax; R++)
    for (G = Gmin; G <= Gmax; G++) {
      histp = histogram + R * MR + G * MG + Bmin;
      for (B = Bmin; B <= Bmax; B++) {
        if ((count = *histp++) != 0) {
          total += count;
          Rtotal += ((R << R_SHIFT) + ((1 << R_SHIFT) >> 1)) * count;
          Gtotal += ((G << G_SHIFT) + ((1 << G_SHIFT) >> 1)) * count;
          Btotal += ((B << B_SHIFT) + ((1 << B_SHIFT) >> 1)) * count;
        }
      }
    }

  quantobj->cmap[icolor].r = (unsigned char)((Rtotal + (total >> 1)) / total);
  quantobj->cmap[icolor].g = (unsigned char)((Gtotal + (total >> 1)) / total);
  quantobj->cmap[icolor].b = (unsigned char)((Btotal + (total >> 1)) / total);
  quantobj->freq[icolor] = total;
}

static void select_colors_rgb(QuantizeObj * quantobj, Histogram histogram)
/* Master routine for color selection */
{
  boxptr boxlist;
  int numboxes;
  int desired = quantobj->desired_number_of_colors;
  int i;

  /* Allocate workspace for box list */
  XMALLOC(boxlist, desired * sizeof(box));

  /* Initialize one box containing whole space */
  numboxes = 1;
  boxlist[0].Rmin = 0;
  boxlist[0].Rmax = (1 << PRECISION_R) - 1;
  boxlist[0].Gmin = 0;
  boxlist[0].Gmax = (1 << PRECISION_G) - 1;
  boxlist[0].Bmin = 0;
  boxlist[0].Bmax = (1 << PRECISION_B) - 1;
  /* Shrink it to actually-used volume and set its statistics */
  update_box_rgb(histogram, boxlist);
  /* Perform median-cut to produce final box list */
  numboxes = median_cut_rgb(histogram, boxlist, numboxes, desired);
  quantobj->actual_number_of_colors = numboxes;
  /* Compute the representative color for each box, fill colormap */
  for (i = 0; i < numboxes; i++)
    compute_color_rgb(quantobj, histogram, boxlist + i, i);
  free(boxlist);
}

/*
 * These routines are concerned with the time-critical task of mapping input
 * colors to the nearest color in the selected colormap.
 *
 * We re-use the histogram space as an "inverse color map", essentially a
 * cache for the results of nearest-color searches.  All colors within a
 * histogram cell will be mapped to the same colormap entry, namely the one
 * closest to the cell's center.  This may not be quite the closest entry to
 * the actual input color, but it's almost as good.  A zero in the cache
 * indicates we haven't found the nearest color for that cell yet; the array
 * is cleared to zeroes before starting the mapping pass.  When we find the
 * nearest color for a cell, its colormap index plus one is recorded in the
 * cache for future use.  The pass2 scanning routines call fill_inverse_cmap
 * when they need to use an unfilled entry in the cache.
 *
 * Our method of efficiently finding nearest colors is based on the "locally
 * sorted search" idea described by Heckbert and on the incremental distance
 * calculation described by Spencer W. Thomas in chapter III.1 of Graphics
 * Gems II (James Arvo, ed.  Academic Press, 1991).  Thomas points out that
 * the distances from a given colormap entry to each cell of the histogram can
 * be computed quickly using an incremental method: the differences between
 * distances to adjacent cells themselves differ by a constant.  This allows a
 * fairly fast implementation of the "brute force" approach of computing the
 * distance from every colormap entry to every histogram cell.  Unfortunately,
 * it needs a work array to hold the best-distance-so-far for each histogram
 * cell (because the inner loop has to be over cells, not colormap entries).
 * The work array elements have to be ints, so the work array would need
 * 256Kb at our recommended precision.  This is not feasible in DOS machines.

[ 256*1024/4 = 65,536 ]

 * To get around these problems, we apply Thomas' method to compute the
 * nearest colors for only the cells within a small subbox of the histogram.
 * The work array need be only as big as the subbox, so the memory usage
 * problem is solved.  Furthermore, we need not fill subboxes that are never
 * referenced in pass2; many images use only part of the color gamut, so a
 * fair amount of work is saved.  An additional advantage of this
 * approach is that we can apply Heckbert's locality criterion to quickly
 * eliminate colormap entries that are far away from the subbox; typically
 * three-fourths of the colormap entries are rejected by Heckbert's criterion,
 * and we need not compute their distances to individual cells in the subbox.
 * The speed of this approach is heavily influenced by the subbox size: too
 * small means too much overhead, too big loses because Heckbert's criterion
 * can't eliminate as many colormap entries.  Empirically the best subbox
 * size seems to be about 1/512th of the histogram (1/8th in each direction).
 *
 * Thomas' article also describes a refined method which is asymptotically
 * faster than the brute-force method, but it is also far more complex and
 * cannot efficiently be applied to small subboxes.  It is therefore not
 * useful for programs intended to be portable to DOS machines.  On machines
 * with plenty of memory, filling the whole histogram in one shot with Thomas'
 * refined method might be faster than the present code --- but then again,
 * it might not be any faster, and it's certainly more complicated.
 */

/* log2(histogram cells in update box) for each axis; this can be adjusted */
#define BOX_R_LOG  (PRECISION_R-3)
#define BOX_G_LOG  (PRECISION_G-3)
#define BOX_B_LOG  (PRECISION_B-3)

#define BOX_R_ELEMS  (1<<BOX_R_LOG) /* # of hist cells in update box */
#define BOX_G_ELEMS  (1<<BOX_G_LOG)
#define BOX_B_ELEMS  (1<<BOX_B_LOG)

#define BOX_R_SHIFT  (R_SHIFT + BOX_R_LOG)
#define BOX_G_SHIFT  (G_SHIFT + BOX_G_LOG)
#define BOX_B_SHIFT  (B_SHIFT + BOX_B_LOG)

/*
 * The next three routines implement inverse colormap filling.  They could
 * all be folded into one big routine, but splitting them up this way saves
 * some stack space (the mindist[] and bestdist[] arrays need not coexist)
 * and may allow some compilers to produce better code by registerizing more
 * inner-loop variables.
 */

static int find_nearby_colors(QuantizeObj * quantobj, int minR, int minG, int minB, int *colorlist)
/* Locate the colormap entries close enough to an update box to be candidates
 * for the nearest entry to some cell(s) in the update box.  The update box
 * is specified by the center coordinates of its first cell.  The number of
 * candidate colormap entries is returned, and their colormap indexes are
 * placed in colorlist[].
 * This routine uses Heckbert's "locally sorted search" criterion to select
 * the colors that need further consideration.
 */
{
  int numcolors = quantobj->actual_number_of_colors;
  int maxR, maxG, maxB;
  int centerR, centerG, centerB;
  int i, x, ncolors;
  int minmaxdist, min_dist = 0, max_dist, tdist;
  int mindist[MAXNUMCOLORS];    /* min distance to colormap entry i */

  /* Compute TRUE coordinates of update box's upper corner and center.
   * Actually we compute the coordinates of the center of the upper-corner
   * histogram cell, which are the upper bounds of the volume we care about.
   * Note that since ">>" rounds down, the "center" values may be closer to
   * min than to max; hence comparisons to them must be "<=", not "<".
   */
  maxR = minR + ((1 << BOX_R_SHIFT) - (1 << R_SHIFT));
  centerR = (minR + maxR) >> 1;
  maxG = minG + ((1 << BOX_G_SHIFT) - (1 << G_SHIFT));
  centerG = (minG + maxG) >> 1;
  maxB = minB + ((1 << BOX_B_SHIFT) - (1 << B_SHIFT));
  centerB = (minB + maxB) >> 1;

  /* For each color in colormap, find:
   *  1. its minimum squared-distance to any point in the update box
   *     (zero if color is within update box);
   *  2. its maximum squared-distance to any point in the update box.
   * Both of these can be found by considering only the corners of the box.
   * We save the minimum distance for each color in mindist[];
   * only the smallest maximum distance is of interest.
   */
  minmaxdist = 0x7FFFFFFFL;

  for (i = 0; i < numcolors; i++) {
    /* We compute the squared-R-distance term, then add in the other two. */
    x = quantobj->cmap[i].r;
    if (x < minR) {
      tdist = (x - minR) R_SCALE;
      min_dist = tdist * tdist;
      tdist = (x - maxR) R_SCALE;
      max_dist = tdist * tdist;
    } else if (x > maxR) {
      tdist = (x - maxR) R_SCALE;
      min_dist = tdist * tdist;
      tdist = (x - minR) R_SCALE;
      max_dist = tdist * tdist;
    } else {
      /* within cell range so no contribution to min_dist */
      min_dist = 0;
      if (x <= centerR) {
        tdist = (x - maxR) R_SCALE;
        max_dist = tdist * tdist;
      } else {
        tdist = (x - minR) R_SCALE;
        max_dist = tdist * tdist;
      }
    }

    x = quantobj->cmap[i].g;
    if (x < minG) {
      tdist = (x - minG) G_SCALE;
      min_dist += tdist * tdist;
      tdist = (x - maxG) G_SCALE;
      max_dist += tdist * tdist;
    } else if (x > maxG) {
      tdist = (x - maxG) G_SCALE;
      min_dist += tdist * tdist;
      tdist = (x - minG) G_SCALE;
      max_dist += tdist * tdist;
    } else {
      /* within cell range so no contribution to min_dist */
      if (x <= centerG) {
        tdist = (x - maxG) G_SCALE;
        max_dist += tdist * tdist;
      } else {
        tdist = (x - minG) G_SCALE;
        max_dist += tdist * tdist;
      }
    }

    x = quantobj->cmap[i].b;
    if (x < minB) {
      tdist = (x - minB) B_SCALE;
      min_dist += tdist * tdist;
      tdist = (x - maxB) B_SCALE;
      max_dist += tdist * tdist;
    } else if (x > maxB) {
      tdist = (x - maxB) B_SCALE;
      min_dist += tdist * tdist;
      tdist = (x - minB) B_SCALE;
      max_dist += tdist * tdist;
    } else {
      /* within cell range so no contribution to min_dist */
      if (x <= centerB) {
        tdist = (x - maxB) B_SCALE;
        max_dist += tdist * tdist;
      } else {
        tdist = (x - minB) B_SCALE;
        max_dist += tdist * tdist;
      }
    }

    mindist[i] = min_dist;      /* save away the results */
    if (max_dist < minmaxdist)
      minmaxdist = max_dist;
  }

  /* Now we know that no cell in the update box is more than minmaxdist
   * away from some colormap entry.  Therefore, only colors that are
   * within minmaxdist of some part of the box need be considered.
   */
  ncolors = 0;
  for (i = 0; i < numcolors; i++) {
    if (mindist[i] <= minmaxdist)
      colorlist[ncolors++] = i;
  }
  return ncolors;
}

static void find_best_colors(QuantizeObj * quantobj, int minR, int minG, int minB, int numcolors, int *colorlist, int *bestcolor)
/* Find the closest colormap entry for each cell in the update box,
  given the list of candidate colors prepared by find_nearby_colors.
  Return the indexes of the closest entries in the bestcolor[] array.
  This routine uses Thomas' incremental distance calculation method to
  find the distance from a colormap entry to successive cells in the box.
 */
{
  int iR, iG, iB;
  int i, icolor;
  int *bptr;                    /* pointer into bestdist[] array */
  int *cptr;                    /* pointer into bestcolor[] array */
  int dist0, dist1;             /* initial distance values */
  int dist2;                    /* current distance in inner loop */
  int xx0, xx1;                 /* distance increments */
  int xx2;
  int inR, inG, inB;            /* initial values for increments */

  /* This array holds the distance to the nearest-so-far color for each cell */
  int bestdist[BOX_R_ELEMS * BOX_G_ELEMS * BOX_B_ELEMS];

  /* Initialize best-distance for each cell of the update box */
  bptr = bestdist;
  for (i = BOX_R_ELEMS * BOX_G_ELEMS * BOX_B_ELEMS - 1; i >= 0; i--)
    *bptr++ = 0x7FFFFFFFL;

  /* For each color selected by find_nearby_colors,
   * compute its distance to the center of each cell in the box.
   * If that's less than best-so-far, update best distance and color number.
   */

  /* Nominal steps between cell centers ("x" in Thomas article) */
#define STEP_R  ((1 << R_SHIFT) R_SCALE)
#define STEP_G  ((1 << G_SHIFT) G_SCALE)
#define STEP_B  ((1 << B_SHIFT) B_SCALE)

  for (i = 0; i < numcolors; i++) {
    icolor = colorlist[i];
    /* Compute (square of) distance from minR/G/B to this color */
    inR = (minR - quantobj->cmap[icolor].r) R_SCALE;
    dist0 = inR * inR;
    inG = (minG - quantobj->cmap[icolor].g) G_SCALE;
    dist0 += inG * inG;
    inB = (minB - quantobj->cmap[icolor].b) B_SCALE;
    dist0 += inB * inB;
    /* Form the initial difference increments */
    inR = inR * (2 * STEP_R) + STEP_R * STEP_R;
    inG = inG * (2 * STEP_G) + STEP_G * STEP_G;
    inB = inB * (2 * STEP_B) + STEP_B * STEP_B;
    /* Now loop over all cells in box, updating distance per Thomas method */
    bptr = bestdist;
    cptr = bestcolor;
    xx0 = inR;
    for (iR = BOX_R_ELEMS - 1; iR >= 0; iR--) {
      dist1 = dist0;
      xx1 = inG;
      for (iG = BOX_G_ELEMS - 1; iG >= 0; iG--) {
        dist2 = dist1;
        xx2 = inB;
        for (iB = BOX_B_ELEMS - 1; iB >= 0; iB--) {
          if (dist2 < *bptr) {
            *bptr = dist2;
            *cptr = icolor;
          }
          dist2 += xx2;
          xx2 += 2 * STEP_B * STEP_B;
          bptr++;
          cptr++;
        }
        dist1 += xx1;
        xx1 += 2 * STEP_G * STEP_G;
      }
      dist0 += xx0;
      xx0 += 2 * STEP_R * STEP_R;
    }
  }
}

static void fill_inverse_cmap_rgb(QuantizeObj * quantobj, Histogram histogram, int R, int G, int B)
/* Fill the inverse-colormap entries in the update box that contains
 histogram cell R/G/B.  (Only that one cell MUST be filled, but
 we can fill as many others as we wish.) */
{
  int minR, minG, minB;         /* lower left corner of update box */
  int iR, iG, iB;
  int *cptr;                    /* pointer into bestcolor[] array */
  ColorFreq *cachep;            /* pointer into main cache array */
  /* This array lists the candidate colormap indexes. */
  int colorlist[MAXNUMCOLORS];
  int numcolors;                /* number of candidate colors */
  /* This array holds the actually closest colormap index for each cell. */
  int bestcolor[BOX_R_ELEMS * BOX_G_ELEMS * BOX_B_ELEMS];

  /* Convert cell coordinates to update box ID */
  R >>= BOX_R_LOG;
  G >>= BOX_G_LOG;
  B >>= BOX_B_LOG;

  /* Compute TRUE coordinates of update box's origin corner.
   * Actually we compute the coordinates of the center of the corner
   * histogram cell, which are the lower bounds of the volume we care about.
   */
  minR = (R << BOX_R_SHIFT) + ((1 << R_SHIFT) >> 1);
  minG = (G << BOX_G_SHIFT) + ((1 << G_SHIFT) >> 1);
  minB = (B << BOX_B_SHIFT) + ((1 << B_SHIFT) >> 1);

  /* Determine which colormap entries are close enough to be candidates
   * for the nearest entry to some cell in the update box.
   */
  numcolors = find_nearby_colors(quantobj, minR, minG, minB, colorlist);

  /* Determine the actually nearest colors. */
  find_best_colors(quantobj, minR, minG, minB, numcolors, colorlist, bestcolor);

  /* Save the best color numbers (plus 1) in the main cache array */
  R <<= BOX_R_LOG;              /* convert ID back to base cell indexes */
  G <<= BOX_G_LOG;
  B <<= BOX_B_LOG;
  cptr = bestcolor;
  for (iR = 0; iR < BOX_R_ELEMS; iR++) {
    for (iG = 0; iG < BOX_G_ELEMS; iG++) {
      cachep = &histogram[(R + iR) * MR + (G + iG) * MG + B];
      for (iB = 0; iB < BOX_B_ELEMS; iB++) {
        *cachep++ = (*cptr++) + 1;
      }
    }
  }
}

/*  This is pass 1  */
static void median_cut_pass1_rgb(QuantizeObj * quantobj, at_bitmap * image, const at_color * ignoreColor)
{
  generate_histogram_rgb(quantobj->histogram, image, ignoreColor);
  select_colors_rgb(quantobj, quantobj->histogram);
}

/* Map some rows of pixels to the output colormapped representation. */
static void median_cut_pass2_rgb(QuantizeObj * quantobj, at_bitmap * image, const at_color * bgColor)
 /* This version performs no dithering */
{
  Histogram histogram = quantobj->histogram;
  ColorFreq *cachep;
  int R, G, B;
  int origR, origG, origB;
  int row, col;
  int spp = AT_BITMAP_PLANES(image);
  int width = AT_BITMAP_WIDTH(image);
  int height = AT_BITMAP_HEIGHT(image);
  unsigned char *src, *dest;
  at_color bg_color = { 0xff, 0xff, 0xff };

  zero_histogram_rgb(histogram);

  if (bgColor) {
    /* Find the nearest colormap entry for the background color. */
    R = bgColor->r >> R_SHIFT;
    G = bgColor->g >> G_SHIFT;
    B = bgColor->b >> B_SHIFT;
    cachep = &histogram[R * MR + G * MG + B];
    if (*cachep == 0)
      fill_inverse_cmap_rgb(quantobj, histogram, R, G, B);
    bg_color = quantobj->cmap[*cachep - 1];
  }

  src = dest = image->bitmap;
  if (spp == 3) {
    for (row = 0; row < height; row++) {
      for (col = 0; col < width; col++) {
        /* get pixel value and index into the cache */
        origR = (*src++);
        origG = (*src++);
        origB = (*src++);

        /*
           if (origR > 253 && origG > 253 && origB > 253)
           {
           (*dest++) = 255; (*dest++) = 255; (*dest++) = 255;
           continue;
           }
         */

        /* get pixel value and index into the cache */
        R = origR >> R_SHIFT;
        G = origG >> G_SHIFT;
        B = origB >> B_SHIFT;
        cachep = &histogram[R * MR + G * MG + B];
        /* If we have not seen this color before, find nearest
           colormap entry and update the cache */
        if (*cachep == 0) {
          fill_inverse_cmap_rgb(quantobj, histogram, R, G, B);
        }
        /* Now emit the colormap index for this cell */
        dest[0] = quantobj->cmap[*cachep - 1].r;
        dest[1] = quantobj->cmap[*cachep - 1].g;
        dest[2] = quantobj->cmap[*cachep - 1].b;

        /* If the colormap entry for this pixel is the same as the
           background's colormap entry, set the pixel to the
           background color. */
        if (bgColor && (dest[0] == bg_color.r && dest[1] == bg_color.g && dest[2] == bg_color.b)) {
          dest[0] = bgColor->r;
          dest[1] = bgColor->g;
          dest[2] = bgColor->b;
        }
        dest += 3;
      }
    }
  } else if (spp == 1) {
    long idx = width * height;
    while (--idx >= 0) {
      origR = src[idx];
      R = origR >> R_SHIFT;
      G = origR >> G_SHIFT;
      B = origR >> B_SHIFT;
      cachep = &histogram[R * MR + G * MG + B];
      if (*cachep == 0)
        fill_inverse_cmap_rgb(quantobj, histogram, R, G, B);

      dest[idx] = quantobj->cmap[*cachep - 1].r;

      /* If the colormap entry for this pixel is the same as the
         background's colormap entry, set the pixel to the
         background color. */
      if (bgColor && dest[idx] == bg_color.r)
        dest[idx] = bgColor->r;
    }
  }
}

static QuantizeObj *initialize_median_cut(int num_colors)
{
  QuantizeObj *quantobj;

  /* Initialize the data structures */
  XMALLOC(quantobj, sizeof(QuantizeObj));

  XMALLOC(quantobj->histogram, sizeof(ColorFreq) * HIST_R_ELEMS * HIST_G_ELEMS * HIST_B_ELEMS);
  quantobj->desired_number_of_colors = num_colors;

  return quantobj;
}

void quantize(at_bitmap * image, long ncolors, const at_color * bgColor, QuantizeObj ** iQuant, at_exception_type * exp)
{
  QuantizeObj *quantobj;
  unsigned int spp = AT_BITMAP_PLANES(image);

  if (spp != 3 && spp != 1) {
    LOG("quantize: %u-plane images are not supported", spp);
    at_exception_fatal(exp, "quantize: wrong plane images are passed");
    return;
  }

  /* If a pointer was sent in, let's use it. */
  if (iQuant) {
    if (*iQuant == NULL) {
      quantobj = initialize_median_cut(ncolors);
      median_cut_pass1_rgb(quantobj, image, bgColor);
      *iQuant = quantobj;
    } else
      quantobj = *iQuant;
  } else {
    quantobj = initialize_median_cut(ncolors);
    median_cut_pass1_rgb(quantobj, image, NULL);
  }

  median_cut_pass2_rgb(quantobj, image, bgColor);

  if (iQuant == NULL)
    quantize_object_free(quantobj);
}

void quantize_object_free(QuantizeObj * quantobj)
{
  free(quantobj->histogram);
  free(quantobj);
}
