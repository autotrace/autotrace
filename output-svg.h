/* output-svg.h - output in SVG format
   Copyright (C) 1999 Bernhard Herzog. */

#ifndef OUTPUT_SVG_H
#define OUTPUT_SVG_H

#include <stdio.h>
#include "ptypes.h"
#include "spline.h"

int output_svg_writer(FILE* file, string name,
		      int llx, int lly, int urx, int ury,
		      spline_list_array_type shape);


#endif /* not OUTPUT_SVG_H */

/* version 0.17 */
