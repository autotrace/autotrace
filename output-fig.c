/* output-fig.c - output autotrace splines in FIG 3.2 format
   Copyright (C) 1999, 2000 Ian MacPhedran. */

#include "output-fig.h"
#include "xstd.h"

static real bezpnt(real, real, real, real, real);
static void out_fig_splines(FILE *, spline_list_array_type, int, int, int, int);
static int get_fig_colour(color_type);
static int fig_col_init();

/* colour information */
#define fig_col_hash(col_typ)  ( col_typ.r & 255 ) + ( col_typ.g & 161 ) + ( col_typ.b & 127 )

static struct {
        unsigned int colour;
        unsigned int alternate;
}       fig_hash[544];

static struct {
        unsigned char r,g,b;
        int alternate;
}       fig_colour_map[544];

static int LAST_FIG_COLOUR=32;
#define MAX_FIG_COLOUR 543

/* Bounding Box data and routines */
static float glob_min_x, glob_max_x, glob_min_y, glob_max_y;
static float loc_min_x, loc_max_x, loc_min_y, loc_max_y;
static int glo_bbox_flag=0,loc_bbox_flag=0,fig_depth;

static void fig_new_depth()
{
	if (glo_bbox_flag == 0) {
		glob_max_y = loc_max_y ; glob_min_y = loc_min_y ;
		glob_max_y = loc_max_y ; glob_min_y = loc_min_y ;
		glob_max_x = loc_max_x ; glob_min_x = loc_min_x ;
		glo_bbox_flag = 1;
	} else {
		if ((loc_max_y <= glob_min_y) ||
		(loc_min_y >= glob_max_y) ||
		(loc_max_x <= glob_min_x) ||
		(loc_min_x >= glob_max_x)) {
/* outside global bounds, increase global box */
		if (loc_max_y > glob_max_y) glob_max_y = loc_max_y ;
		if (loc_min_y < glob_min_y) glob_min_y = loc_min_y ;
		if (loc_max_x > glob_max_x) glob_max_x = loc_max_x ;
		if (loc_min_x < glob_min_x) glob_min_x = loc_min_x ;
	    } else {
/* inside global bounds, decrease depth and create new bounds */
		glob_max_y = loc_max_y ; glob_min_y = loc_min_y ;
		glob_max_x = loc_max_x ; glob_min_x = loc_min_x ;
		if (fig_depth) fig_depth--; // don't let it get < 0
	    }
	}
	loc_bbox_flag = 0;
}

static void fig_addtobbox(float x, float y)
{
	if (loc_bbox_flag == 0) {
	    loc_max_y = y ; loc_min_y = y ;
	    loc_max_x = x ; loc_min_x = x ;
	    loc_bbox_flag = 1;
	} else {
	    if (loc_max_y < y) loc_max_y = y ;
	    if (loc_min_y > y) loc_min_y = y ;
	    if (loc_max_x < x) loc_max_x = x ;
	    if (loc_min_x > x) loc_min_x = x ;
	}
}


/* Convert Bezier Spline */

static real bezpnt(real t, real z1, real z2, real z3, real z4)
{
	real temp, t1;
	/* Determine ordinate on Bezier curve at length "t" on curve */
	if (t < (real) 0.0) { t = (real) 0.0; }
	if (t > (real) 1.0) { t = (real) 1.0; }
	t1 = ((real) 1.0 - t);
	temp = t1*t1*t1*z1 + (real)3.0*t*t1*t1*z2 + (real)3.0*t*t*t1*z3 + t*t*t*z4;
	return(temp);
}

static void out_fig_splines(FILE * file, spline_list_array_type shape,
		int llx, int lly, int urx, int ury)
{
    unsigned this_list;
/*    int fig_colour, fig_depth, i; */
    int fig_colour, fig_fill, fig_width, i;
    int *spline_colours;

/*
	add an array of colours for splines (one for each group)
	create palette hash
*/

/*	Need to create hash table for colours	*/
    spline_colours=(int *)malloc(sizeof(int)*SPLINE_LIST_ARRAY_LENGTH(shape));
    if (spline_colours == NULL) {
	FATAL1("OutputFig: can't get array for %d colours\n",SPLINE_LIST_ARRAY_LENGTH(shape));
    }
    /* Preload the big 8 */
    fig_col_init();

    /*	Load the colours from the splines	*/
    for (this_list = 0; this_list < SPLINE_LIST_ARRAY_LENGTH (shape);
	  this_list++)
    {
	spline_list_type list = SPLINE_LIST_ARRAY_ELT (shape, this_list);
	spline_colours[this_list] = get_fig_colour(list.color);
    }
    /* Output colours */
    if (LAST_FIG_COLOUR > 32) {
	for (i=32; i<LAST_FIG_COLOUR; i++) {
	    fprintf(file,"0 %d #%.2x%.2x%.2x\n",i,fig_colour_map[i].r,
		fig_colour_map[i].g,fig_colour_map[i].b);
	}
    }
/*	Each "spline list" in the array appears to be a group of splines */
    fig_depth = SPLINE_LIST_ARRAY_LENGTH (shape) + 20;
    if (fig_depth > 999) { fig_depth = 999; }

    for (this_list = 0; this_list < SPLINE_LIST_ARRAY_LENGTH (shape);
	  this_list++)
    {
	unsigned this_spline;
	spline_list_type list = SPLINE_LIST_ARRAY_ELT (shape, this_list);

/*	store the spline points in two arrays, control weights in another */
	int *pointx, *pointy;
	real *contrl;
        int pointcount=0,is_spline=0,i,j;
        int maxlength=SPLINE_LIST_LENGTH (list) * 5 + 1;

	XMALLOC (pointx, maxlength * sizeof (int));
	XMALLOC (pointy, maxlength * sizeof (int));
	XMALLOC (contrl, maxlength * sizeof (real));

	if (list.clockwise) { fig_colour = FIG_WHITE; }
	    else { fig_colour = spline_colours[this_list]; }

	for (this_spline = 0; this_spline < SPLINE_LIST_LENGTH (list);
	     this_spline++)
	{
	    spline_type s = SPLINE_LIST_ELT (list, this_spline);

	    if (pointcount == 0) {
		pointx[pointcount] = FIG_X(START_POINT(s).x);
		pointy[pointcount] = FIG_Y(START_POINT(s).y);
		contrl[pointcount] = (real) 0.0;
		fig_addtobbox(START_POINT(s).x,START_POINT(s).y);
		pointcount++;
	    }
	/* Apparently START_POINT for one spline section is same as END_POINT
	   for previous section - should really test for this */
	    if (SPLINE_DEGREE(s) == LINEARTYPE)
	    {
		pointx[pointcount] = FIG_X(END_POINT(s).x);
		pointy[pointcount] = FIG_Y(END_POINT(s).y);
		contrl[pointcount] = (real) 0.0;
		fig_addtobbox(START_POINT(s).x,START_POINT(s).y);
		pointcount++;
	    }
	    else /* Assume Bezier like spline */
	    {
		/* Convert approximated bezier to interpolated X Spline */
		real temp;
		for (temp = (real) 0.2; temp < (real) 0.9; temp += (real) 0.2) {
		    pointx[pointcount] =
			FIG_X(bezpnt(temp,START_POINT(s).x,CONTROL1(s).x,
			CONTROL2(s).x,END_POINT(s).x));
		    pointy[pointcount] =
			FIG_Y(bezpnt(temp,START_POINT(s).y,CONTROL1(s).y,
			CONTROL2(s).y,END_POINT(s).y));
		    contrl[pointcount] = (real) -1.0;
		    pointcount++;
		}
		pointx[pointcount] = FIG_X(END_POINT(s).x);
		pointy[pointcount] = FIG_Y(END_POINT(s).y);
		contrl[pointcount] = (real) 0.0;
		fig_addtobbox(START_POINT(s).x,START_POINT(s).y);
		fig_addtobbox(CONTROL1(s).x,CONTROL1(s).y);
		fig_addtobbox(CONTROL2(s).x,CONTROL2(s).y);
		fig_addtobbox(END_POINT(s).x,END_POINT(s).y);
		pointcount++;
		is_spline=1;
	    }
        }
	if (shape.centerline) {
	    fig_fill = -1; fig_width = 1;
	} else {
	    /* Use zero width lines - unit width is too thick */
	    fig_fill = 20; fig_width = 0;
	}
	if (is_spline != 0) {
	    fig_new_depth();
	    fprintf(file,"3 5 0 %d %d %d %d 0 %d 0.00 0 0 0 %d\n",
	    fig_width, fig_colour, fig_colour, fig_depth, fig_fill, pointcount);
	    /* Print out points */
	    j = 0;
	    for (i=0; i<pointcount; i++) {
		j++; if (j == 1) {fprintf(file,"\t");}
		fprintf(file,"%d %d ",pointx[i],pointy[i]);
		if (j == 8) {fprintf(file,"\n"); j=0;}
	    }
	    if (j != 0) {fprintf(file,"\n");}
	    j = 0;
	    /* Print out control weights */
	    for (i=0; i<pointcount; i++) {
		j++; if (j == 1) {fprintf(file,"\t");}
		fprintf(file,"%f ",contrl[i]);
		if (j == 8) {fprintf(file,"\n"); j=0;}
	    }
	    if (j != 0) {fprintf(file,"\n");}
	} else {
	    /* Polygons can be handled better as polygons */
	    if (pointcount == 2) {
		if ((pointx[0] == pointx[1]) && (pointy[0] == pointy[1])) {
		    /* Point */
		    fig_new_depth();
		    fprintf(file,"2 1 0 1 %d %d %d 0 -1 0.000 0 0 -1 0 0 1\n",
			fig_colour, fig_colour, fig_depth);
		    fprintf(file,"\t%d %d\n",pointx[0],pointy[0]);
		} else {
		    /* Line segment? */
		    fig_new_depth();
		    fprintf(file,"2 1 0 1 %d %d %d 0 -1 0.000 0 0 -1 0 0 2\n",
			fig_colour, fig_colour, fig_depth);
		    fprintf(file,"\t%d %d %d %d\n",pointx[0],pointy[0],
			pointx[1],pointy[1]);
		}
	    } else {
		if ((pointcount == 3) && (pointx[0] == pointx[2])
		   && (pointy[0] == pointy[2])){
		    /* Line segment? */
		    fig_new_depth();
		    fprintf(file,"2 1 0 1 %d %d %d 0 -1 0.000 0 0 -1 0 0 2\n",
			fig_colour, fig_colour, fig_depth);
		    fprintf(file,"\t%d %d %d %d\n",pointx[0],pointy[0],
			pointx[1],pointy[1]);
		} else {
		if ((pointx[0] != pointx[pointcount-1]) ||
		 (pointy[0] != pointy[pointcount-1])) {
		/* Need to have last point same as first for polygon */
		    pointx[pointcount] = pointx[0];
		    pointy[pointcount] = pointy[0];
		    pointcount++;
		}
		fig_new_depth();
		fprintf(file,"2 3 0 %d %d %d %d 0 %d 0.00 0 0 0 0 0 %d\n",
		fig_width, fig_colour, fig_colour, fig_depth, fig_fill, pointcount);
		/* Print out points */
		j = 0;
		for (i=0; i<pointcount; i++) {
		    j++; if (j == 1) {fprintf(file,"\t");}
		    fprintf(file,"%d %d ",pointx[i],pointy[i]);
		    if (j == 8) {fprintf(file,"\n"); j=0;}
		}
		if (j != 0) {fprintf(file,"\n");}
		}
	    }
	}
/*	fig_depth--; */
	if (fig_depth < 0) { fig_depth=0; }
	free (pointx);
	free (pointy);
	free (contrl);
    }
    free(spline_colours);
    return;
}

int output_fig_writer(FILE* file, string name,
		     int llx, int lly, int urx, int ury,
		     spline_list_array_type shape)
{
/*	Output header	*/
    fprintf(file,"#FIG 3.2\nLandscape\nCenter\nInches\nLetter\n100.00\nSingle\n-2\n1200 2\n");

/*	Output data	*/
    out_fig_splines(file, shape, llx, lly, urx, ury);
    return 0;
}

/*
	Create hash number
	At hash number -> set fig number
	If fig number already used, go to next fig number (alternate)
	if alternate is 0, set next unused fig number
*/

static int fig_col_init()
{
    int i;

    for (i=0;i<544;i++) { fig_hash[i].colour = 0; fig_colour_map[i].alternate = 0; }

    /*	populate the first 8 primary colours	*/
    /* Black */
    fig_hash[0].colour = FIG_BLACK;
    fig_colour_map[FIG_BLACK].r = 0;
    fig_colour_map[FIG_BLACK].g = 0;
    fig_colour_map[FIG_BLACK].b = 0;
    /* White */
    fig_hash[543].colour = FIG_WHITE;
    fig_colour_map[FIG_WHITE].r = 255;
    fig_colour_map[FIG_WHITE].g = 255;
    fig_colour_map[FIG_WHITE].b = 255;
    /* Red */
    fig_hash[255].colour = FIG_RED;
    fig_colour_map[FIG_RED].r = 255;
    fig_colour_map[FIG_RED].g = 0;
    fig_colour_map[FIG_RED].b = 0;
    /* Green */
    fig_hash[161].colour = FIG_GREEN;
    fig_colour_map[FIG_GREEN].r = 0;
    fig_colour_map[FIG_GREEN].g = 255;
    fig_colour_map[FIG_GREEN].b = 0;
    /* Blue */
    fig_hash[127].colour = FIG_BLUE;
    fig_colour_map[FIG_BLUE].r = 0;
    fig_colour_map[FIG_BLUE].g = 0;
    fig_colour_map[FIG_BLUE].b = 255;
    /* Cyan */
    fig_hash[198].colour = FIG_CYAN;
    fig_colour_map[FIG_CYAN].r = 0;
    fig_colour_map[FIG_CYAN].g = 255;
    fig_colour_map[FIG_CYAN].b = 255;
    /* Magenta */
    fig_hash[382].colour = FIG_MAGENTA;
    fig_colour_map[FIG_MAGENTA].r = 255;
    fig_colour_map[FIG_MAGENTA].g = 0;
    fig_colour_map[FIG_MAGENTA].b = 255;
    /* Yellow */
    fig_hash[416].colour = FIG_YELLOW;
    fig_colour_map[FIG_YELLOW].r = 255;
    fig_colour_map[FIG_YELLOW].g = 255;
    fig_colour_map[FIG_YELLOW].b = 0;

    return(0);
}

/*
 * Return the FIG colour index based on the RGB triplet.
 * If unknown, create a new colour index and return that.
 */

int get_fig_colour(color_type this_colour)
{
    int hash,i,this_ind;

    hash = fig_col_hash(this_colour);

/*  Special case: black _IS_ zero: */
    if ((hash == 0) & (COLOR_EQUAL(fig_colour_map[0],this_colour)))
	{return(0);}

    if (fig_hash[hash].colour == 0) {
	fig_hash[hash].colour = LAST_FIG_COLOUR;
	fig_colour_map[LAST_FIG_COLOUR].r = this_colour.r;
	fig_colour_map[LAST_FIG_COLOUR].g = this_colour.g;
	fig_colour_map[LAST_FIG_COLOUR].b = this_colour.b;
	LAST_FIG_COLOUR++;
	if (LAST_FIG_COLOUR >= MAX_FIG_COLOUR) {
		FATAL1("Output-Fig: too many colours: %d",LAST_FIG_COLOUR);
	}
	return(fig_hash[hash].colour);
    } else {
	i=0;
	this_ind = fig_hash[hash].colour;
figcolloop:
	/* If colour match return current colour */
	if (COLOR_EQUAL(fig_colour_map[this_ind],this_colour)) {
		return(this_ind);
	}
	/* If next colour zero - set it, return */
	if (fig_colour_map[this_ind].alternate == 0) {
	    fig_colour_map[this_ind].alternate = LAST_FIG_COLOUR;
	    fig_colour_map[LAST_FIG_COLOUR].r = this_colour.r;
	    fig_colour_map[LAST_FIG_COLOUR].g = this_colour.g;
	    fig_colour_map[LAST_FIG_COLOUR].b = this_colour.b;
	    LAST_FIG_COLOUR++;
	    if (LAST_FIG_COLOUR >= MAX_FIG_COLOUR) {
		FATAL1("Output-Fig: too many colours: %d",LAST_FIG_COLOUR);
	    }
	    return(fig_colour_map[this_ind].alternate);
	}
	/* Else get next colour */
	this_ind = fig_colour_map[this_ind].alternate;
        /* Sanity check ... if colour too big - abort */
	if (i++ > MAX_FIG_COLOUR) {
	    FATAL1("Output-Fig: too many colours (loop): %d",i);
	}
	/* Else loop top */
	goto figcolloop;
    }
}

/* version 0.27 */
