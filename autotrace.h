/* autotrace.h
  Copyright (C) 2000, 2001 Martin Weber

  The author can be contacted at <martweb@gmx.net>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA. */

#ifndef AUTOTRACE_H
#define AUTOTRACE_H

#include "types.h"
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/*
 * Typedefs
 */
typedef struct _at_fitting_opts_type at_fitting_opts_type;
typedef struct _at_bitmap_type at_bitmap_type;
typedef struct _at_color_type at_color_type;
typedef enum _at_polynomial_degree at_polynomial_degree;
typedef struct _at_spline_type at_spline_type;
typedef struct _at_spline_list_type at_spline_list_type;
typedef struct _at_spline_list_array_type at_spline_list_array_type;
#define at_splines_type at_spline_list_array_type 

/* Color in RGB */
struct _at_color_type
{
  unsigned char r;
  unsigned char g;
  unsigned char b;
};

/* Third degree is the highest we deal with.  */
enum _at_polynomial_degree
{
  AT_LINEARTYPE = 1, 
  AT_QUADRATICTYPE = 2, 
  AT_CUBICTYPE = 3, 
  AT_PARALLELELLIPSETYPE = 4,
  AT_ELLIPSETYPE = 5, 
  AT_CIRCLETYPE = 6 
  /* not the real number of points to define a
     circle but to distinguish between a cubic spline */
};

/* A Bezier spline can be represented as four points in the real plane:
   a starting point, ending point, and two control points.  The
   curve always lies in the convex hull defined by the four points.  It
   is also convenient to save the divergence of the spline from the
   straight line defined by the endpoints.  */
struct _at_spline_type
{
  at_real_coordinate_type v[4];	/* The control points.  */
  at_polynomial_degree degree;
  at_real linearity;
};

/* Each outline in a character is typically represented by many
   splines.  So, here is a list structure for that:  */
struct _at_spline_list_type
{
  at_spline_type *data;
  unsigned length;
  at_bool clockwise;
  at_color_type color;
  at_bool open;
};

/* Each character is in general made up of many outlines. So here is one
   more list structure.  */
struct _at_spline_list_array_type
{
  at_spline_list_type *data;
  unsigned length;
  /* Whether to trace a character's centerline or its outline  */
  at_bool centerline;
};

struct _at_fitting_opts_type
{
/* Background color, the color of the background that should be ignored */
  at_color_type *bgColor;

/* To how many colors the bitmap is reduced */
  unsigned color_count;

/* If the angle defined by a point and its predecessors and successors
   is smaller than this, it's a corner, even if it's within
   `corner_surround' pixels of a point with a smaller angle.
   (-corner-always-threshold)  */
  at_real corner_always_threshold;

/* Number of points to consider when determining if a point is a corner
   or not.  (-corner-surround)  */
  unsigned corner_surround;

/* If a point, its predecessors, and its successors define an angle
    smaller than this, it's a corner.  Should be in range 0..180.
   (-corner-threshold)  */
  at_real corner_threshold;

/* Amount of error at which a fitted spline is unacceptable.  If any
   pixel is further away than this from the fitted curve, we try again.
   (-error-threshold) */
  at_real error_threshold;

/* Number of times to smooth original data points.  Increasing this
   number dramatically---to 50 or so---can produce vastly better
   results.  But if any points that ``should'' be corners aren't found,
   the curve goes to hell around that point.  (-filter-iterations)  */
  unsigned filter_iteration_count;

/* To produce the new point, use the old point plus this times the
   neighbors.  (-filter-percent)  */
  at_real filter_percent;

/* If a spline is closer to a straight line than this, it remains a
   straight line, even if it would otherwise be changed back to a curve.
   This is weighted by the square of the curve length, to make shorter
   curves more likely to be reverted.  (-line-reversion-threshold)  */
  at_real line_reversion_threshold;

/* How many pixels (on the average) a spline can diverge from the line
   determined by its endpoints before it is changed to a straight line.
   (-line-threshold) */
  at_real line_threshold;

/* Should adjacent corners be removed?  */
  at_bool remove_adj_corners;

/* Number of points to look at on either side of a point when computing
   the approximation to the tangent at that point.  (-tangent-surround)  */
  unsigned tangent_surround;

/* Thin all the lines in the image prior to fitting. */
  at_bool thin;

/* Despeckle level */
  int despeckle_level;

/* Despeckle tightness */
  at_real despeckle_tightness;

/* Whether to trace a character's centerline or its outline  */
  at_bool centerline;
};

struct _at_bitmap_type
{
  unsigned height;
  unsigned width;
  unsigned char *bitmap;
  unsigned int np;
};

/*
 * IO Handler typedefs
 */
typedef 
at_bitmap_type (*at_input_read_func)   (at_string name);

typedef 
int            (*at_output_write_func) (FILE*, at_string name,
					int llx, int lly, 
					int urx, int ury,
					int dpi,
					at_splines_type shape);

/*
 * Progress handler typedefs
 * 0.0 <= percentage <= 1.0
 */
typedef void           (* at_progress_func)    (at_real percentage,
						at_address client_data);

/*
 * Test cancel
 * Return true if auto-tracing should be stopped.
 */
typedef at_bool          (*  at_testcancel_func) (at_address client_data);

/*
 * Functions
 */

/* Option related */
at_fitting_opts_type * at_fitting_opts_new(void);
at_fitting_opts_type * at_fitting_opts_copy (at_fitting_opts_type * original); 
void at_fitting_opts_free(at_fitting_opts_type * opts);
/* TODO internal data access, copy */

/* Bitmap related */
at_bitmap_type * at_bitmap_new (at_input_read_func input_reader,
				at_string filename);
unsigned short at_bitmap_get_width (at_bitmap_type * bitmap);
unsigned short at_bitmap_get_height (at_bitmap_type * bitmap);
void at_bitmap_free (at_bitmap_type * bitmap);
/* TODO internal data access */

/* Input related */
at_input_read_func at_input_get_handler (at_string filename);
at_input_read_func at_input_get_handler_by_suffix (at_string suffix);
char ** at_input_list_new (void);
void at_input_list_free(char ** list);
/* at_input_read_add_handler (at_string suffix, at_input_read_func); */

/* Spline related */
at_splines_type * at_splines_new (at_bitmap_type * bitmap,
				  at_fitting_opts_type * opts);
/* notify_progress is called repeatedly inside at_splines_new_full
   to notify the progress of the execution. This might be useful for 
   interactive applications. notify_progress is called following 
   format:

   notify_progress (percentage, progress_data);

   test_cancel is called repeatedly inside at_splines_new_full
   to test whether the execution is canceled or not.
   If test_cancel returns TRUE, execution of at_splines_new_full
   is stopped as soon as possible and returns NULL. If test_cancel 
   returns FALSE, nothing happens. test_cancel  is called following
   format:

   test_cancel (testcancel_data);
   
   NULL is valid value for notify_progress and/or test_cancel if 
   you don't need to know the progress of the execution and/or 
   cancel the execution */ 
at_splines_type * at_splines_new_full (at_bitmap_type * bitmap,
				       at_fitting_opts_type * opts,
				       at_progress_func notify_progress,
				       at_address progress_data,
				       at_testcancel_func test_cancel,
				       at_address testcancel_data);

void at_splines_free (at_splines_type * splines);
/* TODO internal data access */

/* Output related */
at_output_write_func at_output_get_handler (at_string suffix);
void 
at_output_write(at_output_write_func output_writer,
		FILE * writeto,
		char * name,
		int llx, int lly, int urx, int ury, int dpi,
		at_splines_type * splines);
char ** at_output_list_new (void);
void at_output_list_free(char ** list);
/* at_output_write_add_handler (at_string suffix, at_output_write_func); */

/* Color related */
at_color_type * at_color_new (unsigned char r, 
			      unsigned char g,
			      unsigned char b);
at_color_type * at_color_copy (at_color_type * original);
void at_color_free(at_color_type * color);

/* Version and other informations */
const char * at_version ();
const char * at_home_site ();

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* AUTOTRACE_H */
