/* output-er.h: utility routines for Elastic Reality shape file output */

#ifndef EROUT_H
#define EROUT_H

#include <stdio.h>
#include "ptypes.h"
#include "spline.h"

int output_er_writer(FILE* file, string name,
    int llx, int lly, int urx, int ury,
    spline_list_array_type shape);

#endif
