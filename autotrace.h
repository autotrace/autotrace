/* autotrace.h --- Autotrace API

  Copyright (C) 2000, 2001, 2002 Martin Weber

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

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#ifndef N_
#define N_(x) x
#endif /* Not def: N_ */
 
/* ===================================================================== *
 * Typedefs
 * ===================================================================== */

#include "types.h"

typedef struct _at_fitting_opts_type at_fitting_opts_type;
typedef struct _at_input_opts_type   at_input_opts_type;
typedef struct _at_output_opts_type  at_output_opts_type;
typedef struct _at_bitmap_type at_bitmap_type;
typedef struct _at_color_type at_color_type;
typedef enum _at_polynomial_degree at_polynomial_degree;
typedef struct _at_spline_type at_spline_type;
typedef struct _at_spline_list_type at_spline_list_type;
typedef struct _at_spline_list_array_type at_spline_list_array_type;
#define at_splines_type at_spline_list_array_type 
typedef enum _at_msg_type at_msg_type;

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
  at_real_coord v[4];	/* The control points.  */
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

  /* splines bbox */
  unsigned short height, width;
  
  /* the values for following members are inherited from 
     at_fitting_opts_type */
  at_color_type * background_color;
  at_bool centerline;
  at_bool preserve_width;
  at_real width_weight_factor;

};

/* Fitting option.
   With using at_fitting_opts_doc macro, the description of 
   each option could be get. e.g. at_fitting_opts_doc(background_color) */
struct _at_fitting_opts_type
{
#define at_doc__background_color					\
N_("background-color <hexadezimal>: the color of the background that "	\
"should be ignored, for example FFFFFF; "				\
"default is no background color.")
  at_color_type *background_color;

#define at_doc__color_count							\
N_("color-count <unsigned>: number of colors a color bitmap is reduced to, "	\
"it does not work on grayscale, allowed are 1..256; "				\
"default is 0, that means not color reduction is done.")
  unsigned color_count;

#define at_doc__corner_always_threshold						\
N_("corner-always-threshold <angle-in-degrees>: if the angle at a pixel is "	\
"less than this, it is considered a corner, even if it is within "		\
"`corner-surround' pixels of another corner; default is 60. ")
  at_real corner_always_threshold;

#define at_doc__corner_surround						\
N_("corner-surround <unsigned>: number of pixels on either side of a "	\
"point to consider when determining if that point is a corner; "	\
"default is 4. ")
  unsigned corner_surround;

#define  at_doc__corner_threshold						\
N_("corner-threshold <angle-in-degrees>: if a pixel, its predecessor(s), "	\
"and its successor(s) meet at an angle smaller than this, it's a "		\
"corner; default is 100. ")
  at_real corner_threshold;

#define at_doc__error_threshold						\
N_("error-threshold <real>: subdivide fitted curves that are off by "	\
"more pixels than this; default is 2.0. ")
  at_real error_threshold;

#define  at_doc__filter_iterations					\
N_("filter-iterations <unsigned>: smooth the curve this many times "	\
"before fitting; default is 4.")
  unsigned filter_iterations;

#define at_doc__line_reversion_threshold					\
N_("line-reversion-threshold <real>: if a spline is closer to a straight "	\
"line than this, weighted by the square of the curve length, keep it a "	\
"straight line even if it is a list with curves; default is .01. ")
  at_real line_reversion_threshold;

#define at_doc__line_threshold							\
N_("line-threshold <real>: if the spline is not more than this far away "	\
"from the straight line defined by its endpoints,"				\
"then output a straight line; default is 1. ")
  at_real line_threshold;

#define at_doc__remove_adjacent_corners					\
N_("remove-adjacent-corners: remove corners that are adjacent; "	\
"default doesn't remove.")
  at_bool remove_adjacent_corners;

#define at_doc__tangent_surround					\
N_("tangent-surround <unsigned>: number of points on either side of a "	\
"point to consider when computing the tangent at that point; "		\
" default is 3.")
  unsigned tangent_surround;

#define at_doc__despeckle_level						\
N_("despeckle-level <unsigned>: 0..20; default is no despeckling. ")
  unsigned despeckle_level;

#define at_doc__despeckle_tightness				\
N_("despeckle-tightness <real>: 0.0..8.0; default is 2.0. ")
  at_real despeckle_tightness;

#define  at_doc__centerline						\
N_("centerline: trace a character's centerline, rather than its outline. ")
  at_bool centerline;

#define at_doc__preserve_width							\
N_("preserve-width: whether to preserve linewith with centerline fitting; "	\
"default doesn't preserve.")
  at_bool preserve_width;

#define  at_doc__width_weight_factor				\
N_("width-weight-factor: weight factor for fitting the linewidth")
  at_real width_weight_factor;
};

struct _at_input_opts_type
{
  at_color_type *background_color;
};

struct _at_output_opts_type
{
  int dpi;			/* DPI is used only in MIF output.*/
};

struct _at_bitmap_type
{
  unsigned short height;
  unsigned short width;
  unsigned char *bitmap;
  unsigned int np;
};

enum _at_msg_type
{
  AT_MSG_FATAL = 1,
  AT_MSG_WARNING,
};

typedef
void (* at_msg_func) (at_string msg, at_msg_type msg_type, at_address client_data);

/*
 * IO Handler typedefs
 */

typedef 
at_bitmap_type (*at_input_read_func)   (at_string name,
					at_input_opts_type * opts,
					at_msg_func msg_func, 
					at_address msg_data);

typedef 
int            (*at_output_write_func) (FILE*, at_string name,
					int llx, int lly, 
					int urx, int ury,
					at_output_opts_type * opts,
					at_splines_type shape,
					at_msg_func msg_func, 
					at_address msg_data);

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

/* ===================================================================== *
 * Functions
 * ===================================================================== */

/* --------------------------------------------------------------------- *
 * Fitting option related
 *
 * TODO: internal data access, copy
 * --------------------------------------------------------------------- */
at_fitting_opts_type * at_fitting_opts_new(void);
at_fitting_opts_type * at_fitting_opts_copy (at_fitting_opts_type * original); 
void at_fitting_opts_free(at_fitting_opts_type * opts);

/* TODO: Gettextize */
#define at_fitting_opts_doc(opt) _(at_doc__##opt)

/* --------------------------------------------------------------------- *
 * Input option related
 *
 * TODO: internal data access
 * --------------------------------------------------------------------- */
at_input_opts_type * at_input_opts_new(void);
at_input_opts_type * at_input_opts_copy(at_input_opts_type * original);
void at_input_opts_free(at_input_opts_type * opts);

/* --------------------------------------------------------------------- *
 * Output option related
 *
 * TODO: internal data access
 * --------------------------------------------------------------------- */
at_output_opts_type * at_output_opts_new(void);
at_output_opts_type * at_output_opts_copy(at_output_opts_type * original);
void at_output_opts_free(at_output_opts_type * opts);

/* --------------------------------------------------------------------- *
 * Bitmap related 
 *
 * TODO: internal data access
 * --------------------------------------------------------------------- */

/* There is two way to build at_bitmap_type.
   1. Using input reader
      Use at_bitmap_read. 
      at_input_get_handler_by_suffix or
      at_input_get_handler will help you to get at_input_read_func.
   2. Allocating a bitmap and rendering an image on it by yourself
      Use at_bitmap_new.

   In both case, you have to call at_bitmap_free when at_bitmap_type * 
   data are no longer needed. */
at_bitmap_type * at_bitmap_read (at_input_read_func input_reader,
				 at_string filename,
				 at_input_opts_type * opts,
				 at_msg_func msg_func, at_address msg_data);
at_bitmap_type * at_bitmap_new(unsigned short width,
			       unsigned short height,
			       unsigned int planes);
at_bitmap_type * at_bitmap_copy(at_bitmap_type * src);

/* We have to export functions that supports internal datum 
   access. Such functions might be useful for 
   at_bitmap_new user. */
unsigned short at_bitmap_get_width (at_bitmap_type * bitmap);
unsigned short at_bitmap_get_height (at_bitmap_type * bitmap);
unsigned short at_bitmap_get_planes (at_bitmap_type * bitmap);
void at_bitmap_free (at_bitmap_type * bitmap);


/* --------------------------------------------------------------------- *
 * Spline related
 *
 * TODO: internal data access
 * --------------------------------------------------------------------- */
/* at_splines_new
   
   args:

   BITMAP is modified in at_splines_new according to opts. Therefore
   if you need the original bitmap image, you have to make a backup of
   BITMAP with using at_bitmap_copy.

   MSG_FUNC and MSG_DATA are used to notify a client errors and
   warning from autotrace. NULL is valid value for MSG_FUNC if
   the client is not interested in the notifications. */
at_splines_type * at_splines_new (at_bitmap_type * bitmap,
				  at_fitting_opts_type * opts,
				  at_msg_func msg_func, at_address msg_data);

/* at_splines_new_full

   args:

   NOTIFY_PROGRESS is called repeatedly inside at_splines_new_full
   to notify the progress of the execution. This might be useful for 
   interactive applications. NOTIFY_PROGRESS is called following 
   format:

   NOTIFY_PROGRESS (percentage, progress_data);

   test_cancel is called repeatedly inside at_splines_new_full
   to test whether the execution is canceled or not.
   If test_cancel returns TRUE, execution of at_splines_new_full
   is stopped as soon as possible and returns NULL. If test_cancel 
   returns FALSE, nothing happens. test_cancel  is called following
   format:

   TEST_CANCEL (testcancel_data);
   
   NULL is valid value for notify_progress and/or test_cancel if 
   you don't need to know the progress of the execution and/or 
   cancel the execution */ 
at_splines_type * at_splines_new_full (at_bitmap_type * bitmap,
				       at_fitting_opts_type * opts,
				       at_msg_func msg_func, 
				       at_address msg_data,
				       at_progress_func notify_progress,
				       at_address progress_data,
				       at_testcancel_func test_cancel,
				       at_address testcancel_data);

void at_splines_write(at_output_write_func output_writer,
		  FILE * writeto,
		  at_string file_name,
		  at_output_opts_type * opts,
		  at_splines_type * splines,
		  at_msg_func msg_func, at_address msg_data);

void at_splines_free (at_splines_type * splines);

/* --------------------------------------------------------------------- *
 * Color related 
 * --------------------------------------------------------------------- */
at_color_type * at_color_new   (unsigned char r, 
				unsigned char g,
			        unsigned char b);
at_color_type * at_color_copy  (at_color_type * original);
at_bool         at_color_equal (at_color_type * c1, at_color_type * c2);
void            at_color_free  (at_color_type * color);

/* --------------------------------------------------------------------- *
 * Input related 
 * --------------------------------------------------------------------- */
at_input_read_func at_input_get_handler (at_string filename);
at_input_read_func at_input_get_handler_by_suffix (at_string suffix);

char ** at_input_list_new (void);
void at_input_list_free(char ** list);

/* at_input_shortlist
   return value: Do free by yourself */
char * at_input_shortlist (void); 

/* --------------------------------------------------------------------- *
 * Output related
 * --------------------------------------------------------------------- */
at_output_write_func at_output_get_handler (at_string filename);
at_output_write_func at_output_get_handler_by_suffix (at_string suffix);
char ** at_output_list_new (void);
void at_output_list_free(char ** list);

/* at_output_shortlist
   return value: Do free by yourself */
char * at_output_shortlist (void); 

/* --------------------------------------------------------------------- *
 * Misc
 * --------------------------------------------------------------------- */

/* at_version

   args:
   LONG_FORMAT == true: "AutoTrace version x.y"
   LONG_FORMAT == false: "x.y" 

   return value: Don't free. It is allocated statically */
const char * at_version (at_bool long_format);

/* at_home_site

   return value: Don't free. It is allocated statically */
const char * at_home_site (void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* AUTOTRACE_H */
