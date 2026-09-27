/*
 * SPDX-FileCopyrightText: © 2003 Serge Vakulenko
 * SPDX-FileCopyrightText: © 2017-2025 Peter Lemenkov
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

/* glyph-metrics.h: the font metrics the GF reader passes to the UGS
   writer.  module.c registers both handlers with a pointer to the same
   at_glyph_metrics as their user data.  */

#ifndef GLYPH_METRICS_H
#define GLYPH_METRICS_H

typedef struct {
  long charcode;
  long design_pixels; /* design size of the font in pixels */
  long advance_width;
  long left_bearing;
  long descend;
  long max_col;
  long max_row;
} at_glyph_metrics;

#endif /* not GLYPH_METRICS_H */
