/* output-pdf.h: utility routines for PDF output. */

#ifndef OUTPUT_PDF_H
#define OUTPUT_PDF_H

#include <stdio.h>
#include "ptypes.h"
#include "spline.h"

int output_pdf_writer (FILE* file, string name,
		       int llx, int lly, int urx, int ury, int dpi,
		       spline_list_array_type shape);


#endif /* not OUTPUT_PDF_H */
