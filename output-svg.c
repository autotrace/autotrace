/* output-svg.h - output in SVG format
   Copyright (C) 1999, 2000 Bernhard Herzog. */

#include "spline.h"
#include "output-svg.h"


static void
out_splines (FILE * file, spline_list_array_type shape, int height)
{
    unsigned this_list;

    for (this_list = 0; this_list < SPLINE_LIST_ARRAY_LENGTH (shape);
	 this_list++)
    {
	unsigned this_spline;
	spline_list_type list = SPLINE_LIST_ARRAY_ELT (shape, this_list);
	spline_type first = SPLINE_LIST_ELT (list, 0);

	fprintf(file, "<path style=\"fill:#%02x%02x%02x; stroke:none\" d=\"",
		list.color.r, list.color.g, list.color.b);
	fprintf(file, "M%g %g",
		START_POINT(first).x, height - START_POINT(first).y);
      
	for (this_spline = 0; this_spline < SPLINE_LIST_LENGTH (list);
	     this_spline++)
	{
	    spline_type s = SPLINE_LIST_ELT (list, this_spline);

	    if (SPLINE_DEGREE(s) == LINEARTYPE)
	    {
		fprintf(file, "L%g %g",
			END_POINT(s).x, height - END_POINT(s).y);
	    }
	    else
	    {
		fprintf(file, "C%g %g %g %g %g %g",
			CONTROL1(s).x, height - CONTROL1(s).y,
			CONTROL2(s).x, height - CONTROL2(s).y,
			END_POINT(s).x, height - END_POINT(s).y);
	    }
        }
	fputs("z", file);
	fputs("\"/>\n", file);
    }
}


int output_svg_writer(FILE* file, string name,
		     int llx, int lly, int urx, int ury,
		     spline_list_array_type shape)
{
    int width = urx - llx;
    int height = ury - lly;
    fputs("<?xml version=\"1.0\" standalone=\"yes\"?>\n", file);
    fprintf(file, "<svg width=\"%d\" height=\"%d\">\n", width, height);

    out_splines(file, shape, height);
    fputs("</svg>\n", file);
    
    return 0;
}

/* version 0.17 */
