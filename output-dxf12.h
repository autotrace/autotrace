/* output-dxf12.h - output in AutoCAD R14 DXF format (without SPLINE entity)
   see output-dxf for true splines
   Author: Reini Urban 2000-02-08 19:58
   Copyright (C) 2000 Martin Weber
 */

#ifndef OUTPUT_DXF12_H
#define OUTPUT_DXF12_H

#include <stdio.h>
#include "types.h"
#include "spline.h"

int output_dxf12_writer(FILE* file, string name,
		     int llx, int lly, int urx, int ury,
		     spline_list_array_type shape);


#endif /* not OUTPUT_DXF12_H */

/* version 0.22 */