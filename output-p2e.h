/* output-p2e.h: utility routines for pstoedit intermediate format output
   Copyright (C) 2000 Martin Weber 
   Copyright (C) 2000 Wolfgang Glunz 
*/

#ifndef P2EOUT_H
#define P2EOUT_H

#include <stdio.h>
#include "types.h"
#include "spline.h"

int output_p2e_writer (FILE* file, string name,
		      int llx, int lly, int urx, int ury,
		      spline_list_array_type shape);


#endif /* not P2EOUT_H */

/* version 0.24a */
