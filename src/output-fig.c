/* output-fig.c - output autotrace splines in FIG 3.2 format

   Copyright (C) 1999, 2000, 2001 Ian MacPhedran

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public License
   as published by the Free Software Foundation; either version 2.1 of
   the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with this library; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
   USA. */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* Def: HAVE_CONFIG_H */

#include "output-fig.h"
#include "xstd.h"
#include "logreport.h"
#include "color.h"
#include "spline.h"

/* use FIG_X and FIG_Y to convert from local units (pixels) to FIG ones */
/* assume 1 pixel is equal to 1/80 inches (old FIG unit) */
/* Offset by 300 units (1/4 inch) */

#define FIG_X(x) (int)((x * 15.0) + 300.0)
#define FIG_Y(y) (int)(((ury - y) * 15.0) + 300.0)

/* the basic colours */
#define FIG_BLACK	0
#define FIG_BLUE	1
#define FIG_GREEN	2
#define FIG_CYAN	3
#define FIG_RED		4
#define FIG_MAGENTA	5
#define FIG_YELLOW	6
#define FIG_WHITE	7

static gfloat bezpnt(gfloat, gfloat, gfloat, gfloat, gfloat);
static void out_fig_splines(FILE *, spline_list_array_type, int, int, int, int, at_exception_type *);
static int get_fig_colour(at_color, at_exception_type *);
static void fig_col_init(void);

/* colour information */
#define fig_col_hash(col_typ)  ( ( (col_typ).r & 255 ) + ( (col_typ).g & 161 ) + ( (col_typ).b & 127 ) )

static struct {
  unsigned int colour;
  unsigned int alternate;
} fig_hash[544];

static struct {
  at_color c;
  int alternate;
} fig_colour_map[544];

static int LAST_FIG_COLOUR = 32;
#define MAX_FIG_COLOUR 543

/* Bounding Box data and routines */
static float glob_min_x, glob_max_x, glob_min_y, glob_max_y;
static float loc_min_x, loc_max_x, loc_min_y, loc_max_y;
static int glo_bbox_flag = 0, loc_bbox_flag = 0, fig_depth;

static void fig_new_depth()
{
  if (glo_bbox_flag == 0) {
    glob_max_y = loc_max_y;
    glob_min_y = loc_min_y;
    glob_max_x = loc_max_x;
    glob_min_x = loc_min_x;
    glo_bbox_flag = 1;
  } else {
    if ((loc_max_y <= glob_min_y) || (loc_min_y >= glob_max_y) || (loc_max_x <= glob_min_x) || (loc_min_x >= glob_max_x)) {
/* outside global bounds, increase global box */
      if (loc_max_y > glob_max_y)
        glob_max_y = loc_max_y;
      if (loc_min_y < glob_min_y)
        glob_min_y = loc_min_y;
      if (loc_max_x > glob_max_x)
        glob_max_x = loc_max_x;
      if (loc_min_x < glob_min_x)
        glob_min_x = loc_min_x;
    } else {
/* inside global bounds, decrease depth and create new bounds */
      glob_max_y = loc_max_y;
      glob_min_y = loc_min_y;
      glob_max_x = loc_max_x;
      glob_min_x = loc_min_x;
      if (fig_depth)
        fig_depth--;            /* don't let it get < 0 */
    }
  }
  loc_bbox_flag = 0;
}

static void fig_addtobbox(float x, float y)
{
  if (loc_bbox_flag == 0) {
    loc_max_y = y;
    loc_min_y = y;
    loc_max_x = x;
    loc_min_x = x;
    loc_bbox_flag = 1;
  } else {
    if (loc_max_y < y)
      loc_max_y = y;
    if (loc_min_y > y)
      loc_min_y = y;
    if (loc_max_x < x)
      loc_max_x = x;
    if (loc_min_x > x)
      loc_min_x = x;
  }
}

/* Convert Bezier Spline */

static gfloat bezpnt(gfloat t, gfloat z1, gfloat z2, gfloat z3, gfloat z4)
{
  gfloat temp, t1;
  /* Determine ordinate on Bezier curve at length "t" on curve */
  if (t < (gfloat) 0.0) {
    t = (gfloat) 0.0;
  }
  if (t > (gfloat) 1.0) {
    t = (gfloat) 1.0;
  }
  t1 = ((gfloat) 1.0 - t);
  temp = t1 * t1 * t1 * z1 + (gfloat) 3.0 *t * t1 * t1 * z2 + (gfloat) 3.0 *t * t * t1 * z3 + t * t * t * z4;
  return (temp);
}

static void out_fig_splines(FILE * file, spline_list_array_type shape, int llx, int lly, int urx, int ury, at_exception_type * exp)
{
  unsigned this_list;
/*    int fig_colour, fig_depth, i; */
  int fig_colour, fig_fill, fig_width, fig_subt, fig_spline_close, i;
  int *spline_colours;

/*
	add an array of colours for splines (one for each group)
	create palette hash
*/

  /*  Need to create hash table for colours */
  XMALLOC(spline_colours, (sizeof(int) * SPLINE_LIST_ARRAY_LENGTH(shape)));

  /* Preload the big 8 */
  fig_col_init();

  /*  Load the colours from the splines */
  for (this_list = 0; this_list < SPLINE_LIST_ARRAY_LENGTH(shape); this_list++) {
    spline_list_type list = SPLINE_LIST_ARRAY_ELT(shape, this_list);
    at_color curr_color = (list.clockwise && shape.background_color != NULL) ? *(shape.background_color) : list.color;
    spline_colours[this_list] = get_fig_colour(curr_color, exp);
  }
  /* Output colours */
  if (LAST_FIG_COLOUR > 32) {
    for (i = 32; i < LAST_FIG_COLOUR; i++) {
      fprintf(file, "0 %d #%.2x%.2x%.2x\n", i, fig_colour_map[i].c.r, fig_colour_map[i].c.g, fig_colour_map[i].c.b);
    }
  }
/*	Each "spline list" in the array appears to be a group of splines */
  fig_depth = SPLINE_LIST_ARRAY_LENGTH(shape) + 20;
  if (fig_depth > 999) {
    fig_depth = 999;
  }

  for (this_list = 0; this_list < SPLINE_LIST_ARRAY_LENGTH(shape); this_list++) {
    unsigned this_spline;
    spline_list_type list = SPLINE_LIST_ARRAY_ELT(shape, this_list);

/*	store the spline points in two arrays, control weights in another */
    int *pointx, *pointy;
    gfloat *contrl;
    int pointcount = 0, is_spline = 0, j;
    int maxlength = SPLINE_LIST_LENGTH(list) * 5 + 1;

    XMALLOC(pointx, maxlength * sizeof(int));
    XMALLOC(pointy, maxlength * sizeof(int));
    XMALLOC(contrl, maxlength * sizeof(gfloat));

    if (list.clockwise) {
      fig_colour = FIG_WHITE;
    } else {
      fig_colour = spline_colours[this_list];
    }

    fig_spline_close = 5;

    for (this_spline = 0; this_spline < SPLINE_LIST_LENGTH(list); this_spline++) {
      spline_type s = SPLINE_LIST_ELT(list, this_spline);

      if (pointcount == 0) {
        pointx[pointcount] = FIG_X(START_POINT(s).x);
        pointy[pointcount] = FIG_Y(START_POINT(s).y);
        contrl[pointcount] = (gfloat) 0.0;
        fig_addtobbox(START_POINT(s).x, START_POINT(s).y);
        pointcount++;
      }
      /* Apparently START_POINT for one spline section is same as END_POINT
         for previous section - should gfloatly test for this */
      if (SPLINE_DEGREE(s) == LINEARTYPE) {
        pointx[pointcount] = FIG_X(END_POINT(s).x);
        pointy[pointcount] = FIG_Y(END_POINT(s).y);
        contrl[pointcount] = (gfloat) 0.0;
        fig_addtobbox(START_POINT(s).x, START_POINT(s).y);
        pointcount++;
      } else {                  /* Assume Bezier like spline */

        /* Convert approximated bezier to interpolated X Spline */
        gfloat temp;
        for (temp = (gfloat) 0.2; temp < (gfloat) 0.9; temp += (gfloat) 0.2) {
          pointx[pointcount] = FIG_X(bezpnt(temp, START_POINT(s).x, CONTROL1(s).x, CONTROL2(s).x, END_POINT(s).x));
          pointy[pointcount] = FIG_Y(bezpnt(temp, START_POINT(s).y, CONTROL1(s).y, CONTROL2(s).y, END_POINT(s).y));
          contrl[pointcount] = (gfloat) - 1.0;
          pointcount++;
        }
        pointx[pointcount] = FIG_X(END_POINT(s).x);
        pointy[pointcount] = FIG_Y(END_POINT(s).y);
        contrl[pointcount] = (gfloat) 0.0;
        fig_addtobbox(START_POINT(s).x, START_POINT(s).y);
        fig_addtobbox(CONTROL1(s).x, CONTROL1(s).y);
        fig_addtobbox(CONTROL2(s).x, CONTROL2(s).y);
        fig_addtobbox(END_POINT(s).x, END_POINT(s).y);
        pointcount++;
        is_spline = 1;
      }
    }
    if (shape.centerline) {
      fig_fill = -1;
      fig_width = 1;
      fig_spline_close = 4;
    } else {
      /* Use zero width lines - unit width is too thick */
      fig_fill = 20;
      fig_width = 0;
      fig_spline_close = 5;
    }
    if (is_spline != 0) {
      fig_new_depth();
      fprintf(file, "3 %d 0 %d %d %d %d 0 %d 0.00 0 0 0 %d\n", fig_spline_close, fig_width, fig_colour, fig_colour, fig_depth, fig_fill, pointcount);
      /* Print out points */
      j = 0;
      for (i = 0; i < pointcount; i++) {
        j++;
        if (j == 1) {
          fprintf(file, "\t");
        }
        fprintf(file, "%d %d ", pointx[i], pointy[i]);
        if (j == 8) {
          fprintf(file, "\n");
          j = 0;
        }
      }
      if (j != 0) {
        fprintf(file, "\n");
      }
      j = 0;
      /* Print out control weights */
      for (i = 0; i < pointcount; i++) {
        j++;
        if (j == 1) {
          fprintf(file, "\t");
        }
        fprintf(file, "%f ", contrl[i]);
        if (j == 8) {
          fprintf(file, "\n");
          j = 0;
        }
      }
      if (j != 0) {
        fprintf(file, "\n");
      }
    } else {
      /* Polygons can be handled better as polygons */
      fig_subt = 3;
      if (pointcount == 2) {
        if ((pointx[0] == pointx[1]) && (pointy[0] == pointy[1])) {
          /* Point */
          fig_new_depth();
          fprintf(file, "2 1 0 1 %d %d %d 0 -1 0.000 0 0 -1 0 0 1\n", fig_colour, fig_colour, fig_depth);
          fprintf(file, "\t%d %d\n", pointx[0], pointy[0]);
        } else {
          /* Line segment? */
          fig_new_depth();
          fprintf(file, "2 1 0 1 %d %d %d 0 -1 0.000 0 0 -1 0 0 2\n", fig_colour, fig_colour, fig_depth);
          fprintf(file, "\t%d %d %d %d\n", pointx[0], pointy[0], pointx[1], pointy[1]);
        }
      } else {
        if ((pointcount == 3) && (pointx[0] == pointx[2])
            && (pointy[0] == pointy[2])) {
          /* Line segment? */
          fig_new_depth();
          fprintf(file, "2 1 0 1 %d %d %d 0 -1 0.000 0 0 -1 0 0 2\n", fig_colour, fig_colour, fig_depth);
          fprintf(file, "\t%d %d %d %d\n", pointx[0], pointy[0], pointx[1], pointy[1]);
        } else {
          if ((pointx[0] != pointx[pointcount - 1]) || (pointy[0] != pointy[pointcount - 1])) {
            if (shape.centerline) {
              fig_subt = 1;
            } else {
              /* Need to have last point same as first for polygon */
              pointx[pointcount] = pointx[0];
              pointy[pointcount] = pointy[0];
              pointcount++;
            }
          }
          fig_new_depth();
          fprintf(file, "2 %d 0 %d %d %d %d 0 %d 0.00 0 0 0 0 0 %d\n", fig_subt, fig_width, fig_colour, fig_colour, fig_depth, fig_fill, pointcount);
          /* Print out points */
          j = 0;
          for (i = 0; i < pointcount; i++) {
            j++;
            if (j == 1) {
              fprintf(file, "\t");
            }
            fprintf(file, "%d %d ", pointx[i], pointy[i]);
            if (j == 8) {
              fprintf(file, "\n");
              j = 0;
            }
          }
          if (j != 0) {
            fprintf(file, "\n");
          }
        }
      }
    }
/*	fig_depth--; */
    if (fig_depth < 0) {
      fig_depth = 0;
    }
    free(pointx);
    free(pointy);
    free(contrl);
  }
  free(spline_colours);
  return;
}

int output_fig_writer(FILE * file, gchar * name, int llx, int lly, int urx, int ury, at_output_opts_type * opts, spline_list_array_type shape, at_msg_func msg_func, gpointer msg_data, gpointer user_data)
{
  at_exception_type exp = at_exception_new(msg_func, msg_data);
/*	Output header	*/
  fprintf(file, "#FIG 3.2\nLandscape\nCenter\nInches\nLetter\n100.00\nSingle\n-2\n1200 2\n");

/*	Output data	*/
  out_fig_splines(file, shape, llx, lly, urx, ury, &exp);
  return 0;
}

/*
	Create hash number
	At hash number -> set fig number
	If fig number already used, go to next fig number (alternate)
	if alternate is 0, set next unused fig number
*/

static void fig_col_init(void)
{
  int i;

  for (i = 0; i < 544; i++) {
    fig_hash[i].colour = 0;
    fig_colour_map[i].alternate = 0;
  }

  /*  populate the first 8 primary colours  */
  /* Black */
  fig_hash[0].colour = FIG_BLACK;
  fig_colour_map[FIG_BLACK].c.r = 0;
  fig_colour_map[FIG_BLACK].c.g = 0;
  fig_colour_map[FIG_BLACK].c.b = 0;
  /* White */
  fig_hash[543].colour = FIG_WHITE;
  fig_colour_map[FIG_WHITE].c.r = 255;
  fig_colour_map[FIG_WHITE].c.g = 255;
  fig_colour_map[FIG_WHITE].c.b = 255;
  /* Red */
  fig_hash[255].colour = FIG_RED;
  fig_colour_map[FIG_RED].c.r = 255;
  fig_colour_map[FIG_RED].c.g = 0;
  fig_colour_map[FIG_RED].c.b = 0;
  /* Green */
  fig_hash[161].colour = FIG_GREEN;
  fig_colour_map[FIG_GREEN].c.r = 0;
  fig_colour_map[FIG_GREEN].c.g = 255;
  fig_colour_map[FIG_GREEN].c.b = 0;
  /* Blue */
  fig_hash[127].colour = FIG_BLUE;
  fig_colour_map[FIG_BLUE].c.r = 0;
  fig_colour_map[FIG_BLUE].c.g = 0;
  fig_colour_map[FIG_BLUE].c.b = 255;
  /* Cyan */
  fig_hash[198].colour = FIG_CYAN;
  fig_colour_map[FIG_CYAN].c.r = 0;
  fig_colour_map[FIG_CYAN].c.g = 255;
  fig_colour_map[FIG_CYAN].c.b = 255;
  /* Magenta */
  fig_hash[382].colour = FIG_MAGENTA;
  fig_colour_map[FIG_MAGENTA].c.r = 255;
  fig_colour_map[FIG_MAGENTA].c.g = 0;
  fig_colour_map[FIG_MAGENTA].c.b = 255;
  /* Yellow */
  fig_hash[416].colour = FIG_YELLOW;
  fig_colour_map[FIG_YELLOW].c.r = 255;
  fig_colour_map[FIG_YELLOW].c.g = 255;
  fig_colour_map[FIG_YELLOW].c.b = 0;
}

/*
 * Return the FIG colour index based on the RGB triplet.
 * If unknown, create a new colour index and return that.
 */

static int get_fig_colour(at_color this_colour, at_exception_type * exp)
{
  int hash, i, this_ind;

  hash = fig_col_hash(this_colour);

/*  Special case: black _IS_ zero: */
  if ((hash == 0) && (at_color_equal(&(fig_colour_map[0].c), &this_colour))) {
    return (0);
  }

  if (fig_hash[hash].colour == 0) {
    fig_hash[hash].colour = LAST_FIG_COLOUR;
    fig_colour_map[LAST_FIG_COLOUR].c.r = this_colour.r;
    fig_colour_map[LAST_FIG_COLOUR].c.g = this_colour.g;
    fig_colour_map[LAST_FIG_COLOUR].c.b = this_colour.b;
    LAST_FIG_COLOUR++;
    if (LAST_FIG_COLOUR >= MAX_FIG_COLOUR) {
      LOG("Output-Fig: too many colours: %d", LAST_FIG_COLOUR);
      at_exception_fatal(exp, "Output-Fig: too many colours");
      return 0;
    }
    return (fig_hash[hash].colour);
  } else {
    i = 0;
    this_ind = fig_hash[hash].colour;
figcolloop:
    /* If colour match return current colour */
    if (at_color_equal(&(fig_colour_map[this_ind].c), &this_colour)) {
      return (this_ind);
    }
    /* If next colour zero - set it, return */
    if (fig_colour_map[this_ind].alternate == 0) {
      fig_colour_map[this_ind].alternate = LAST_FIG_COLOUR;
      fig_colour_map[LAST_FIG_COLOUR].c.r = this_colour.r;
      fig_colour_map[LAST_FIG_COLOUR].c.g = this_colour.g;
      fig_colour_map[LAST_FIG_COLOUR].c.b = this_colour.b;
      LAST_FIG_COLOUR++;
      if (LAST_FIG_COLOUR >= MAX_FIG_COLOUR) {
        LOG("Output-Fig: too many colours: %d", LAST_FIG_COLOUR);
        at_exception_fatal(exp, "Output-Fig: too many colours");
        return 0;
      }
      return (fig_colour_map[this_ind].alternate);
    }
    /* Else get next colour */
    this_ind = fig_colour_map[this_ind].alternate;
    /* Sanity check ... if colour too big - abort */
    if (i++ > MAX_FIG_COLOUR) {
      LOG("Output-Fig: too many colours (loop): %d", i);
      at_exception_fatal(exp, "Output-Fig: too many colours (loop)");
      return 0;
    }
    /* Else loop top */
    goto figcolloop;
  }
}
