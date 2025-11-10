/*
 * Copyright (C) 2003 Martin Weber
 * Copyright (C) 2003 Masatake YAMATO
 * SPDX-FileCopyrightText: © 2003 Masatake YAMATO
 * SPDX-FileCopyrightText: © 2017-2018 Peter Lemenkov
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef PRIVATE_H
#define PRIVATE_H

#include "autotrace.h"
#include "input.h"
#include "output.h"

struct _at_bitmap_reader {
  at_input_func func;
  gpointer data;
};

struct _at_spline_writer {
  at_output_func func;
  gpointer data;
};

int at_input_init(void);
int at_output_init(void);
int at_param_init(void);
int at_module_init(void);
#endif /* Not def: PRIVATE_H */
