/* output-swf.h - output in SWF format
Written by Kevin O' Gorman <spidey@maths.tcd.ie>
Uses the Ming SWF library from http://www.opaque.net/ming/
*/

#ifndef OUTPUTSWF_H
#define OUTPUTSWF_H 

#include <stdio.h>
#include "types.h"
#include "spline.h"
#include <ming.h>

int output_swf_writer(FILE* file, string name,
		      int llx, int lly, int urx, int ury,
		      spline_list_array_type shape);

#endif /* Not def: OUTPUTSWF_H */
