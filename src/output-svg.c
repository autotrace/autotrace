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

static void out_splines(FILE *file, spline_list_array_type shape, int height)
{
  unsigned this_list;
  spline_list_type list;
  at_color last_color = {0, 0, 0};

  for (this_list = 0; this_list < SPLINE_LIST_ARRAY_LENGTH(shape); this_list++) {
    unsigned this_spline;
    spline_type first;

    list = SPLINE_LIST_ARRAY_ELT(shape, this_list);
    first = SPLINE_LIST_ELT(list, 0);

    if (this_list == 0 || !at_color_equal(&list.color, &last_color)) {
      if (this_list > 0) {
        if (!(shape.centerline || list.open))
          fputs("z", file);
        fputs("\"/>\n", file);
      }
      fprintf(file, "<path style=\"%s:#%02x%02x%02x; %s:none;\" d=\"",
              (shape.centerline || list.open) ? "stroke" : "fill", list.color.r, list.color.g,
              list.color.b, (shape.centerline || list.open) ? "fill" : "stroke");
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
