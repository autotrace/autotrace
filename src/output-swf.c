/*
 * Copyright (C) 1999, 2000, 2001 Kevin O' Gorman
 * SPDX-FileCopyrightText: © 2000-2002 Martin Weber
 * SPDX-FileCopyrightText: © 2001 Kevin O' Gorman
 * SPDX-FileCopyrightText: © 2001 Per Grahn
 * SPDX-FileCopyrightText: © 2001-2003 Masatake YAMATO
 * SPDX-FileCopyrightText: © 2017-2025 Peter Lemenkov
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "spline.h"
#include "color.h"
#include "output-swf.h"
#include <ming.h>

#define FPS 24.0
#define IMGID 1
#define IMGLAYER 1
#define SWFSCALE 20

static void out_splines(SWFMovie m, spline_list_array_type shape, int height)
{
  unsigned this_list;
  at_color last_color = {0, 0, 0};

  for (this_list = 0; this_list < SPLINE_LIST_ARRAY_LENGTH(shape); this_list++) {
    SWFShape k;

    unsigned this_spline;
    spline_list_type list = SPLINE_LIST_ARRAY_ELT(shape, this_list);
    spline_type first = SPLINE_LIST_ELT(list, 0);

    if (this_list == 0 || !at_color_equal(&list.color, &last_color)) {
      k = newSWFShape();
      SWFShape_setRightFill(
          k, SWFShape_addSolidFill(k, list.color.r, list.color.g, list.color.b, 0xff));
      last_color = list.color;
    }
    SWFShape_movePenTo(k, SWFSCALE * START_POINT(first).x,
                       SWFSCALE * height - SWFSCALE * START_POINT(first).y);

    for (this_spline = 0; this_spline < SPLINE_LIST_LENGTH(list); this_spline++) {
      spline_type s = SPLINE_LIST_ELT(list, this_spline);

      if (SPLINE_DEGREE(s) == LINEARTYPE) {
        SWFShape_drawLineTo(k, SWFSCALE * END_POINT(s).x,
                            SWFSCALE * height - SWFSCALE * END_POINT(s).y);
      } else {
        SWFShape_drawCubicTo(
            k, SWFSCALE * CONTROL1(s).x, SWFSCALE * height - SWFSCALE * CONTROL1(s).y,
            SWFSCALE * CONTROL2(s).x, SWFSCALE * height - SWFSCALE * CONTROL2(s).y,
            SWFSCALE * END_POINT(s).x, SWFSCALE * height - SWFSCALE * END_POINT(s).y);
      }
    }
    SWFMovie_add(m, k);
  }
}

int output_swf_writer(FILE *file, gchar *name, int llx, int lly, int urx, int ury,
                      at_output_opts_type *opts, spline_list_array_type shape, at_msg_func msg_func,
                      gpointer msg_data, gpointer user_data)
{
  int width = urx - llx;
  int height = ury - lly;
  SWFMovie m;

#ifdef _WINDOWS
  if (file == stdout) {
    fprintf(stderr, "This driver couldn't write to stdout!\n");
    return -1;
  }
#endif

  Ming_init();
  Ming_setCubicThreshold(20000);

  m = newSWFMovie();

  out_splines(m, shape, height);

  SWFMovie_setDimension(m, SWFSCALE * (float)width, SWFSCALE * (float)height);
  SWFMovie_setRate(m, FPS);
  SWFMovie_nextFrame(m);
  SWFMovie_output(m, fileOutputMethod, file);
  return 0;
}
