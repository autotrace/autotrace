/* output-svg.h - output in SVG format
   Copyright (C) 1999 Bernhard Herzog. */

#ifndef OUTPUT_SVG_H
#define OUTPUT_SVG_H

#include "output.h"

int output_svg_writer(FILE* file, at_string name,
		      int llx, int lly, int urx, int ury, int dpi,
		      spline_list_array_type shape,
		      at_msg_func msg_func, 
		      at_address msg_data);


#endif /* not OUTPUT_SVG_H */

