/* input.h: input routines
   Copyright (C) 1999 Bernhard Herzog. */

#ifndef INPUT_H
#define INPUT_H 

#include "autotrace.h"
#include "ptypes.h"

typedef at_input_read_func input_read;
input_read input_get_handler (string filename);
input_read input_get_handler_by_suffix (string suffix);
char ** input_list (void);
char * input_shortlist (void);

#endif /* Not def: INPUT_H */
