/* output-fig.h - use with output-fig.c
   Copyright (C) 1999 Ian MacPhedran */

#ifndef OUTPUT_FIG_H
#define OUTPUT_FIG_H

#include <stdio.h>
#include "ptypes.h"
#include "spline.h"
#include "color.h"

int output_fig_writer(FILE* file, string name,
			int llx, int lly, int urx, int ury,
			spline_list_array_type shape);

/* use FIG_X and FIG_Y to convert from local units (pixels) to FIG ones */
/* assume 1 pixel is equal to 1/80 inches (old FIG unit) */
/* Offset by 300 units (1/4 inch) */

#define FIG_X(x) (int)((x * 15.0) + 300.0)
#define FIG_Y(y) (int)(((ury - y) * 15.0) + 300.0)

/* the basic colours */
#define FIG_BLACK	0
#define FIG_BLUE	1
#define FIG_GREEN	2
#define FIG_CYAN	3
#define FIG_RED		4
#define FIG_MAGENTA	5
#define FIG_YELLOW	6
#define FIG_WHITE	7

#endif /* not OUTPUT_FIG_H */

/* version 0.24a */
