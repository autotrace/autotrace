/* output-sk.h - output in sketch format
   Copyright (C) 1999 Bernhard Herzog. */

#ifndef OUTPUT_SK_H
#define OUTPUT_SK_H

#include <stdio.h>
#include "types.h"
#include "spline.h"

int output_sk_writer(FILE* file, string name,
		     int llx, int lly, int urx, int ury,
		     spline_list_array_type shape);


#endif /* not OUTPUT_SK_H */

/* version 0.17 */
