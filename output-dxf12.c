/* output-dxf12.c - output in partial dxf format without splines.

   not functional yet.

   see output-dxf for true splines
   Author: Reini Urban
   Copyright (C) 2000 Martin Weber
*/

#include "spline.h"
#include "output-dxf12.h"


static void
out_splines (FILE * file, spline_list_array_type shape)
{
    unsigned this_list;

    for (this_list = 0; this_list < SPLINE_LIST_ARRAY_LENGTH (shape);
	 this_list++)
    {
	unsigned int i;
	spline_list_type list = SPLINE_LIST_ARRAY_ELT (shape, this_list);
	spline_type first = SPLINE_LIST_ELT (list, 0);

	fputs("  0\nPOLYLINE\n", file);
    	fputs("  8\n0\n", file);        /* Layer */
    	fputs(" 66\n1\n", file);        /* vertices follow */
	fprintf(file, " 10\n0.0\n 20\n0.0\n 30\n0.0\n");
	fprintf(file, " 70\n%d\n", 8);  /* polyline type: */

	fputs("  0\nVERTEX\n", file);
    	fputs("  8\n0\n", file);        /* Layer */
	fprintf(file, " 10\n%g\n 20\n%g\n 30\n0.0\n",
		START_POINT(first).x, START_POINT(first).y);

	for (i = 0; i < SPLINE_LIST_LENGTH (list); i++)
	{
	    spline_type s = SPLINE_LIST_ELT (list, i);

	    fputs("  0\nVERTEX\n", file);
    	    fputs("  8\n0\n", file);        /* Layer */
	    if (SPLINE_DEGREE(s) == LINEARTYPE)
	    {
		fprintf(file, " 10\n%g\n 20\n%g\n 30\n0.0\n",
			END_POINT(s).x, END_POINT(s).y);
		fprintf(file, " 70\n%d\n", 0);  /* vertex type */
	    }
	    else
	    {
		fprintf(file, " 10\n%g\n 20\n%g\n 30\n0.0\n",
			END_POINT(s).x, END_POINT(s).y);
		fprintf(file, " 70\n%d\n", 32);   /* vertex type */
		fprintf(file, " 42\n%g\n", 0.0);  /* bulge */
	    }
        }
	fputs("  0\nSEQEND\n", file);
    	fputs("  8\n0\n", file);
    }
}


int output_dxf12_writer(FILE* file, string name,
		     int llx, int lly, int urx, int ury,
		     spline_list_array_type shape)
{
/*
    fputs("  0\nSECTION\n", file);
    fputs("  2\nENTITIES\n", file);

    out_splines(file, shape);

    fputs("  0\nENDSEC\n", file);
    fputs("  0\nEOF\n", file);
 */
    return 0;
}

/* version 0.22 */