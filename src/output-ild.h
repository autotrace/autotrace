/*
 * Copyright (C) 2004 Steven P. Hirshman
 * Copyright (C) 2005 Robin Adams
 * SPDX-FileCopyrightText: © 2000-2001 Martin Weber
 * SPDX-FileCopyrightText: © 2001 Per Grahn
 * SPDX-FileCopyrightText: © 2001-2003 Masatake YAMATO
 * SPDX-FileCopyrightText: © 2004 Steven P. Hirshman
 * SPDX-FileCopyrightText: © 2005 Robin Adams
 * SPDX-FileCopyrightText: © 2017-2025 Peter Lemenkov
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#ifndef __OUTPUT_ILD_H
#define __OUTPUT_ILD_H

#include "output.h"

int output_ild_writer(FILE *file, gchar *name, int llx, int lly, int urx, int ury,
                      at_output_opts_type *opts, at_spline_list_array_type shape,
                      at_msg_func msg_func, gpointer msg_data, gpointer user_data);

#endif /* __OUTPUT_ILD_H */
