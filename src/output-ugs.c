/* output-ugs.h - output in UGS format

   Copyright (C) 2003 Serge Vakulenko <vak@cronyx.ru>

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

#include "spline.h"
#include "color.h"
#include "output-ugs.h"
#include "logreport.h"
#include <math.h>

long ugs_design_pixels;         /*  A design size of font in pixels. */

long ugs_charcode;
long ugs_advance_width;
long ugs_left_bearing, ugs_descend;
long ugs_max_col, ugs_max_row;

static long lowerx, upperx, lowery, uppery;

static int compute_determinant(double *det, double a, double b, double c, double d)
{
  double lensq;

  *det = a * d - b * c;
  lensq = (a * a + b * b) * (c * c + d * d);
  if (lensq < 1 || *det * *det * 4000000 < lensq)
    return 0;
  return 1;
}

static void cubic_to_quadratic(double ax, double ay, double bx, double by, double cx, double cy, double dx, double dy, double *fx, double *fy, double *ex, double *ey, double *gx, double *gy)
{
  double t, t1, det, tanx, tany, s, tw1, tw2;

  /* Find the twist point. If no, then use 1/2. */
  tw1 = -ax + 2 * bx - cx;
  tw2 = -ax + 3 * bx - 3 * cx + dx;
  if (tw2 < 0) {
    tw1 = -tw1;
    tw2 = -tw2;
  }
  if (tw1 > 0.001 && tw1 < tw2) {
    t = tw1 / tw2;
  } else {
    tw1 = -ay + 2 * by - cy;
    tw2 = -ay + 3 * by - 3 * cy + dy;
    if (tw2 < 0) {
      tw1 = -tw1;
      tw2 = -tw2;
    }
    if (tw1 > 0.001 && tw1 < tw2) {
      t = tw1 / tw2;
    } else
      t = 0.5;
  }
  t1 = 1 - t;

  /* Compute the point E way t from x to y, along the curve. */
  *ex = ax * t1 * t1 * t1 + 3 * bx * t * t1 * t1 + 3 * cx * t * t * t1 + dx * t * t * t;
  *ey = ay * t1 * t1 * t1 + 3 * by * t * t1 * t1 + 3 * cy * t * t * t1 + dy * t * t * t;

  /* Compute the tangent at point E. */
  tanx = 3 * (-ax * t1 * t1 + bx * t1 * (1 - 3 * t) + cx * t * (2 - 3 * t) + dx * t * t);
  tany = 3 * (-ay * t1 * t1 + by * t1 * (1 - 3 * t) + cy * t * (2 - 3 * t) + dy * t * t);

  /* Find point F as an intersection of AB and tangent at E.
   * Solving a system:
   * (B-A) t + (D+C-B-A) * s = (E-A) */
  if (compute_determinant(&det, bx - ax, by - ay, tanx, tany)) {
    compute_determinant(&s, *ex - ax, *ey - ay, tanx, tany);
    s /= det;
    if (s < 0)
      s = 0;

    *fx = bx * s + ax * (1 - s);
    *fy = by * s + ay * (1 - s);
  } else {
    /* Cannot solve, let F be equal E. */
    *fx = *ex;
    *fy = *ey;
  }

  /* Find point G as an intersection of CD and tangent at E.
   * Solving a system:
   * (C-D) t + (A+B-C-D) * s = (E-D) */
  if (compute_determinant(&det, cx - dx, cy - dy, -tanx, -tany)) {
    compute_determinant(&s, *ex - dx, *ey - dy, -tanx, -tany);
    s /= det;
    if (s < 0)
      s = 0;

    *gx = cx * s + dx * (1 - s);
    *gy = cy * s + dy * (1 - s);
  } else {
    /* Cannot solve, let G be equal E. */
    *gx = *ex;
    *gy = *ey;
  }
}

#if CUBIC
static void output_contour(FILE * file, FILE * tracer, unsigned height)
{
  int x, lastx;

  fprintf(file, "\tcontour\n");
  for (lastx = 0; (x = getc(tracer)) >= 0; lastx = x) {
    double x1, y1, x1a, y1a, x3a, y3a, x3, y3;

    if (x != 'M' || lastx != '"')
      continue;

    if (fscanf(tracer, "%lg%lg", &x1, &y1) != 2)
      FATAL("Autotrace format error");
    y1 = height - y1;

    fprintf(file, "\t\tpath\n");
    fprintf(file, "\t\t\tmove %g %g\n", x1, y1);

    while ((x = getc(tracer)) >= 0) {
      if (x == 'L') {
        if (fscanf(tracer, "%lg%lg", &x3, &y3) != 2)
          FATAL("Autotrace format error");
        y3 = height - y3;
        fprintf(file, "\t\t\tline %g %g\n", x3, y3);
      } else if (x == 'C') {
        if (fscanf(tracer, "%lg%lg%lg%lg%lg%lg", &x1a, &y1a, &x3a, &y3a, &x3, &y3) != 6)
          FATAL("Autotrace format error");
        y1a = height - y1a;
        y3a = height - y3a;
        y3 = height - y3;
        fprintf(file, "\t\t\tcurve %g %g %g %g %g %g\n", x1a, y1a, x3a, y3a, x3, y3);
      } else
        break;
    }
    fprintf(file, "\t\tend path\n");
  }
  fprintf(file, "\tend contour\n");
}
#endif

static void output_splines(FILE * file, spline_list_array_type shape, int height)
{
  unsigned l, s;
  spline_list_type list;
  spline_type first, t;
  double x1, y1, x1a, y1a, x2, y2, x3a, y3a, x3, y3;
  int ix1, iy1, ix1a, iy1a, ix2, iy2, ix3a, iy3a, ix3, iy3;

  fprintf(file, "\tcontour\n");
  for (l = 0; l < SPLINE_LIST_ARRAY_LENGTH(shape); l++) {
    list = SPLINE_LIST_ARRAY_ELT(shape, l);
    first = SPLINE_LIST_ELT(list, 0);

    x1 = START_POINT(first).x + ugs_left_bearing;
    y1 = START_POINT(first).y + ugs_descend;
    ix1 = lround(x1);
    iy1 = lround(y1);

    fprintf(file, "\t\tpath\n");
    fprintf(file, "\t\t\tdot-on %d %d\n", ix1, iy1);

    if (lowerx > ix1)
      lowerx = ix1;
    if (lowery > iy1)
      lowery = iy1;
    if (upperx < ix1)
      upperx = ix1;
    if (uppery < iy1)
      uppery = iy1;

    for (s = 0; s < SPLINE_LIST_LENGTH(list); s++) {
      t = SPLINE_LIST_ELT(list, s);

      if (SPLINE_DEGREE(t) == LINEARTYPE) {
        x3 = END_POINT(t).x + ugs_left_bearing;
        y3 = END_POINT(t).y + ugs_descend;
        ix3 = lround(x3);
        iy3 = lround(y3);

        if (!(ix3 == lround(x1) && iy3 == lround(y1)))
          fprintf(file, "\t\t\tdot-on %d %d\n", ix3, iy3);

        if (lowerx > ix3)
          lowerx = ix3;
        if (lowery > iy3)
          lowery = iy3;
        if (upperx < ix3)
          upperx = ix3;
        if (uppery < iy3)
          uppery = iy3;
      } else {
        x1a = CONTROL1(t).x + ugs_left_bearing;
        y1a = CONTROL1(t).y + ugs_descend;
        x3a = CONTROL2(t).x + ugs_left_bearing;
        y3a = CONTROL2(t).y + ugs_descend;
        x3 = END_POINT(t).x + ugs_left_bearing;
        y3 = END_POINT(t).y + ugs_descend;
        ix3 = lround(x3);
        iy3 = lround(y3);

        cubic_to_quadratic(x1, y1, x1a, y1a, x3a, y3a, x3, y3, &x1a, &y1a, &x2, &y2, &x3a, &y3a);
        ix1a = lround(x1a);
        iy1a = lround(y1a);
        ix2 = lround(x2);
        iy2 = lround(y2);
        ix3a = lround(x3a);
        iy3a = lround(y3a);

        if (!(ix1a == lround(x1) && iy1a == lround(y1)) && !(ix1a == ix2 && iy1a == iy2))
          fprintf(file, "\t\t\tdot-off %d %d\n", ix1a, iy1a);

        fprintf(file, "\t\t\tdot-on %d %d\n", ix2, iy2);

        if (!(ix3a == ix2 && iy3a == iy2) && !(ix3a == ix3 && iy3a == iy3))
          fprintf(file, "\t\t\tdot-off %d %d\n", ix3a, iy3a);

        fprintf(file, "\t\t\tdot-on %d %d\n", ix3, iy3);

        if (lowerx > ix1a)
          lowerx = ix1a;
        if (lowery > iy1a)
          lowery = iy1a;
        if (upperx < ix1a)
          upperx = ix1a;
        if (uppery < iy1a)
          uppery = iy1a;

        if (lowerx > ix2)
          lowerx = ix2;
        if (lowery > iy2)
          lowery = iy2;
        if (upperx < ix2)
          upperx = ix2;
        if (uppery < iy2)
          uppery = iy2;

        if (lowerx > ix3a)
          lowerx = ix3a;
        if (lowery > iy3a)
          lowery = iy3a;
        if (upperx < ix3a)
          upperx = ix3a;
        if (uppery < iy3a)
          uppery = iy3a;

        if (lowerx > ix3)
          lowerx = ix3;
        if (lowery > iy3)
          lowery = iy3;
        if (upperx < ix3)
          upperx = ix3;
        if (uppery < iy3)
          uppery = iy3;
      }
      x1 = x3;
      y1 = y3;
    }
    fprintf(file, "\t\tend path\n");
  }
  fprintf(file, "\tend contour\n");
}

int output_ugs_writer(FILE * file, gchar * name, int llx, int lly, int urx, int ury, at_output_opts_type * opts, spline_list_array_type shape, at_msg_func msg_func, gpointer msg_data, gpointer usar_data)
{
  /* Write the header.  */
  fprintf(file, "symbol %#lx design-size %ld\n", ugs_charcode, ugs_design_pixels);
  fprintf(file, "\tadvance-width %ld\n", ugs_advance_width);

  upperx = ugs_advance_width - ugs_max_col - 1;
  uppery = ugs_max_row;

  lowerx = ugs_left_bearing;
  lowery = ugs_descend;

  output_splines(file, shape, ury - lly);

  fprintf(file, "\tleft-bearing %ld\n", lowerx);
  fprintf(file, "\tright-bearing %ld\n", ugs_advance_width - upperx - 1);
  fprintf(file, "\tascend %ld\n", uppery + 1);
  fprintf(file, "\tdescend %ld\n", lowery);

  /* Write the trailer.  */
  fputs("end symbol\n\n", file);
  return 0;
}
