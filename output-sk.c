/* output-sk.h - output in sketch format
   Copyright (C) 1999, 2000 Bernhard Herzog. */

#include "spline.h"
#include "output-sk.h"


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

	fprintf(file, (at_centerline || list.open) ? "lp((%g,%g,%g))\n"
		: "fp((%g,%g,%g))\n", list.color.r / 255.0,
		list.color.g / 255.0, list.color.b / 255.0); 
	fputs((at_centerline || list.open) ? "fe()\n" : "le()\n", file); /* no outline */
    
	fputs("b()\n", file); /* the beginning of a bezier object */
	fprintf(file, "bs(%g,%g,0)\n",
		START_POINT(first).x, START_POINT(first).y);
      
	for (this_spline = 0; this_spline < SPLINE_LIST_LENGTH (list);
	     this_spline++)
	{
	    spline_type s = SPLINE_LIST_ELT (list, this_spline);

	    if (SPLINE_DEGREE(s) == LINEARTYPE)
	    {
		fprintf(file, "bs(%g,%g,0)\n", END_POINT(s).x, END_POINT(s).y);
	    }
	    else
	    {
		fprintf(file, "bc(%g,%g,%g,%g,%g,%g,0)\n",
			CONTROL1(s).x, CONTROL1(s).y,
			CONTROL2(s).x, CONTROL2(s).y,
			END_POINT(s).x, END_POINT(s).y);
	    }
        }
	if (!at_centerline)
	  fputs("bC()\n", file);
    }
}


int output_sk_writer(FILE* file, string name,
		     int llx, int lly, int urx, int ury,
		     spline_list_array_type shape)
{
    fputs("##Sketch 1 0\n", file);
    fputs("document()\n", file);
    fputs("layer('Layer 1',1,1,0,0)\n", file);
    fputs("guess_cont()\n", file);

    out_splines(file, shape);
    return 0;
}
