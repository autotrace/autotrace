/* output-swf.c - output in SWF format */



#include "spline.h"
#include "output-swf.h"

#define FPS 24.0
#define IMGID 1
#define IMGLAYER 1
#define SWFSCALE 20

static void
out_splines (SWFMovie m, spline_list_array_type shape, int height)
{
    unsigned this_list;

    for (this_list = 0; this_list < SPLINE_LIST_ARRAY_LENGTH (shape);
	 this_list++)
    {
        SWFShape k;

	unsigned this_spline;
	spline_list_type list = SPLINE_LIST_ARRAY_ELT (shape, this_list);
	spline_type first = SPLINE_LIST_ELT (list, 0);

        k = newSWFShape();
 /*       SWFShape_setLine(k, 10, 0x7f, 0, 0, 0xff); */
        SWFShape_setRightFill(k, SWFShape_addSolidFill(k, list.color.r, list.color.g, list.color.b, 0xff));
        SWFShape_movePenTo(k, SWFSCALE*START_POINT(first).x,
			     SWFSCALE*height - SWFSCALE*START_POINT(first).y);
     
	for (this_spline = 0; this_spline < SPLINE_LIST_LENGTH (list);
	     this_spline++)
	{
	    spline_type s = SPLINE_LIST_ELT (list, this_spline);

	    if (SPLINE_DEGREE(s) == LINEARTYPE)
	    {
                SWFShape_drawLineTo(k, SWFSCALE*END_POINT(s).x,
				      SWFSCALE*height - SWFSCALE*END_POINT(s).y);
	    }
	    else
	    {
		SWFShape_drawCubicTo (k, SWFSCALE*CONTROL1(s).x,
				      SWFSCALE*height - SWFSCALE*CONTROL1(s).y,
				      SWFSCALE*CONTROL2(s).x,
				      SWFSCALE*height - SWFSCALE*CONTROL2(s).y,
				      SWFSCALE*END_POINT(s).x,
				      SWFSCALE*height - SWFSCALE*END_POINT(s).y);

	    }
        }
        SWFMovie_add(m,k);
    }
}


int output_swf_writer(FILE* file, at_string name,
		      int llx, int lly, int urx, int ury, int dpi,
		      spline_list_array_type shape)
{
    int width = urx - llx;
    int height = ury - lly;
    SWFMovie m;

    Ming_init();
    Ming_setCubicThreshold(20000);

    m = newSWFMovie();

    out_splines(m, shape, height);

    SWFMovie_setDimension(m, SWFSCALE*(float)width, SWFSCALE*(float)height);
    SWFMovie_setRate(m, FPS);
    SWFMovie_nextFrame(m);
    SWFMovie_output(m, fileOutputMethod, file);
    return 0;
}

