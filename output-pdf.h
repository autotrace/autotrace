/* output-pdf.h: utility routines for PDF output. */

#ifndef OUTPUT_PDF_H
#define OUTPUT_PDF_H

#include <stdio.h>
#include "types.h"
#include "spline.h"

#include "output.h"

int output_pdf_writer (FILE* file, at_string name,
		       int llx, int lly, int urx, int ury, int dpi,
		       spline_list_array_type shape,
		       at_msg_func msg_func, 
		       at_address msg_data);

#endif /* not OUTPUT_PDF_H */
