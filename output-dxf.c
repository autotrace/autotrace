/* output-dxf.c - output in partial dxf format with true splines.

   not functional yet.

   see output-dxf12 for splines as polylines
   Author: Reini Urban 2000-02-08 20:15
   Copyright (C) 2000 Martin Weber
*/
#include "spline.h"
#include "output-dxf.h"


static void
out_splines (FILE * file, spline_list_array_type shape)
{
    unsigned this_list;

    for (this_list = 0; this_list < SPLINE_LIST_ARRAY_LENGTH (shape);
	 this_list++)
    {
	unsigned this_spline;
	spline_list_type list = SPLINE_LIST_ARRAY_ELT (shape, this_list);
	spline_type first = SPLINE_LIST_ELT (list, 0);

	fputs("  0\nSPLINE\n", file);
    	fputs("  8\n0\n", file);        /* Layer */
    	fputs("210\n0.0\n220\n0.0\n230\n1.0\n", file);  /* Norm vector */

	fprintf(file, " 70\n%d\n", 8);  /* spline type: */
					/* 8: planar; 1 closed; 2 periodic; 16 linear */
	fprintf(file, " 71\n%d\n", 3);  			/* degree of curve */
	fprintf(file, " 72\n%d\n", 0);  			/* no of knots */
	fprintf(file, " 73\n%d\n", SPLINE_LIST_LENGTH (list));  /* no of control points */
	fprintf(file, " 74\n%d\n", SPLINE_LIST_LENGTH (list));  /* no of fit points */
	fprintf(file, " 42\n%d\n", 0.0000001);        		/* knot tolerance */
	fprintf(file, " 43\n%d\n", 0.0000001);        		/* control-point tolerance */
	fprintf(file, " 44\n%d\n", 0.0000000001);     		/* fit tolerance */

	fprintf(file, " 10\n%g\n 20\n%g\n 30\n0.0\n",
		START_POINT(first).x, START_POINT(first).y);

	for (this_spline = 0; this_spline < SPLINE_LIST_LENGTH (list);
	     this_spline++)
	{
	   /* 10 control points
	      11 fit points
	      40 knot values
	    */
	    spline_type s = SPLINE_LIST_ELT (list, this_spline);

	    if (SPLINE_DEGREE(s) == LINEARTYPE)
	    {
		fprintf(file, " 11\n%g\n 21\n%g\n 31\n0.0\n",
			END_POINT(s).x, END_POINT(s).y);
	    }
	    else
	    {
		fprintf(file, " 11\n%g\n 21\n%g\n 31\n0.0\n",
			END_POINT(s).x, END_POINT(s).y);
		fprintf(file, " 10\n%g\n 20\n%g\n 30\n0.0\n",
			CONTROL1(s).x, CONTROL1(s).y);
		fprintf(file, " 10\n%g\n 20\n%g\n 30\n0.0\n",
			CONTROL2(s).x, CONTROL2(s).y);
	    }
        }
    }
}


int output_dxf_writer(FILE* file, string name,
		     int llx, int lly, int urx, int ury,
		     spline_list_array_type shape)
{
    fputs("  0\nSECTION\n", file);
    fputs("  2\nENTITIES\n", file);

    out_splines(file, shape);

    fputs("  0\nENDSEC\n", file);
    fputs("  0\nEOF\n", file);
    return 0;
}

/* version 0.22 */