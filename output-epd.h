/* output-epd.h: utility routines for EPD output. */

#ifndef OUTPUT_EPD_H
#define OUTPUT_EPD_H

#include "output.h"

int output_epd_writer (FILE* file, at_string name,
		       int llx, int lly, int urx, int ury, int dpi,
		      spline_list_array_type shape);


#endif /* not OUTPUT_EPD_H */
