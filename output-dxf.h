/* output-dxf.h: utility routines for Autocad Drawing Exchange Format
   Copyright (C) 2000 Martin Weber */

#ifndef OUTPUT_DXF_H
#define OUTPUT_DXF_H

#include <stdio.h>
#include "ptypes.h"
#include "spline.h"

int output_dxf12_writer (FILE* file, string name,
		      int llx, int lly, int urx, int ury,
		      spline_list_array_type shape);

#endif /* not OUTPUT_DXF_H */
