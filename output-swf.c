/* output-swf.h - output in SWF format

Copyright (C) 1999 Kevin O' Gorman - but see header file for notes on swf 
library. */

#include "spline.h"
#include "output-swf.h"

#define FPS 24.0
#define IMGID 1
#define IMGLAYER 1

void play(void)
{
    swf_startdoaction();
    swf_actionPlay();
    swf_enddoaction();
}

void stop(void) {
   swf_startdoaction();
   swf_actionStop();
   swf_enddoaction();
}

static void
out_splines (FILE * file, spline_list_array_type shape, int height)
{
    unsigned this_list;

    swf_shapefillsolid (0.0, 0.0, 0.0, 1.0); 
    swf_shapelinesolid (0.0, 0.0, 0.0, 0.0, 0.0);

    for (this_list = 0; this_list < SPLINE_LIST_ARRAY_LENGTH (shape);
	 this_list++)
    {
	unsigned this_spline;
	spline_list_type list = SPLINE_LIST_ARRAY_ELT (shape, this_list);
	spline_type first = SPLINE_LIST_ELT (list, 0);

        swf_shapemoveto (START_POINT(first).x, START_POINT(first).y);
     
	for (this_spline = 0; this_spline < SPLINE_LIST_LENGTH (list);
	     this_spline++)
	{
	    spline_type s = SPLINE_LIST_ELT (list, this_spline);

	    if (SPLINE_DEGREE(s) == LINEARTYPE)
	    {
                swf_shapelineto (END_POINT(s).x, END_POINT(s).y);
	    }
	    else
	    {
	     	swf_shapecurveto3 (CONTROL1(s).x, CONTROL1(s).y,
        	     		   CONTROL2(s).x, CONTROL2(s).y,
	     		           END_POINT(s).x, END_POINT(s).y);
             

	    }
        }
    }
}


int output_swf_writer(FILE* file, string name,
		     int llx, int lly, int urx, int ury,
		     spline_list_array_type shape)
{
    int width = urx - llx;
    int height = ury - lly;
    swf_openfile (name, (float) width, (float) height, FPS, 1.0, 1.0, 1.0 );
    play();
    swf_startshape (IMGID);    
    out_splines(file, shape, height);
    swf_endshape();

    swf_placeobject(IMGID, IMGLAYER);
    swf_showframe();
    stop();
    swf_closefile();
    return 0;
}

/* version 0.17 */
