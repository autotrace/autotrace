/* quantize.h: Quantize a high color bitmap

   Copyright (C) 2001, 2002 Martin Weber

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

#include "bitmap.h"
#include "color.h"
#include "exception.h"

#ifndef QUANTIZE_H
#define QUANTIZE_H

#define PRECISION_R	7
#define PRECISION_G	7
#define PRECISION_B	7

#define HIST_R_ELEMS	(1<<PRECISION_R)
#define HIST_G_ELEMS	(1<<PRECISION_G)
#define HIST_B_ELEMS	(1<<PRECISION_B)

#define MR		HIST_G_ELEMS*HIST_B_ELEMS
#define MG		HIST_B_ELEMS

typedef unsigned long ColorFreq;
typedef ColorFreq *Histogram;

typedef struct {
  int desired_number_of_colors; /* Number of colors we will allow */
  int actual_number_of_colors;  /* Number of colors actually needed */
  at_color cmap[256];           /* colormap created by quantization */
  ColorFreq freq[256];
  Histogram histogram;          /* holds the histogram */
} QuantizeObj;

void quantize(at_bitmap *, long ncolors, const at_color * bgColor, QuantizeObj **, at_exception_type * exp);

void quantize_object_free(QuantizeObj * obj);
#endif /* NOT QUANTIZE_H */
