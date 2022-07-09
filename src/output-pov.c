/*
 * Copyright (C) 2003 Martin Weber
 * SPDX-FileCopyrightText: © 2003 Martin Weber
 * SPDX-FileCopyrightText: © 2003 Masatake YAMATO
 * SPDX-FileCopyrightText: © 2017-2025 Peter Lemenkov
 * SPDX-FileCopyrightText: © 2020 Han Mertens
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "spline.h"
#include "color.h"
#include "output-pov.h"
#include "logreport.h"
#include "autotrace.h"

/* Output macros.  */

/* This should be used for outputting a string S on a line by itself.  */
#define OUT_LINE(s) fprintf(pov_file, "%s\n", s)

/* These output their arguments, preceded by the indentation.  */
#define OUT(s, ...) fprintf(pov_file, s, __VA_ARGS__)

/* This outputs the Povray code which produces the shape in
   SHAPE.  */

static void out_splines(FILE *pov_file, spline_list_array_type shape)
{
  unsigned this_list;
  spline_list_type list;

  at_color last_color = {0, 0, 0};

  for (this_list = 0; this_list < SPLINE_LIST_ARRAY_LENGTH(shape); this_list++) {
    unsigned this_spline;
    spline_type first;
    unsigned test_list;
    unsigned number = 0;

    list = SPLINE_LIST_ARRAY_ELT(shape, this_list);

    for (test_list = this_list; test_list < SPLINE_LIST_ARRAY_LENGTH(shape); test_list++) {
      spline_list_type testlist = SPLINE_LIST_ARRAY_ELT(shape, test_list);
      if (!at_color_equal(&testlist.color, &list.color))
        break;
      number += SPLINE_LIST_LENGTH(testlist) * 4;
    }

    first = SPLINE_LIST_ELT(list, 0);

    if (this_list > 0) {
      if (!at_color_equal(&list.color, &last_color)) {
        OUT("\n  pigment {rgb<%.3f, %.3f, %.3f>}\n", (double)last_color.r / 255.0,
            (double)last_color.g / 255.0, (double)last_color.b / 255.0);
        OUT_LINE("  translate <0.0, 0.0, 0.0>");
        OUT_LINE("}");
      } else
        OUT_LINE(",");
    }

    if (this_list == 0 || !at_color_equal(&list.color, &last_color)) {
      OUT_LINE("prism {");
      OUT_LINE("  bezier_spline");
      OUT("  %.1f\n", 0.0);
      OUT("  %.4f\n", 0.0001);
      OUT("  %d\n", number);
      last_color = list.color;
    }

    for (this_spline = 0; this_spline < SPLINE_LIST_LENGTH(list); this_spline++) {
      spline_type s = SPLINE_LIST_ELT(list, this_spline);

      if (this_spline > 0)
        OUT_LINE(",");

      if (SPLINE_DEGREE(s) == LINEARTYPE)
        OUT("  <%.3f, %.3f>, <%.3f, %.3f>, <%.3f, %.3f>, <%.3f, %.3f>", START_POINT(s).x,
            START_POINT(s).y, START_POINT(s).x, START_POINT(s).y, END_POINT(s).x, END_POINT(s).y,
            END_POINT(s).x, END_POINT(s).y);
      else
        OUT("  <%.3f, %.3f>, <%.3f, %.3f>, <%.3f, %.3f>, <%.3f, %.3f>", START_POINT(s).x,
            START_POINT(s).y, CONTROL1(s).x, CONTROL1(s).y, CONTROL2(s).x, CONTROL2(s).y,
            END_POINT(s).x, END_POINT(s).y);
    }
  }
  if (SPLINE_LIST_ARRAY_LENGTH(shape) > 0) {
    OUT_LINE("");
    OUT("  pigment {rgb<%.3f, %.3f, %.3f>}\n", (double)list.color.r / 255.0,
        (double)list.color.g / 255.0, (double)list.color.b / 255.0);
    OUT_LINE("  translate <0.0, 0.0, 0.0>");
    OUT_LINE("}");
  }
}

int output_pov_writer(FILE *pov_file, gchar *name, int llx, int lly, int urx, int ury,
                      at_output_opts_type *opts, spline_list_array_type shape, at_msg_func msg_func,
                      gpointer msg_data, gpointer user_data)
{
  if (shape.centerline == TRUE)
    FATAL("Povray output currently not supported for centerline method");

  out_splines(pov_file, shape);

  return 0;
}
