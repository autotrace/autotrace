#ifndef OUTPUT_CGM_H
#define OUTPUT_CGM_H

#include "output.h"

int output_cgm_writer(FILE* file, at_string name,
		      int llx, int lly, int urx, int ury, int dpi,
		      spline_list_array_type shape,
		      at_msg_func msg_func, 
		      at_address msg_data);

#endif /* not OUTPUT_CGM_H */
