/* output-mif.h: utility routines for FrameMaker MIF output
   Copyright (C) 2001 Per Grahn */

#ifndef OUTPUT_MIF_H
#define OUTPUT_MIF_H

#include "output.h"

int output_mif_writer (FILE* file, at_string name,
		       int llx, int lly, int urx, int ury, int dpi,
		       spline_list_array_type shape,
		       at_msg_func msg_func, 
		       at_address msg_data);


#endif /* not OUTPUT_MIF_H */

