/* output-mif.h: utility routines for FrameMaker MIF output
   Copyright (C) 2001 Per Grahn */

#ifndef OUTPUT_MIF_H
#define OUTPUT_MIF_H

#include <stdio.h>
#include "types.h"
#include "spline.h"

int output_mif_writer (FILE* file, at_string name,
		       int llx, int lly, int urx, int ury, int dpi,
		       spline_list_array_type shape);


#endif /* not OUTPUT_MIF_H */

