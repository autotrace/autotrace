/*
 * Copyright (C) 2000, 2001 Enrico Persiani
 * SPDX-FileCopyrightText: © 2000 Enrico Persiani
 * SPDX-FileCopyrightText: © 2001 Martin Weber
 * SPDX-FileCopyrightText: © 2001 Per Grahn
 * SPDX-FileCopyrightText: © 2001-2003 Masatake YAMATO
 * SPDX-FileCopyrightText: © 2017-2025 Peter Lemenkov
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#ifndef __OUTPUT_EMF_H
#define __OUTPUT_EMF_H

#include "output.h"

int output_emf_writer(FILE *file, gchar *name, int llx, int lly, int urx, int ury,
                      at_output_opts_type *opts, at_spline_list_array_type shape,
                      at_msg_func msg_func, gpointer msg_data, gpointer user_data);

#endif /* __OUTPUT_EMF_H */
