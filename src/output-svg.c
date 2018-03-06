/* output-svg.h - output in SVG format

   Copyright (C) 1999, 2000, 2001 Bernhard Herzog

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
#include "output-svg.h"

static void out_splines(FILE * file, spline_list_array_type shape, int height)
{
  unsigned this_list;
  spline_list_type list;
  at_color last_color = { 0, 0, 0 };

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
      fprintf(file, "<path style=\"%s:#%02x%02x%02x; %s:none;\" d=\"", (shape.centerline || list.open) ? "stroke" : "fill", list.color.r, list.color.g, list.color.b, (shape.centerline || list.open) ? "fill" : "stroke");
    }
    fprintf(file, "M%g %g", START_POINT(first).x, height - START_POINT(first).y);
    for (this_spline = 0; this_spline < SPLINE_LIST_LENGTH(list); this_spline++) {
      spline_type s = SPLINE_LIST_ELT(list, this_spline);

      if (SPLINE_DEGREE(s) == LINEARTYPE) {
        fprintf(file, "L%g %g", END_POINT(s).x, height - END_POINT(s).y);
      } else {
        fprintf(file, "C%g %g %g %g %g %g", CONTROL1(s).x, height - CONTROL1(s).y, CONTROL2(s).x, height - CONTROL2(s).y, END_POINT(s).x, height - END_POINT(s).y);
      }
      last_color = list.color;
    }
  }
  if (!(shape.centerline || list.open))
    fputs("z", file);
  if (SPLINE_LIST_ARRAY_LENGTH(shape) > 0)
    fputs("\"/>\n", file);
}

int output_svg_writer(FILE * file, gchar * name, int llx, int lly, int urx, int ury, at_output_opts_type * opts, spline_list_array_type shape, at_msg_func msg_func, gpointer msg_data, gpointer user_data)
{
  int width = urx - llx;
  int height = ury - lly;
  fputs("<?xml version=\"1.0\" standalone=\"yes\"?>\n", file);
  fprintf(file, "<svg width=\"%d\" height=\"%d\">\n", width, height);

  out_splines(file, shape, height);
  fputs("</svg>\n", file);

  return 0;
}
