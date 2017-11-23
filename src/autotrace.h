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
#endif                          /* __cplusplus */

#ifndef N_
#define N_(x) x
#endif                          /* Not def: N_ */

/* ===================================================================== *
 * Typedefs
 * ===================================================================== */

#include "types.h"
#include "color.h"

/* Third degree is the highest we deal with.  */
  enum _at_polynomial_degree {
    AT_LINEARTYPE = 1,
    AT_QUADRATICTYPE = 2,
    AT_CUBICTYPE = 3,
    AT_PARALLELELLIPSETYPE = 4,
    AT_ELLIPSETYPE = 5,
    AT_CIRCLETYPE = 6
        /* not the real number of points to define a
           circle but to distinguish between a cubic spline */
  };

  enum _at_msg_type {
    AT_MSG_NOT_SET = 0,         /* is used in autotrace internally */
    AT_MSG_FATAL = 1,
    AT_MSG_WARNING,
  };

  typedef struct _at_fitting_opts_type at_fitting_opts_type;
  typedef struct _at_input_opts_type at_input_opts_type;
  typedef struct _at_output_opts_type at_output_opts_type;
  typedef struct _at_bitmap at_bitmap;
  typedef enum _at_polynomial_degree at_polynomial_degree;
  typedef struct _at_spline_type at_spline_type;
  typedef struct _at_spline_list_type at_spline_list_type;
  typedef struct _at_spline_list_array_type at_spline_list_array_type;
#define at_splines_type at_spline_list_array_type
  typedef enum _at_msg_type at_msg_type;

/* A Bezier spline can be represented as four points in the real plane:
   a starting point, ending point, and two control points.  The
   curve always lies in the convex hull defined by the four points.  It
   is also convenient to save the divergence of the spline from the
   straight line defined by the endpoints.  */
  struct _at_spline_type {
    at_real_coord v[4];         /* The control points.  */
    at_polynomial_degree degree;
    gfloat linearity;
  };

/* Each outline in a character is typically represented by many
   splines.  So, here is a list structure for that:  */
  struct _at_spline_list_type {
    at_spline_type *data;
    unsigned length;
    gboolean clockwise;
    at_color color;
    gboolean open;
  };

/* Each character is in general made up of many outlines. So here is one
   more list structure.  */
  struct _at_spline_list_array_type {
    at_spline_list_type *data;
    unsigned length;

    /* splines bbox */
    unsigned short height, width;

    /* the values for following members are inherited from
       at_fitting_opts_type */
    at_color *background_color;
    gboolean centerline;
    gboolean preserve_width;
    gfloat width_weight_factor;

  };

/* Fitting option.
   With using at_fitting_opts_doc macro, the description of
   each option could be get. e.g. at_fitting_opts_doc(background_color) */
  struct _at_fitting_opts_type {
#define at_doc__background_color					\
N_("background-color <hexadezimal>: the color of the background that "	\
"should be ignored, for example FFFFFF; "				\
"default is no background color.")
    at_color *background_color;

#define at_doc__charcode							\
N_("charcode <unsigned>: code of character to load from GF file, "	\
"allowed are 0..255; default is the first character in font.")
    unsigned charcode;

#define at_doc__color_count							\
N_("color-count <unsigned>: number of colors a color bitmap is reduced to, "	\
"it does not work on grayscale, allowed are 1..256; "				\
"default is 0, that means not color reduction is done.")
    unsigned color_count;

#define at_doc__corner_always_threshold						\
N_("corner-always-threshold <angle-in-degrees>: if the angle at a pixel is "	\
"less than this, it is considered a corner, even if it is within "		\
"`corner-surround' pixels of another corner; default is 60. ")
    gfloat corner_always_threshold;

#define at_doc__corner_surround						\
N_("corner-surround <unsigned>: number of pixels on either side of a "	\
"point to consider when determining if that point is a corner; "	\
"default is 4. ")
    unsigned corner_surround;

#define  at_doc__corner_threshold						\
N_("corner-threshold <angle-in-degrees>: if a pixel, its predecessor(s), "	\
"and its successor(s) meet at an angle smaller than this, it's a "		\
"corner; default is 100. ")
    gfloat corner_threshold;

#define at_doc__error_threshold						\
N_("error-threshold <real>: subdivide fitted curves that are off by "	\
"more pixels than this; default is 2.0. ")
    gfloat error_threshold;

#define  at_doc__filter_iterations					\
N_("filter-iterations <unsigned>: smooth the curve this many times "	\
"before fitting; default is 4.")
    unsigned filter_iterations;

#define at_doc__line_reversion_threshold					\
N_("line-reversion-threshold <real>: if a spline is closer to a straight "	\
"line than this, weighted by the square of the curve length, keep it a "	\
"straight line even if it is a list with curves; default is .01. ")
    gfloat line_reversion_threshold;

#define at_doc__line_threshold							\
N_("line-threshold <real>: if the spline is not more than this far away "	\
"from the straight line defined by its endpoints,"				\
"then output a straight line; default is 1. ")
    gfloat line_threshold;

#define at_doc__remove_adjacent_corners					\
N_("remove-adjacent-corners: remove corners that are adjacent; "	\
"default doesn't remove.")
    gboolean remove_adjacent_corners;

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
    gfloat despeckle_tightness;

#define at_doc__noise_removal				\
N_("noise-removal <real>: 1.0..0.0; default is 0.99. ")
    gfloat noise_removal;

#define  at_doc__centerline						\
N_("centerline: trace a character's centerline, rather than its outline. ")
    gboolean centerline;

#define at_doc__preserve_width							\
N_("preserve-width: whether to preserve linewith with centerline fitting; "	\
"default doesn't preserve.")
    gboolean preserve_width;

#define  at_doc__width_weight_factor				\
N_("width-weight-factor <real>: weight factor for fitting the linewidth.")
    gfloat width_weight_factor;
  };

  struct _at_input_opts_type {
    at_color *background_color;
    unsigned charcode;          /* Character code used only in GF input. */
  };

  struct _at_output_opts_type {
    int dpi;                    /* DPI is used only in MIF output. */
  };

  struct _at_bitmap {
    unsigned short height;
    unsigned short width;
    unsigned char *bitmap;
    unsigned int np;
  };

  typedef
  void (*at_msg_func) (const gchar * msg, at_msg_type msg_type, gpointer client_data);

/*
 * Autotrace initializer
 */
#define AUTOTRACE_INIT
  void autotrace_init(void);

/*
 * IO Handler typedefs
 */

  typedef struct _at_bitmap_reader at_bitmap_reader;
  struct _at_bitmap_reader;

  typedef struct _at_spline_writer at_spline_writer;
  struct _at_spline_writer;

/*
 * Progress handler typedefs
 * 0.0 <= percentage <= 1.0
 */
  typedef void (*at_progress_func) (gfloat percentage, gpointer client_data);

/*
 * Test cancel
 * Return TRUE if auto-tracing should be stopped.
 */
  typedef gboolean(*at_testcancel_func) (gpointer client_data);

/* ===================================================================== *
 * Functions
 * ===================================================================== */

/* --------------------------------------------------------------------- *
 * Fitting option related
 *
 * TODO: internal data access, copy
 * --------------------------------------------------------------------- */
  at_fitting_opts_type *at_fitting_opts_new(void);
  at_fitting_opts_type *at_fitting_opts_copy(at_fitting_opts_type * original);
  void at_fitting_opts_free(at_fitting_opts_type * opts);

/* Gettextize */
#define at_fitting_opts_doc(opt) at_fitting_opts_doc_func(at_doc__##opt)
/* Don't use next function directly from clients */
  const char *at_fitting_opts_doc_func(char *string);

/* --------------------------------------------------------------------- *
 * Input option related
 *
 * TODO: internal data access
 * --------------------------------------------------------------------- */
  at_input_opts_type *at_input_opts_new(void);
  at_input_opts_type *at_input_opts_copy(at_input_opts_type * original);
  void at_input_opts_free(at_input_opts_type * opts);

/* --------------------------------------------------------------------- *
 * Output option related
 *
 * TODO: internal data access
 * --------------------------------------------------------------------- */
  at_output_opts_type *at_output_opts_new(void);
  at_output_opts_type *at_output_opts_copy(at_output_opts_type * original);
  void at_output_opts_free(at_output_opts_type * opts);

/* --------------------------------------------------------------------- *
 * Bitmap related
 *
 * TODO: internal data access
 * --------------------------------------------------------------------- */

/* There is two way to build at_bitmap.
   1. Using input reader
      Use at_bitmap_read.
      at_input_get_handler_by_suffix or
      at_input_get_handler will help you to get at_bitmap_reader.
   2. Allocating a bitmap and rendering an image on it by yourself
      Use at_bitmap_new.

   In both case, you have to call at_bitmap_free when at_bitmap *
   data are no longer needed. */
  at_bitmap *at_bitmap_read(at_bitmap_reader * reader, gchar * filename, at_input_opts_type * opts, at_msg_func msg_func, gpointer msg_data);
  at_bitmap *at_bitmap_new(unsigned short width, unsigned short height, unsigned int planes);
  at_bitmap *at_bitmap_copy(const at_bitmap * src);

/* We have to export functions that supports internal datum
   access. Such functions might be useful for
   at_bitmap_new user. */
  unsigned short at_bitmap_get_width(const at_bitmap * bitmap);
  unsigned short at_bitmap_get_height(const at_bitmap * bitmap);
  unsigned short at_bitmap_get_planes(const at_bitmap * bitmap);
  void at_bitmap_get_color(const at_bitmap * bitmap, unsigned int row, unsigned int col, at_color * color);
  gboolean at_bitmap_equal_color(const at_bitmap * bitmap, unsigned int row, unsigned int col, at_color * color);
  void at_bitmap_free(at_bitmap * bitmap);

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
  at_splines_type *at_splines_new(at_bitmap * bitmap, at_fitting_opts_type * opts, at_msg_func msg_func, gpointer msg_data);

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
  at_splines_type *at_splines_new_full(at_bitmap * bitmap, at_fitting_opts_type * opts, at_msg_func msg_func, gpointer msg_data, at_progress_func notify_progress, gpointer progress_data, at_testcancel_func test_cancel, gpointer testcancel_data);

  void at_splines_write(at_spline_writer * writer, FILE * writeto, gchar * file_name, at_output_opts_type * opts, at_splines_type * splines, at_msg_func msg_func, gpointer msg_data);

  void at_splines_free(at_splines_type * splines);

/* --------------------------------------------------------------------- *
 * Input related
 * --------------------------------------------------------------------- */
  at_bitmap_reader *at_input_get_handler(gchar * filename);
  at_bitmap_reader *at_input_get_handler_by_suffix(gchar * suffix);

  const char **at_input_list_new(void);
  void at_input_list_free(const char **list);

/* at_input_shortlist
   return value: Do free by yourself */
  char *at_input_shortlist(void);

/* --------------------------------------------------------------------- *
 * Output related
 * --------------------------------------------------------------------- */
  at_spline_writer *at_output_get_handler(gchar * filename);
  at_spline_writer *at_output_get_handler_by_suffix(gchar * suffix);
  const char **at_output_list_new(void);
  void at_output_list_free(const char **list);

/* at_output_shortlist
   return value: Do free by yourself */
  char *at_output_shortlist(void);

/* --------------------------------------------------------------------- *
 * Misc
 * --------------------------------------------------------------------- */

/* at_version

   args:
   LONG_FORMAT == TRUE: "AutoTrace version x.y"
   LONG_FORMAT == FALSE: "x.y"

   return value: Don't free. It is allocated statically */
  const char *at_version(gboolean long_format);

/* at_home_site

   return value: Don't free. It is allocated statically */
  const char *at_home_site(void);

#ifdef __cplusplus
}
#endif                          /* __cplusplus */
#endif                          /* AUTOTRACE_H */
