/* output-er.c: utility routines for Elastic Reality shape file output

   Copyright (C) 1999, 2000, 2001 Martin Weber.

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

#include "types.h"
#include "spline.h"
#include "output-er.h"
#include "xstd.h"
#include "datetime.h"
#include <string.h>
#include <glib.h>

#define NUM_CORRESP_POINTS 4

/* This should be called before the others in this file.  It opens the
   output file and writes some preliminary boilerplate. */

static int output_er_header(FILE * er_file, gchar * name, int llx, int lly, int urx, int ury)
{
  gchar *time;

  fprintf(er_file, "#Elastic Reality Shape File\n\n#Date: %s\n\n", time = at_time_string());

  g_free(time);

  fprintf(er_file, "ImageSize = {\n\tWidth = %d\n\tHeight = %d\n}\n\n", urx - llx, ury - lly);

  return 0;
}

/* This outputs shape data and the point list for the shape in SHAPE. */

static void out_splines(FILE * er_file, spline_list_array_type shape, unsigned width, unsigned height, at_output_opts_type * opts)
{
  unsigned this_list, corresp_pt;
  double x0, y0, x1, y1, x2, y2, corresp_length;

  for (this_list = 0; this_list < SPLINE_LIST_ARRAY_LENGTH(shape); this_list++) {
    unsigned this_spline;
    spline_type prev;

    spline_list_type list = SPLINE_LIST_ARRAY_ELT(shape, this_list);
    unsigned length = SPLINE_LIST_LENGTH(list);
    unsigned out_length = (list.open || length == 1 ? length + 1 : length);

    fprintf(er_file, "Shape = {\n");
    fprintf(er_file, "\t#Shape Number %d\n", this_list + 1);
    fprintf(er_file, "\tGroup = Default\n");
    fprintf(er_file, "\tType = Source\n");
    fprintf(er_file, "\tRoll = A\n");
    fprintf(er_file, "\tOpaque = True\n");
    fprintf(er_file, "\tLocked = False\n");
    fprintf(er_file, "\tWarp = True\n");
    fprintf(er_file, "\tCookieCut = True\n");
    fprintf(er_file, "\tColorCorrect = True\n");
    fprintf(er_file, "\tPrecision = 10\n");
    fprintf(er_file, "\tClosed = %s\n", (list.open ? "False" : "True"));
    fprintf(er_file, "\tTween = Linear\n");
    fprintf(er_file, "\tBPoints = %d\n", out_length);
    fprintf(er_file, "\tCPoints = %d\n", NUM_CORRESP_POINTS);
    fprintf(er_file, "\tFormKey = {\n");
    fprintf(er_file, "\t\tFrame = 1\n");
    fprintf(er_file, "\t\tPointList = {\n");

    prev = PREV_SPLINE_LIST_ELT(list, 0);
    if (list.open || length == 1)
      SPLINE_DEGREE(prev) = (polynomial_degree) - 1;

    for (this_spline = 0; this_spline < length; this_spline++) {
      spline_type s = SPLINE_LIST_ELT(list, this_spline);

      if (SPLINE_DEGREE(prev) == -1) {
        x0 = START_POINT(s).x;
        y0 = START_POINT(s).y;
      } else if (SPLINE_DEGREE(prev) == CUBICTYPE) {
        x0 = CONTROL2(prev).x;
        y0 = CONTROL2(prev).y;
      } else {                  /* if (SPLINE_DEGREE(prev) == LINEARTYPE) */
        x0 = START_POINT(s).x;
        y0 = START_POINT(s).y;
      }

      x1 = START_POINT(s).x;
      y1 = START_POINT(s).y;

      if (SPLINE_DEGREE(s) == CUBICTYPE) {
        x2 = CONTROL1(s).x;
        y2 = CONTROL1(s).y;
      } else {
        x2 = START_POINT(s).x;
        y2 = START_POINT(s).y;
      }

      fprintf(er_file, "\t\t\t(%f, %f), (%f, %f), (%f, %f),\n", x0 / width, y0 / height, x1 / width, y1 / height, x2 / width, y2 / height);

      prev = s;
    }

    if (list.open || length == 1) {
      x0 = CONTROL2(prev).x;
      y0 = CONTROL2(prev).y;
      x2 = x1 = END_POINT(prev).x;
      y2 = y1 = END_POINT(prev).y;

      fprintf(er_file, "\t\t\t(%f, %f), (%f, %f), (%f, %f),\n", x0 / width, y0 / height, x1 / width, y1 / height, x2 / width, y2 / height);
    }

    /* Close PointList and enclosing FormKey. */
    fprintf(er_file, "\t\t}\n\n\t}\n\n");

    if (shape.centerline && shape.preserve_width) {
      gfloat w = (gfloat) 1.0 / (shape.width_weight_factor);

      fprintf(er_file, "\tWeightKey = {\n");
      fprintf(er_file, "\t\tFrame = 1\n");
      fprintf(er_file, "\t\tPointList = {\n");
      prev = PREV_SPLINE_LIST_ELT(list, 0);
      if (list.open || length == 1)
        SPLINE_DEGREE(prev) = (polynomial_degree) - 1;
      for (this_spline = 0; this_spline < length; this_spline++) {
        spline_type s = SPLINE_LIST_ELT(list, this_spline);

        if (SPLINE_DEGREE(prev) == -1)
          x0 = START_POINT(s).z;
        else if (SPLINE_DEGREE(prev) == CUBICTYPE)
          x0 = CONTROL2(prev).z;
        else                    /* if (SPLINE_DEGREE(prev) == LINEARTYPE) */
          x0 = START_POINT(s).z;

        x1 = START_POINT(s).z;

        if (SPLINE_DEGREE(s) == CUBICTYPE)
          x2 = CONTROL1(s).z;
        else
          x2 = START_POINT(s).z;

        fprintf(er_file, "\t\t\t%g, %g, %g,\n", x0 * w, x1 * w, x2 * w);

        prev = s;
      }
      if (list.open || length == 1) {
        x0 = CONTROL2(prev).z;
        x2 = x1 = END_POINT(prev).z;
        fprintf(er_file, "\t\t\t%g, %g, %g,\n", x0 * w, x1 * w, x2 * w);
      }
      /* Close PointList and enclosing WeightKey. */
      fprintf(er_file, "\t\t}\n\n\t}\n\n");
    }

    fprintf(er_file, "\tCorrKey = {\n");
    fprintf(er_file, "\t\tFrame = 1\n");
    fprintf(er_file, "\t\tPointList = {\n");
    fprintf(er_file, "\t\t\t0");
    corresp_length = out_length - (list.open ? 1.0 : 2.0);
    for (corresp_pt = 1; corresp_pt < NUM_CORRESP_POINTS; corresp_pt++) {
      fprintf(er_file, ", %g", corresp_length * corresp_pt / (NUM_CORRESP_POINTS - (list.open ? 1.0 : 0.0)));
    }
    /* Close PointList and enclosing CorrKey. */
    fprintf(er_file, "\n\t\t}\n\n\t}\n\n");

    /* Close Shape. */
    fprintf(er_file, "}\n\n");
  }
}

int output_er_writer(FILE * file, gchar * name, int llx, int lly, int urx, int ury, at_output_opts_type * opts, spline_list_array_type shape, at_msg_func msg_func, gpointer msg_data, gpointer user_data)
{
  int result;
  unsigned width, height;

  result = output_er_header(file, name, llx, lly, urx, ury);
  if (result != 0)
    return result;

  width = urx - llx;
  height = ury - lly;
  out_splines(file, shape, width, height, opts);

  return 0;
}
