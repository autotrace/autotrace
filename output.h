/* output.h: output routines
   Copyright (C) 1999 Bernhard Herzog. */

#ifndef OUTPUT_H
#define OUTPUT_H

#include <stdio.h>
#include "types.h"
#include "spline.h"

typedef int (*output_write)(FILE*, string name,
			    int llx, int lly, int urx, int ury,
			    spline_list_array_type shape);


output_write output_get_handler(string name);
void output_list_formats(FILE *);


#endif /* not OUTPUT_H */

/* version 0.17 */
