/* output-er.c: utility routines for Elastic Reality shape file output */

#include "usefull.h"
#include "types.h"
#include "spline.h"
#include "output-er.h"
#include "xstd.h"
#include <time.h>
#include <string.h>
#include <malloc.h>

extern string version_string;

static string now(void);

#define NUM_CORRESP_POINTS 4


/* This should be called before the others in this file.  It opens the
   output file and writes some preliminary boilerplate. */

static int
output_er_header(FILE* er_file, string name, int llx, int lly, int urx, int ury)
{
    string time;

    fprintf(er_file, "#Elastic Reality Shape File\n\n#Date: %s\n\n",
	time = now());

    free(time);

    return 0;
}




/* This outputs shape data and the point list for the shape in SHAPE. */

static void
out_splines(FILE* er_file, spline_list_array_type shape,
    unsigned width, unsigned height)
{
    unsigned this_list, corresp_pt;
    double x0, y0, x1, y1, x2, y2;

    for (this_list = 0; this_list < SPLINE_LIST_ARRAY_LENGTH(shape);
	this_list++)
    {
	unsigned this_spline;
	spline_type prev;

	spline_list_type list = SPLINE_LIST_ARRAY_ELT(shape, this_list);
	unsigned length = SPLINE_LIST_LENGTH(list);
	unsigned out_length = (list.open && length == 1 ? 2 : length);

	fprintf(er_file, "Shape = {\n");
	fprintf(er_file, "\t#Shape Number %d\n", this_list + 1);
	fprintf(er_file, "\tGroup = Default\n");
	fprintf(er_file, "\tType = Source\n");
	fprintf(er_file, "\tRoll = A\n");
	fprintf(er_file, "\tOpaque = True\n");
	fprintf(er_file, "\tLocked = False\n");
	fprintf(er_file, "\tWarp = True\n");
	fprintf(er_file, "\tCookieCut = True\n");
	fprintf(er_file, "\tColorCorrect = True\n");
	fprintf(er_file, "\tPrecision = 10\n");
	fprintf(er_file, "\tClosed = %s\n", (list.open ? "False" : "True"));
	fprintf(er_file, "\tTween = Linear\n");
	fprintf(er_file, "\tBPoints = %d\n", out_length);
	fprintf(er_file, "\tCPoints = %d\n", NUM_CORRESP_POINTS);
	fprintf(er_file, "\tFormKey = {\n");
	fprintf(er_file, "\t\tFrame = 1\n");
	fprintf(er_file, "\t\tPointList = {\n");

	prev = PREV_SPLINE_LIST_ELT(list, 0);
	if (list.open)
	    SPLINE_DEGREE(prev) = (polynomial_degree) -1;

	for (this_spline = 0; this_spline < length; this_spline++)
	{
	    spline_type s = SPLINE_LIST_ELT(list, this_spline);

	    if (SPLINE_DEGREE(prev) == -1)
		{ x0 = START_POINT(s).x; y0 = START_POINT(s).y; }
	    else if (SPLINE_DEGREE(prev) == CUBICTYPE)
		{ x0 = CONTROL2(prev).x; y0 = CONTROL2(prev).y; }
	    else /* if (SPLINE_DEGREE(prev) == LINEARTYPE) */
		{ x0 = START_POINT(s).x; y0 = START_POINT(s).y; }

	    x1 = START_POINT(s).x; y1 = START_POINT(s).y;

	    if (SPLINE_DEGREE(s) == CUBICTYPE)
		{ x2 = CONTROL1(s).x; y2 = CONTROL1(s).y; }
	    else
		{ x2 = START_POINT(s).x; y2 = START_POINT(s).y; }

	    fprintf(er_file, "\t\t\t(%f, %f), (%f, %f), (%f, %f),\n",
		x0 / width, y0 / height, x1 / width, y1 / height,
		x2 / width, y2 / height);

	    prev = s;
	}

	if (list.open && length == 1)
	{
	    x0 = CONTROL2(prev).x; y0 = CONTROL2(prev).y;
	    x2 = x1 = END_POINT(prev).x; y2 = y1 = END_POINT(prev).y;

	    fprintf(er_file, "\t\t\t(%f, %f), (%f, %f), (%f, %f),\n",
		x0 / width, y0 / height, x1 / width, y1 / height,
		x2 / width, y2 / height);
	}

	/* Close PointList and enclosing FormKey. */
	fprintf(er_file, "\t\t}\n\n\t}\n\n");
	fprintf(er_file, "\tCorrKey = {\n");
	fprintf(er_file, "\t\tFrame = 1\n");
	fprintf(er_file, "\t\tPointList = {\n");
	fprintf(er_file, "\t\t\t0");
	for (corresp_pt = 1; corresp_pt < NUM_CORRESP_POINTS; corresp_pt++)
	{
	    fprintf(er_file, ", %g",
		(out_length - 1.0) * corresp_pt / (NUM_CORRESP_POINTS - 1.0));
	}
	/* Close PointList and enclosing CorrKey. */
	fprintf(er_file, "\n\t\t}\n\n\t}\n\n");

	/* Close Shape. */
	fprintf(er_file, "}\n\n");
    }
}




int
output_er_writer(FILE* file, string name, int llx, int lly, int urx, int ury,
    spline_list_array_type shape)
{
    int result;
    unsigned width, height;

    result = output_er_header(file, name, llx, lly, urx, ury);
    if (result != 0) return result;

    width = urx - llx;
    height = ury - lly;
    out_splines(file, shape, width, height);

    return 0;
}




static string
now(void)
{
    string time_string;
    time_t t = time (0);

    XMALLOC (time_string, 26);  /* not 25 ! */
    strcpy (time_string, ctime (&t));
    time_string[24] = 0;  /* No newline. */

    return time_string;
}
