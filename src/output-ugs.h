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

#ifndef OUTPUT_UGS_H
#define OUTPUT_UGS_H

#include "output.h"

int output_ugs_writer(FILE * file, gchar * name, int llx, int lly, int urx, int ury, at_output_opts_type * opts, at_spline_list_array_type shape, at_msg_func msg_func, gpointer msg_data, gpointer user_data);

extern long ugs_charcode;
extern long ugs_design_pixels;
extern long ugs_advance_width;
extern long ugs_left_bearing, ugs_descend;
extern long ugs_max_col, ugs_max_row;

#endif /* not OUTPUT_UGS_H */
