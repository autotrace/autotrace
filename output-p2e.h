/* output-p2e.h: utility routines for pstoedit intermediate format output
   Copyright (C) 2000 Martin Weber 
   Copyright (C) 2000 Wolfgang Glunz 
*/

#ifndef OUTPUT_P2E_H
#define OUTPUT_P2E_H

#include "output.h"

int output_p2e_writer (FILE* file, at_string name,
		       int llx, int lly, int urx, int ury, int dpi,
		       spline_list_array_type shape);


#endif /* not OUTPUT_P2E_H */

