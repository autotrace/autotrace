/*
 * Copyright (C) 1999, 2000, 2001 Martin Weber
 * SPDX-FileCopyrightText: © 2001 Per Grahn
 * SPDX-FileCopyrightText: © 2001-2002 Martin Weber
 * SPDX-FileCopyrightText: © 2001-2003 Masatake YAMATO
 * SPDX-FileCopyrightText: © 2017-2025 Peter Lemenkov
 * SPDX-FileCopyrightText: © 2020 Han Mertens
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "spline.h"
#include "color.h"
#include "output-epd.h"
#include "autotrace.h"
#include <math.h>
#include <glib.h>

/* Output macros.  */

/* This should be used for outputting a string S on a line by itself.  */
#define OUT_LINE(s) fprintf(epd_file, "%s\n", s)

/* This output their arguments, preceded by the indentation.  */
#define OUT(...) fprintf(epd_file, __VA_ARGS__)

/* These macros just output their arguments.  */
#define OUT_REAL(r)                                                                                \
  do {                                                                                             \
    double _r = (r);                                                                               \
    double _frac = fabs(_r - round(_r));                                                           \
    fprintf(epd_file, _frac < 0.0001 ? "%.0f " : "%.3f ", _r);                                     \
  } while (0)

/* For a PostScript command with two real arguments, e.g., lineto.  OP
   should be a constant string.  */
#define OUT_COMMAND2(first, second, op)                                                            \
  do {                                                                                             \
    OUT_REAL(first);                                                                               \
    OUT_REAL(second);                                                                              \
    OUT(op "\n");                                                                                  \
  } while (0)

/* For a PostScript command with six real arguments, e.g., curveto.
   Again, OP should be a constant string.  */
#define OUT_COMMAND6(first, second, third, fourth, fifth, sixth, op)                               \
  do {                                                                                             \
    OUT_REAL(first);                                                                               \
    OUT_REAL(second);                                                                              \
    OUT(" ");                                                                                      \
    OUT_REAL(third);                                                                               \
    OUT_REAL(fourth);                                                                              \
    OUT(" ");                                                                                      \
    OUT_REAL(fifth);                                                                               \
    OUT_REAL(sixth);                                                                               \
    OUT(" " op " \n");                                                                             \
  } while (0)

/* This should be called before the others in this file.  It opens the
   output file `OUTPUT_NAME.ps', and writes some preliminary boilerplate. */

static int output_epd_header(FILE *epd_file, gchar *name, int llx, int lly, int urx, int ury)
{
  g_autoptr(GDateTime) date = g_date_time_new_now_local();
  g_autofree gchar *time = g_date_time_format(date, "%a %b %e %H:%M:%S %Y");

  OUT_LINE("%EPD-1.0");
  OUT("%% Created by %s\n", at_version(TRUE));
  OUT("%% Title: %s\n", name);
  OUT("%% CreationDate: %s\n", time);
  OUT("%%BBox(%d,%d,%d,%d)\n", llx, lly, urx, ury);

  return 0;
}

/* This outputs the PostScript code which produces the shape in
   SHAPE.  */

static void out_splines(FILE *epd_file, spline_list_array_type shape)
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
        OUT_LINE((shape.centerline || list.open) ? "S" : "f");
        OUT_LINE("h");
      }
      OUT("%.3f %.3f %.3f %s\n", (double)list.color.r / 255.0, (double)list.color.g / 255.0,
          (double)list.color.b / 255.0, (shape.centerline || list.open) ? "RG" : "rg");
      last_color = list.color;
    }
    OUT_COMMAND2(START_POINT(first).x, START_POINT(first).y, "m");

    for (this_spline = 0; this_spline < SPLINE_LIST_LENGTH(list); this_spline++) {
      spline_type s = SPLINE_LIST_ELT(list, this_spline);

      if (SPLINE_DEGREE(s) == LINEARTYPE)
        OUT_COMMAND2(END_POINT(s).x, END_POINT(s).y, "l");
      else
        OUT_COMMAND6(CONTROL1(s).x, CONTROL1(s).y, CONTROL2(s).x, CONTROL2(s).y, END_POINT(s).x,
                     END_POINT(s).y, "c");
    }
  }
  if (SPLINE_LIST_ARRAY_LENGTH(shape) > 0)
    OUT_LINE((shape.centerline || list.open) ? "S" : "f");
}

int output_epd_writer(FILE *epd_file, gchar *name, int llx, int lly, int urx, int ury,
                      at_output_opts_type *opts, spline_list_array_type shape, at_msg_func msg_func,
                      gpointer msg_data, gpointer user_data)
{
  int result;

  result = output_epd_header(epd_file, name, llx, lly, urx, ury);
  if (result != 0)
    return result;

  out_splines(epd_file, shape);

  return 0;
}
