/* output.h: output routines
   Copyright (C) 1999 Bernhard Herzog. */

#ifndef OUTPUT_H
#define OUTPUT_H

#include "types.h"
#include "autotrace.h"

#define OUTPUT_SUFFIX_LIST "emf, eps, ai, sk, p2e, svg, swf, dxf, dxf12 and fig"

typedef at_output_write_func output_write;

output_write output_get_handler(string name);
char ** output_list (void);

#endif /* not OUTPUT_H */

/* version 0.17 */
