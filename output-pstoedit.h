/* output-pstoedit.h: utility routines for libpstoedit.so function

   Copyright (C) 2002 Masatake YAMATO

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public License
   as published by the Free Software Foundation; either version 2.1 of
   the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with this library; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
   USA. */

#ifndef OUTPUT_PSTOEDIT_H
#define OUTPUT_PSTOEDIT_H 

/* These should be in pstoedit.h */
#include <pstoedll.h>
int pstoedit_plainC(int argc,
		    const char * const argv[],
		    const char * const psinterpreter  /* if 0, then pstoedit will look for one using whichpi() */
		    );
struct DriverDescription_S* getPstoeditDriverInfo_plainC(void);
void pstoedit_checkversion (unsigned int callersversion);


#include "output.h"

int output_pstoedit_writer (FILE* file, at_string name,
			    int llx, int lly, int urx, int ury, int dpi,
			    at_spline_list_array_type shape,
			    at_msg_func msg_func, 
			    at_address msg_data);

at_bool output_pstoedit_is_unusable_writer(at_string name);

#endif /* Not def: OUTPUTPSTOEDIT_H */
