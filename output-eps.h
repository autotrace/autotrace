/* output-eps.h: utility routines for PostScript output
   Copyright (C) 2000 Martin Weber */

#ifndef PSOUT_H
#define PSOUT_H

#include <stdio.h>
#include "ptypes.h"
#include "spline.h"

int output_eps_writer (FILE* file, string name,
		       int llx, int lly, int urx, int ury, int dpi,
		       spline_list_array_type shape);


#endif /* not PSOUT_H */

/* version 0.22 */
