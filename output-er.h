/* output-er.h: utility routines for Elastic Reality shape file output */

#ifndef EROUT_H
#define EROUT_H

#include "output.h"

int output_er_writer(FILE* file, at_string name,
		     int llx, int lly, int urx, int ury, int dpi,
		     spline_list_array_type shape,
		     at_msg_func msg_func, 
		     at_address msg_data);

#endif
