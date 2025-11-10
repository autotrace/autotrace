/*
 * Copyright (C) 1999, 2000, 2001, 2003 Bernhard Herzog
 * SPDX-FileCopyrightText: © 2000-2003 Martin Weber
 * SPDX-FileCopyrightText: © 2001 Jerritt Collord
 * SPDX-FileCopyrightText: © 2001 Per Grahn
 * SPDX-FileCopyrightText: © 2001-2003 Masatake YAMATO
 * SPDX-FileCopyrightText: © 2017-2025 Peter Lemenkov
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "spline.h"
#include "output-sk.h"

static void out_splines(FILE *file, spline_list_array_type shape)
{
  unsigned this_list;
  spline_list_type list;

  for (this_list = 0; this_list < SPLINE_LIST_ARRAY_LENGTH(shape); this_list++) {
    unsigned this_spline;
    spline_type first;
    /* Whether to fill or stroke */
    gboolean stroke;

    list = SPLINE_LIST_ARRAY_ELT(shape, this_list);
    first = SPLINE_LIST_ELT(list, 0);

    stroke = shape.centerline || list.open;

    /* If stroke, set outline color and no fill, otherwise set fill
     * color and no outline */
    fprintf(file, "%s((%g,%g,%g))\n", stroke ? "lp" : "fp", list.color.r / 255.0,
            list.color.g / 255.0, list.color.b / 255.0);
    fputs(stroke ? "fe()\n" : "le()\n", file);

    /* Start a bezier object */
    fputs("b()\n", file);

    /* Move to the start point */
    fprintf(file, "bs(%g,%g,0)\n", START_POINT(first).x, START_POINT(first).y);

    /* write the splines */
    for (this_spline = 0; this_spline < SPLINE_LIST_LENGTH(list); this_spline++) {
      spline_type s = SPLINE_LIST_ELT(list, this_spline);
      if (SPLINE_DEGREE(s) == LINEARTYPE)
        fprintf(file, "bs(%g,%g,0)\n", END_POINT(s).x, END_POINT(s).y);
      else
        fprintf(file, "bc(%g,%g,%g,%g,%g,%g,0)\n", CONTROL1(s).x, CONTROL1(s).y, CONTROL2(s).x,
                CONTROL2(s).y, END_POINT(s).x, END_POINT(s).y);
    }

    /* End the bezier object. If it's stroked do nothing otherwise
       close the path. */
    if (!stroke)
      fputs("bC()\n", file);
  }
}

int output_sk_writer(FILE *file, gchar *name, int llx, int lly, int urx, int ury,
                     at_output_opts_type *opts, spline_list_array_type shape, at_msg_func msg_func,
                     gpointer msg_data, gpointer user_data)
{
  fputs("##Sketch 1 0\n", file);
  fputs("document()\n", file);
  fputs("layer('Layer 1',1,1,0,0)\n", file);
  fputs("guess_cont()\n", file);

  out_splines(file, shape);
  return 0;
}
