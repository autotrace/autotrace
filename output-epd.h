/* output-epd.h: utility routines for EPD output. */

#ifndef OUTPUT_EPD_H
#define OUTPUT_EPD_H

#include <stdio.h>
#include "ptypes.h"
#include "spline.h"

int output_epd_writer (FILE* file, string name,
		      int llx, int lly, int urx, int ury,
		      spline_list_array_type shape);


#endif /* not OUTPUT_EPD_H */
