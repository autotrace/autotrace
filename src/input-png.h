/*
 * Copyright (C) 2000 MenTaLguY
 * SPDX-FileCopyrightText: © 2000 MenTaLguY
 * SPDX-FileCopyrightText: © 2001-2003 Masatake YAMATO
 * SPDX-FileCopyrightText: © 2017-2025 Peter Lemenkov
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

/* $Id: input-png.h,v 1.10 2003/08/17 10:18:27 masata-y Exp $ */

#ifndef INPUT_PNG_H
#define INPUT_PNG_H

#include "input.h"

at_bitmap input_png_reader(gchar *filename, at_input_opts_type *opts, at_msg_func msg_func,
                           gpointer msg_data, gpointer user_data);

#endif /* not INPUT_PNG_H */
