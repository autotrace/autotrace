/* output-fig.h - use with output-fig.c
   Copyright (C) 1999 Ian MacPhedran */

#ifndef OUTPUT_FIG_H
#define OUTPUT_FIG_H

#include "output.h"

int output_fig_writer(FILE* file, at_string name,
		      int llx, int lly, int urx, int ury, int dpi,
			spline_list_array_type shape);

#endif /* not OUTPUT_FIG_H */

