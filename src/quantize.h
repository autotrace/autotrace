/*
 * Copyright (C) 2001, 2002 Martin Weber
 * SPDX-FileCopyrightText: © 2000-2001 Martin Weber
 * SPDX-FileCopyrightText: © 2001-2003 Masatake YAMATO
 * SPDX-FileCopyrightText: © 2017-2025 Peter Lemenkov
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "autotrace.h"
#include "color.h"
#include "exception.h"
#include "input.h"
#include <stdio.h>

#ifndef QUANTIZE_H
#define QUANTIZE_H

#define PRECISION_R 7
#define PRECISION_G 7
#define PRECISION_B 7

#define HIST_R_ELEMS (1 << PRECISION_R)
#define HIST_G_ELEMS (1 << PRECISION_G)
#define HIST_B_ELEMS (1 << PRECISION_B)

#define MR HIST_G_ELEMS *HIST_B_ELEMS
#define MG HIST_B_ELEMS

typedef unsigned long ColorFreq;
typedef ColorFreq *Histogram;

typedef struct {
  int desired_number_of_colors; /* Number of colors we will allow */
  int actual_number_of_colors;  /* Number of colors actually needed */
  at_color cmap[256];           /* colormap created by quantization */
  ColorFreq freq[256];
  Histogram histogram; /* holds the histogram */
} QuantizeObj;

void quantize(at_bitmap *, long ncolors, const at_color *bgColor, QuantizeObj **,
              at_exception_type *exp);

void quantize_object_free(QuantizeObj *obj);
#endif /* NOT QUANTIZE_H */
