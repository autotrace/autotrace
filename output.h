/* output.h: output routines
   Copyright (C) 1999 Bernhard Herzog. */

#ifndef OUTPUT_H
#define OUTPUT_H

#include "ptypes.h"
#include "autotrace.h"

#if HAVE_LIBSWF
#define SWF_SUFFIX "swf, "
#else 
#define SWF_SUFFIX ""
#endif /* HAVE_LIBSWF */

#define OUTPUT_SUFFIX_LIST "er, emf, eps, ai, sk, p2e, svg, " SWF_SUFFIX "dxf, epd, pdf and fig"

typedef at_output_write_func output_write;

output_write output_get_handler(string name);
char ** output_list (void);

#endif /* not OUTPUT_H */
