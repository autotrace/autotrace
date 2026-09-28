/*
 * Copyright (C) 1999, 2000, 2001 Bernhard Herzog
 * SPDX-FileCopyrightText: © 2000-2001 Martin Weber
 * SPDX-FileCopyrightText: © 2001 Per Grahn
 * SPDX-FileCopyrightText: © 2001-2003 Masatake YAMATO
 * SPDX-FileCopyrightText: © 2017-2025 Peter Lemenkov
 * SPDX-FileCopyrightText: © 2022 EdwardTheLegend
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "spline.h"
#include "color.h"
#include "output-svg.h"

/* With -preserve-width the centerline fitter stores the distance from each
   point to the edge of the stroke, scaled by the width weight factor, in the
   z coordinate.  Average it over the stroke and double it to get a stroke
   width in pixels.  */
static gfloat stroke_width(spline_list_type list, gfloat width_weight_factor)
{
  gfloat sum = 0;
  unsigned n = 0;

  for (unsigned i = 0; i < SPLINE_LIST_LENGTH(list); i++) {
    spline_type s = SPLINE_LIST_ELT(list, i);

    sum += START_POINT(s).z + END_POINT(s).z;
    n += 2;
  }
  return n ? 2 * sum / n / width_weight_factor : 0;
}

static void out_splines(FILE *file, spline_list_array_type shape, int height)
{
  unsigned this_list;
  spline_list_type list;
  at_color last_color = {0, 0, 0};
  /* Every stroke gets its own width, and so its own path element.  */
  gboolean widths = shape.centerline && shape.preserve_width;

  for (this_list = 0; this_list < SPLINE_LIST_ARRAY_LENGTH(shape); this_list++) {
    unsigned this_spline;
    spline_type first;

    list = SPLINE_LIST_ARRAY_ELT(shape, this_list);
    first = SPLINE_LIST_ELT(list, 0);

    if (this_list == 0 || widths || !at_color_equal(&list.color, &last_color)) {
      if (this_list > 0) {
        if (!(shape.centerline || list.open))
          fputs("z", file);
        fputs("\"/>\n", file);
      }
      fprintf(file, "<path style=\"%s:#%02x%02x%02x; %s:none;",
              (shape.centerline || list.open) ? "stroke" : "fill", list.color.r, list.color.g,
              list.color.b, (shape.centerline || list.open) ? "fill" : "stroke");
      if (widths)
        /* Round caps make a dot, whose centerline has no length, visible.  */
        fprintf(file, " stroke-width:%.2f; stroke-linecap:round; stroke-linejoin:round;",
                stroke_width(list, shape.width_weight_factor));
      fputs("\" d=\"", file);
    }
    fprintf(file, "M%g %g", START_POINT(first).x, height - START_POINT(first).y);
    for (this_spline = 0; this_spline < SPLINE_LIST_LENGTH(list); this_spline++) {
      spline_type s = SPLINE_LIST_ELT(list, this_spline);

      if (SPLINE_DEGREE(s) == LINEARTYPE) {
        fprintf(file, "L%g %g", END_POINT(s).x, height - END_POINT(s).y);
      } else {
        fprintf(file, "C%g %g %g %g %g %g", CONTROL1(s).x, height - CONTROL1(s).y, CONTROL2(s).x,
                height - CONTROL2(s).y, END_POINT(s).x, height - END_POINT(s).y);
      }
      last_color = list.color;
    }
  }
  if (!(shape.centerline || list.open))
    fputs("z", file);
  if (SPLINE_LIST_ARRAY_LENGTH(shape) > 0)
    fputs("\"/>\n", file);
}

int output_svg_writer(FILE *file, gchar *name, int llx, int lly, int urx, int ury,
                      at_output_opts_type *opts, spline_list_array_type shape, at_msg_func msg_func,
                      gpointer msg_data, gpointer user_data)
{
  int width = urx - llx;
  int height = ury - lly;
  fputs("<?xml version=\"1.0\" standalone=\"yes\"?>\n", file);
  fprintf(file, "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"%d\" height=\"%d\">\n", width,
          height);

  out_splines(file, shape, height);
  fputs("</svg>\n", file);

  return 0;
}
