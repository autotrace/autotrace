/* output.h: output routines
   Copyright (C) 1999 Bernhard Herzog. */

#ifndef OUTPUT_H
#define OUTPUT_H

#include "ptypes.h"
#include "autotrace.h"

typedef at_output_write_func output_write;

output_write output_get_handler(string name);
char ** output_list (void);
char * output_shortlist (void);

#endif /* not OUTPUT_H */
