/* output-swf.h - output in SWF format

Copyright (C) 2000 Kevin O' Gorman - but uses Paul Haeberli's non-free
swf library (http://reality.sgi.com/grafica/flash/) */

#ifndef OUTPUT_SWF_H
#define OUTPUT_SWF_H

#include <stdio.h>
#include "types.h"
#include "spline.h"
#include "swf.h"

int output_swf_writer(FILE* file, string name,
		      int llx, int lly, int urx, int ury,
		      spline_list_array_type shape);


#endif /* not OUTPUT_SWF_H */

/* version 0.17 */
