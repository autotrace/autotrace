/* output-dxf.h: utility routines for Autocad Drawing Exchange Format
   Copyright (C) 2000 Martin Weber */

#ifndef OUTPUT_DXF_H
#define OUTPUT_DXF_H

#include "output.h"

int output_dxf12_writer (FILE* file, at_string name,
			 int llx, int lly, int urx, int ury, int dpi,
			 spline_list_array_type shape,
			 at_msg_func msg_func, 
			 at_address msg_data);

#endif /* not OUTPUT_DXF_H */
