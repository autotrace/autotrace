/*
**  output-emf.h 
**  output in Enhanced Metafile format
**  20-09-2000 - First Release, Enrico Persiani. 
*/

#ifndef __OUTPUT_EMF_H
#define __OUTPUT_EMF_H

#include <stdio.h>
#include "ptypes.h"
#include "spline.h"

int output_emf_writer(FILE* file, string name,
		      int llx, int lly, int urx, int ury, int dpi,
		      spline_list_array_type shape);


#endif /* __OUTPUT_EMF_H */
