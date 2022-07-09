/*
 * Copyright (C) 2003 Serge Vakulenko
 * SPDX-FileCopyrightText: © 2000 Enrico Persiani
 * SPDX-FileCopyrightText: © 2001 Martin Weber
 * SPDX-FileCopyrightText: © 2001 Per Grahn
 * SPDX-FileCopyrightText: © 2001-2003 Masatake YAMATO
 * SPDX-FileCopyrightText: © 2003 Serge Vakulenko
 * SPDX-FileCopyrightText: © 2017-2025 Peter Lemenkov
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#ifndef OUTPUT_UGS_H
#define OUTPUT_UGS_H

#include "output.h"

int output_ugs_writer(FILE *file, gchar *name, int llx, int lly, int urx, int ury,
                      at_output_opts_type *opts, at_spline_list_array_type shape,
                      at_msg_func msg_func, gpointer msg_data, gpointer user_data);

extern long ugs_charcode;
extern long ugs_design_pixels;
extern long ugs_advance_width;
extern long ugs_left_bearing, ugs_descend;
extern long ugs_max_col, ugs_max_row;

#endif /* not OUTPUT_UGS_H */
