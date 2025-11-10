/*
 * Copyright (C) 1999, 2000, 2001 Martin Weber
 * SPDX-FileCopyrightText: © 2001 Martin Weber
 * SPDX-FileCopyrightText: © 2001 Per Grahn
 * SPDX-FileCopyrightText: © 2001-2003 Masatake YAMATO
 * SPDX-FileCopyrightText: © 2017-2025 Peter Lemenkov
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#ifndef OUTPUT_PDF_H
#define OUTPUT_PDF_H

#include <stdio.h>
#include "spline.h"

#include "output.h"

int output_pdf_writer(FILE *file, gchar *name, int llx, int lly, int urx, int ury,
                      at_output_opts_type *opts, spline_list_array_type shape, at_msg_func msg_func,
                      gpointer msg_data, gpointer user_data);

#endif /* not OUTPUT_PDF_H */
