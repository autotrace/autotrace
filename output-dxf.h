/* output-dxf.h - output in AutoCAD R14 DXF format (with SPLINE entity)
   see output-dxf12 for splines as polylines
   Author: Reini Urban 2000-02-08 19:58
   Copyright (C) 2000 Martin Weber */


#ifndef OUTPUT_DXF_H
#define OUTPUT_DXF_H

#include <stdio.h>
#include "types.h"
#include "spline.h"

int output_dxf_writer (FILE* file, string name,
		     int llx, int lly, int urx, int ury,
		     spline_list_array_type shape);


#endif /* not OUTPUT_DXF_H */

/* version 0.22 */