/* fit.c: turn a bitmap representation of a curve into a list of splines.
    Some of the ideas, but not the code, comes from the Phoenix thesis.
   See README for the reference.

   The code was partially derived from limn.

   Copyright (C) 1992 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* Def: HAVE_CONFIG_H */

#include "autotrace.h"
#include "fit.h"
#include "logreport.h"
#include "spline.h"
#include "vector.h"
#include "curve.h"
#include "pxl-outline.h"
#include "epsilon-equal.h"
#include "xstd.h"
#include <math.h>
#ifndef FLT_MAX
#include <limits.h>
#include <float.h>
#endif
#ifndef FLT_MIN
#include <limits.h>
#include <float.h>
#endif
#include <string.h>
#include <assert.h>

#define SQUARE(x) ((x) * (x))
#define CUBE(x) ((x) * (x) * (x))

/* We need to manipulate lists of array indices.  */

typedef struct index_list {
  unsigned *data;
  unsigned length;
} index_list_type;

/* The usual accessor macros.  */
#define GET_INDEX(i_l, n)  ((i_l).data[n])
#define INDEX_LIST_LENGTH(i_l)  ((i_l).length)
#define GET_LAST_INDEX(i_l)  ((i_l).data[INDEX_LIST_LENGTH (i_l) - 1])

static void append_index(index_list_type *, unsigned);
static void free_index_list(index_list_type *);
static index_list_type new_index_list(void);
static void remove_adjacent_corners(index_list_type *, unsigned, gboolean, at_exception_type * exception);
static void change_bad_lines(spline_list_type *, fitting_opts_type *);
static void filter(curve_type, fitting_opts_type *);
static void find_vectors(unsigned, pixel_outline_type, vector_type *, vector_type *, unsigned);
static index_list_type find_corners(pixel_outline_type, fitting_opts_type *, at_exception_type * exception);
static gfloat find_error(curve_type, spline_type, unsigned *, at_exception_type * exception);
static vector_type find_half_tangent(curve_type, gboolean start, unsigned *, unsigned);
static void find_tangent(curve_type, gboolean, gboolean, unsigned);
static spline_type fit_one_spline(curve_type, at_exception_type * exception);
static spline_list_type *fit_curve(curve_type, fitting_opts_type *, at_exception_type * exception);
static spline_list_type fit_curve_list(curve_list_type, fitting_opts_type *, at_distance_map *, at_exception_type * exception);
static spline_list_type *fit_with_least_squares(curve_type, fitting_opts_type *, at_exception_type * exception);
static spline_list_type *fit_with_line(curve_type);
static void remove_knee_points(curve_type, gboolean);
static void set_initial_parameter_values(curve_type);
static gboolean spline_linear_enough(spline_type *, curve_type, fitting_opts_type *);
static curve_list_array_type split_at_corners(pixel_outline_list_type, fitting_opts_type *, at_exception_type * exception);
static at_coord real_to_int_coord(at_real_coord);
static gfloat distance(at_real_coord, at_real_coord);

/* Get a new set of fitting options */
fitting_opts_type new_fitting_opts(void)
{
  fitting_opts_type fitting_opts;

  fitting_opts.background_color = NULL;
  fitting_opts.charcode = 0;
  fitting_opts.color_count = 0;
  fitting_opts.corner_always_threshold = (gfloat) 60.0;
  fitting_opts.corner_surround = 4;
  fitting_opts.corner_threshold = (gfloat) 100.0;
  fitting_opts.error_threshold = (gfloat) 2.0;
  fitting_opts.filter_iterations = 4;
  fitting_opts.line_reversion_threshold = (gfloat) .01;
  fitting_opts.line_threshold = (gfloat) 1.0;
  fitting_opts.remove_adjacent_corners = FALSE;
  fitting_opts.tangent_surround = 3;
  fitting_opts.despeckle_level = 0;
  fitting_opts.despeckle_tightness = 2.0;
  fitting_opts.noise_removal = (gfloat) 0.99;
  fitting_opts.centerline = FALSE;
  fitting_opts.preserve_width = FALSE;
  fitting_opts.width_weight_factor = 6.0;

  return (fitting_opts);
}

/* The top-level call that transforms the list of pixels in the outlines
   of the original character to a list of spline lists fitted to those
   pixels.  */

spline_list_array_type fitted_splines(pixel_outline_list_type pixel_outline_list, fitting_opts_type * fitting_opts, at_distance_map * dist, unsigned short width, unsigned short height, at_exception_type * exception, at_progress_func notify_progress, gpointer progress_data, at_testcancel_func test_cancel, gpointer testcancel_data)
{
  unsigned this_list;

  spline_list_array_type char_splines = new_spline_list_array();
  curve_list_array_type curve_array = split_at_corners(pixel_outline_list,
                                                       fitting_opts,
                                                       exception);

  char_splines.centerline = fitting_opts->centerline;
  char_splines.preserve_width = fitting_opts->preserve_width;
  char_splines.width_weight_factor = fitting_opts->width_weight_factor;

  if (fitting_opts->background_color)
    char_splines.background_color = at_color_copy(fitting_opts->background_color);
  else
    char_splines.background_color = NULL;
  /* Set dummy values. Real value is set in upper context. */
  char_splines.width = width;
  char_splines.height = height;

  for (this_list = 0; this_list < CURVE_LIST_ARRAY_LENGTH(curve_array); this_list++) {
    spline_list_type curve_list_splines;
    curve_list_type curves = CURVE_LIST_ARRAY_ELT(curve_array, this_list);

    if (notify_progress)
      notify_progress((((gfloat) this_list) / ((gfloat) CURVE_LIST_ARRAY_LENGTH(curve_array) * (gfloat) 3.0) + (gfloat) 0.333), progress_data);
    if (test_cancel && test_cancel(testcancel_data))
      goto cleanup;

    LOG("\nFitting curve list #%u:\n", this_list);

    curve_list_splines = fit_curve_list(curves, fitting_opts, dist, exception);
    if (at_exception_got_fatal(exception)) {
      if (char_splines.background_color)
        at_color_free(char_splines.background_color);
      goto cleanup;
    }
    curve_list_splines.clockwise = curves.clockwise;

    memcpy(&(curve_list_splines.color), &(O_LIST_OUTLINE(pixel_outline_list, this_list).color), sizeof(at_color));
    append_spline_list(&char_splines, curve_list_splines);
  }
cleanup:
  free_curve_list_array(&curve_array, notify_progress, progress_data);

  return char_splines;
}

/* Fit the list of curves CURVE_LIST to a list of splines, and return
   it.  CURVE_LIST represents a single closed paths, e.g., either the
   inside or outside outline of an `o'.  */

static spline_list_type fit_curve_list(curve_list_type curve_list, fitting_opts_type * fitting_opts, at_distance_map * dist, at_exception_type * exception)
{
  curve_type curve;
  unsigned this_curve, this_spline;
  unsigned curve_list_length = CURVE_LIST_LENGTH(curve_list);
  spline_list_type curve_list_splines = empty_spline_list();

  curve_list_splines.open = curve_list.open;

  /* Remove the extraneous ``knee'' points before filtering.  Since the
     corners have already been found, we don't need to worry about
     removing a point that should be a corner.  */

  LOG("\nRemoving knees:\n");
  for (this_curve = 0; this_curve < curve_list_length; this_curve++) {
    LOG("#%u:", this_curve);
    remove_knee_points(CURVE_LIST_ELT(curve_list, this_curve), CURVE_LIST_CLOCKWISE(curve_list));
  }

  if (dist != NULL) {
    unsigned this_point;
    unsigned height = dist->height;
    for (this_curve = 0; this_curve < curve_list_length; this_curve++) {
      curve = CURVE_LIST_ELT(curve_list, this_curve);
      for (this_point = 0; this_point < CURVE_LENGTH(curve); this_point++) {
        unsigned x, y;
        float width, w;
        at_real_coord *coord = &CURVE_POINT(curve, this_point);
        x = (unsigned)(coord->x);
        y = height - (unsigned)(coord->y) - 1;

        /* Each (x, y) is a point on the skeleton of the curve, which
           might be offset from the TRUE centerline, where the width
           is maximal.  Therefore, use as the local line width the
           maximum distance over the neighborhood of (x, y).  */
        width = dist->d[y][x];
        if (y >= 1) {
          if ((w = dist->d[y - 1][x]) > width)
            width = w;
          if (x >= 1) {
            if ((w = dist->d[y][x - 1]) > width)
              width = w;
            if ((w = dist->d[y - 1][x - 1]) > width)
              width = w;
          }
          if (x + 1 < dist->width) {
            if ((w = dist->d[y][x + 1]) > width)
              width = w;
            if ((w = dist->d[y - 1][x + 1]) > width)
              width = w;
          }
        }
        if (y + 1 < height) {
          if ((w = dist->d[y + 1][x]) > width)
            width = w;
          if (x >= 1 && (w = dist->d[y + 1][x - 1]) > width)
            width = w;
          if (x + 1 < dist->width && (w = dist->d[y + 1][x + 1]) > width)
            width = w;
        }
        coord->z = width * (fitting_opts->width_weight_factor);
      }
    }
  }

  /* We filter all the curves in CURVE_LIST at once; otherwise, we would
     look at an unfiltered curve when computing tangents.  */

  LOG("\nFiltering curves:\n");
  for (this_curve = 0; this_curve < curve_list.length; this_curve++) {
    LOG("#%u: ", this_curve);
    filter(CURVE_LIST_ELT(curve_list, this_curve), fitting_opts);
  }

  /* Make the first point in the first curve also be the last point in
     the last curve, so the fit to the whole curve list will begin and
     end at the same point.  This may cause slight errors in computing
     the tangents and t values, but it's worth it for the continuity.
     Of course we don't want to do this if the two points are already
     the same, as they are if the curve is cyclic.  (We don't append it
     earlier, in `split_at_corners', because that confuses the
     filtering.)  Finally, we can't append the point if the curve is
     exactly three points long, because we aren't adding any more data,
     and three points isn't enough to determine a spline.  Therefore,
     the fitting will fail.  */
  curve = CURVE_LIST_ELT(curve_list, 0);
  if (CURVE_CYCLIC(curve) == TRUE)
    append_point(curve, CURVE_POINT(curve, 0));

  /* Finally, fit each curve in the list to a list of splines.  */
  for (this_curve = 0; this_curve < curve_list_length; this_curve++) {
    spline_list_type *curve_splines;
    curve_type current_curve = CURVE_LIST_ELT(curve_list, this_curve);

    LOG("\nFitting curve #%u:\n", this_curve);

    curve_splines = fit_curve(current_curve, fitting_opts, exception);
    if (at_exception_got_fatal(exception))
      goto cleanup;
    else if (curve_splines == NULL) {
      LOG("Could not fit curve #%u", this_curve);
      at_exception_warning(exception, "Could not fit curve");
    } else {
      LOG("Fitted splines for curve #%u:\n", this_curve);
      for (this_spline = 0; this_spline < SPLINE_LIST_LENGTH(*curve_splines); this_spline++) {
        LOG("  %u: ", this_spline);
        if (logging)
          print_spline(SPLINE_LIST_ELT(*curve_splines, this_spline));
      }

      /* After fitting, we may need to change some would-be lines
         back to curves, because they are in a list with other
         curves.  */
      change_bad_lines(curve_splines, fitting_opts);

      concat_spline_lists(&curve_list_splines, *curve_splines);
      free_spline_list(*curve_splines);
      free(curve_splines);
    }
  }

  if (logging) {
    LOG("\nFitted splines are:\n");
    for (this_spline = 0; this_spline < SPLINE_LIST_LENGTH(curve_list_splines); this_spline++) {
      LOG("  %u: ", this_spline);
      print_spline(SPLINE_LIST_ELT(curve_list_splines, this_spline));
    }
  }
cleanup:
  return curve_list_splines;
}

/* Transform a set of locations to a list of splines (the fewer the
   better).  We are guaranteed that CURVE does not contain any corners.
   We return NULL if we cannot fit the points at all.  */

static spline_list_type *fit_curve(curve_type curve, fitting_opts_type * fitting_opts, at_exception_type * exception)
{
  spline_list_type *fittedsplines;

  if (CURVE_LENGTH(curve) < 2) {
    LOG("Tried to fit curve with less than two points");
    at_exception_warning(exception, "Tried to fit curve with less than two points");
    return NULL;
  }

  /* Do we have enough points to fit with a spline?  */
  fittedsplines = CURVE_LENGTH(curve) < 4 ? fit_with_line(curve)
      : fit_with_least_squares(curve, fitting_opts, exception);

  return fittedsplines;
}

/* As mentioned above, the first step is to find the corners in
   PIXEL_LIST, the list of points.  (Presumably we can't fit a single
   spline around a corner.)  The general strategy is to look through all
   the points, remembering which we want to consider corners.  Then go
   through that list, producing the curve_list.  This is dictated by the
   fact that PIXEL_LIST does not necessarily start on a corner---it just
   starts at the character's first outline pixel, going left-to-right,
   top-to-bottom.  But we want all our splines to start and end on real
   corners.

   For example, consider the top of a capital `C' (this is in cmss20):
                     x
                     ***********
                  ******************

   PIXEL_LIST will start at the pixel below the `x'.  If we considered
   this pixel a corner, we would wind up matching a very small segment
   from there to the end of the line, probably as a straight line, which
   is certainly not what we want.

   PIXEL_LIST has one element for each closed outline on the character.
   To preserve this information, we return an array of curve_lists, one
   element (which in turn consists of several curves, one between each
   pair of corners) for each element in PIXEL_LIST.  */

static curve_list_array_type split_at_corners(pixel_outline_list_type pixel_list, fitting_opts_type * fitting_opts, at_exception_type * exception)
{
  unsigned this_pixel_o;
  curve_list_array_type curve_array = new_curve_list_array();

  LOG("\nFinding corners:\n");

  for (this_pixel_o = 0; this_pixel_o < O_LIST_LENGTH(pixel_list); this_pixel_o++) {
    curve_type curve, first_curve;
    index_list_type corner_list;
    unsigned p, this_corner;
    curve_list_type curve_list = new_curve_list();
    pixel_outline_type pixel_o = O_LIST_OUTLINE(pixel_list, this_pixel_o);

    CURVE_LIST_CLOCKWISE(curve_list) = O_CLOCKWISE(pixel_o);
    curve_list.open = pixel_o.open;

    LOG("#%u:", this_pixel_o);

    /* If the outline does not have enough points, we can't do
       anything.  The endpoints of the outlines are automatically
       corners.  We need at least `corner_surround' more pixels on
       either side of a point before it is conceivable that we might
       want another corner.  */
    if (O_LENGTH(pixel_o) > fitting_opts->corner_surround * 2 + 2)
      corner_list = find_corners(pixel_o, fitting_opts, exception);

    else {
      int surround;
      if ((surround = (int)(O_LENGTH(pixel_o) - 3) / 2) >= 2) {
        unsigned save_corner_surround = fitting_opts->corner_surround;
        fitting_opts->corner_surround = surround;
        corner_list = find_corners(pixel_o, fitting_opts, exception);
        fitting_opts->corner_surround = save_corner_surround;
      } else {
        corner_list.length = 0;
        corner_list.data = NULL;
      }
    }

    /* Remember the first curve so we can make it be the `next' of the
       last one.  (And vice versa.)  */
    first_curve = new_curve();

    curve = first_curve;

    if (corner_list.length == 0) {  /* No corners.  Use all of the pixel outline as the curve.  */
      for (p = 0; p < O_LENGTH(pixel_o); p++)
        append_pixel(curve, O_COORDINATE(pixel_o, p));

      if (curve_list.open == TRUE)
        CURVE_CYCLIC(curve) = FALSE;
      else
        CURVE_CYCLIC(curve) = TRUE;
    } else {                    /* Each curve consists of the points between (inclusive) each pair
                                   of corners.  */
      for (this_corner = 0; this_corner < corner_list.length - 1; this_corner++) {
        curve_type previous_curve = curve;
        unsigned corner = GET_INDEX(corner_list, this_corner);
        unsigned next_corner = GET_INDEX(corner_list, this_corner + 1);

        for (p = corner; p <= next_corner; p++)
          append_pixel(curve, O_COORDINATE(pixel_o, p));

        append_curve(&curve_list, curve);
        curve = new_curve();
        NEXT_CURVE(previous_curve) = curve;
        PREVIOUS_CURVE(curve) = previous_curve;
      }

      /* The last curve is different.  It consists of the points
         (inclusive) between the last corner and the end of the list,
         and the beginning of the list and the first corner.  */
      for (p = GET_LAST_INDEX(corner_list); p < O_LENGTH(pixel_o); p++)
        append_pixel(curve, O_COORDINATE(pixel_o, p));

      if (!pixel_o.open) {
        for (p = 0; p <= GET_INDEX(corner_list, 0); p++)
          append_pixel(curve, O_COORDINATE(pixel_o, p));
      } else {
        curve_type last_curve = PREVIOUS_CURVE(curve);
        PREVIOUS_CURVE(first_curve) = NULL;
        if (last_curve)
          NEXT_CURVE(last_curve) = NULL;
      }
    }

    LOG(" [%u].\n", corner_list.length);
    free_index_list(&corner_list);

    /* Add `curve' to the end of the list, updating the pointers in
       the chain.  */
    append_curve(&curve_list, curve);
    NEXT_CURVE(curve) = first_curve;
    PREVIOUS_CURVE(first_curve) = curve;

    /* And now add the just-completed curve list to the array.  */
    append_curve_list(&curve_array, curve_list);
  }                             /* End of considering each pixel outline.  */

  return curve_array;
}

/* We consider a point to be a corner if (1) the angle defined by the
   `corner_surround' points coming into it and going out from it is less
   than `corner_threshold' degrees, and no point within
   `corner_surround' points has a smaller angle; or (2) the angle is less
   than `corner_always_threshold' degrees.

   Because of the different cases, it is convenient to have the
   following macro to append a corner on to the list we return.  The
   character argument C is simply so that the different cases can be
   distinguished in the log file.  */

#define APPEND_CORNER(index, angle, c)			\
  do							\
    {							\
      append_index (&corner_list, index);		\
      LOG (" (%d,%d)%c%.3f",				\
            O_COORDINATE (pixel_outline, index).x,	\
            O_COORDINATE (pixel_outline, index).y,	\
            c, angle);					\
    }							\
  while (0)

static index_list_type find_corners(pixel_outline_type pixel_outline, fitting_opts_type * fitting_opts, at_exception_type * exception)
{
  unsigned p, start_p, end_p;
  index_list_type corner_list = new_index_list();

  start_p = 0;
  end_p = O_LENGTH(pixel_outline) - 1;
  if (pixel_outline.open) {
    if (end_p <= fitting_opts->corner_surround * 2)
      return corner_list;
    APPEND_CORNER(0, 0.0, '@');
    start_p += fitting_opts->corner_surround;
    end_p -= fitting_opts->corner_surround;
  }

  /* Consider each pixel on the outline in turn.  */
  for (p = start_p; p <= end_p; p++) {
    gfloat corner_angle;
    vector_type in_vector, out_vector;

    /* Check if the angle is small enough.  */
    find_vectors(p, pixel_outline, &in_vector, &out_vector, fitting_opts->corner_surround);
    corner_angle = Vangle(in_vector, out_vector, exception);
    if (at_exception_got_fatal(exception))
      goto cleanup;

    if (fabs(corner_angle) <= fitting_opts->corner_threshold) {
      /* We want to keep looking, instead of just appending the
         first pixel we find with a small enough angle, since there
         might be another corner within `corner_surround' pixels, with
         a smaller angle.  If that is the case, we want that one.  */
      gfloat best_corner_angle = corner_angle;
      unsigned best_corner_index = p;
      index_list_type equally_good_list = new_index_list();
      /* As we come into the loop, `p' is the index of the point
         that has an angle less than `corner_angle'.  We use `i' to
         move through the pixels next to that, and `q' for moving
         through the adjacent pixels to each `p'.  */
      unsigned q = p;
      unsigned i = p + 1;

      while (TRUE) {
        /* Perhaps the angle is sufficiently small that we want to
           consider this a corner, even if it's not the best
           (unless we've already wrapped around in the search,
           i.e., `q<i', in which case we have already added the
           corner, and we don't want to add it again).  We want to
           do this check on the first candidate we find, as well
           as the others in the loop, hence this comes before the
           stopping condition.  */
        if (corner_angle <= fitting_opts->corner_always_threshold && q >= p)
          APPEND_CORNER(q, corner_angle, '\\');

        /* Exit the loop if we've looked at `corner_surround'
           pixels past the best one we found, or if we've looked
           at all the pixels.  */
        if (i >= best_corner_index + fitting_opts->corner_surround || i >= O_LENGTH(pixel_outline))
          break;

        /* Check the angle.  */
        q = i % O_LENGTH(pixel_outline);
        find_vectors(q, pixel_outline, &in_vector, &out_vector, fitting_opts->corner_surround);
        corner_angle = Vangle(in_vector, out_vector, exception);
        if (at_exception_got_fatal(exception))
          goto cleanup;

        /* If we come across a corner that is just as good as the
           best one, we should make it a corner, too.  This
           happens, for example, at the points on the `W' in some
           typefaces, where the ``points'' are flat.  */
        if (epsilon_equal(corner_angle, best_corner_angle))
          append_index(&equally_good_list, q);

        else if (corner_angle < best_corner_angle) {
          best_corner_angle = corner_angle;
          /* We want to check `corner_surround' pixels beyond the
             new best corner.  */
          i = best_corner_index = q;
          free_index_list(&equally_good_list);
          equally_good_list = new_index_list();
        }

        i++;
      }

      /* After we exit the loop, `q' is the index of the last point
         we checked.  We have already added the corner if
         `best_corner_angle' is less than `corner_always_threshold'.
         Again, if we've already wrapped around, we don't want to
         add the corner again.  */
      if (best_corner_angle > fitting_opts->corner_always_threshold && best_corner_index >= p) {
        unsigned j;

        APPEND_CORNER(best_corner_index, best_corner_angle, '/');

        for (j = 0; j < INDEX_LIST_LENGTH(equally_good_list); j++)
          APPEND_CORNER(GET_INDEX(equally_good_list, j), best_corner_angle, '@');
      }
      free_index_list(&equally_good_list);

      /* If we wrapped around in our search, we're done; otherwise,
         we don't want the outer loop to look at the pixels that we
         already looked at in searching for the best corner.  */
      p = (q < p) ? O_LENGTH(pixel_outline) : q;
    }                           /* End of searching for the best corner.  */
  }                             /* End of considering each pixel.  */

  if (INDEX_LIST_LENGTH(corner_list) > 0)
    /* We never want two corners next to each other, since the
       only way to fit such a ``curve'' would be with a straight
       line, which usually interrupts the continuity dreadfully.  */
    remove_adjacent_corners(&corner_list, O_LENGTH(pixel_outline) - (pixel_outline.open ? 2 : 1), fitting_opts->remove_adjacent_corners, exception);
cleanup:
  return corner_list;
}

/* Return the difference vectors coming in and going out of the outline
   OUTLINE at the point whose index is TEST_INDEX.  In Phoenix,
   Schneider looks at a single point on either side of the point we're
   considering.  That works for him because his points are not touching.
   But our points *are* touching, and so we have to look at
   `corner_surround' points on either side, to get a better picture of
   the outline's shape.  */

static void find_vectors(unsigned test_index, pixel_outline_type outline, vector_type * in, vector_type * out, unsigned corner_surround)
{
  int i;
  unsigned n_done;
  at_coord candidate = O_COORDINATE(outline, test_index);

  in->dx = in->dy = in->dz = 0.0;
  out->dx = out->dy = out->dz = 0.0;

  /* Add up the differences from p of the `corner_surround' points
     before p.  */
  for (i = O_PREV(outline, test_index), n_done = 0; n_done < corner_surround; i = O_PREV(outline, i), n_done++)
    *in = Vadd(*in, IPsubtract(O_COORDINATE(outline, i), candidate));

  /* And the points after p.  */
  for (i = O_NEXT(outline, test_index), n_done = 0; n_done < corner_surround; i = O_NEXT(outline, i), n_done++)
    *out = Vadd(*out, IPsubtract(O_COORDINATE(outline, i), candidate));
}

/* Remove adjacent points from the index list LIST.  We do this by first
   sorting the list and then running through it.  Since these lists are
   quite short, a straight selection sort (e.g., p.139 of the Art of
   Computer Programming, vol.3) is good enough.  LAST_INDEX is the index
   of the last pixel on the outline, i.e., the next one is the first
   pixel. We need this for checking the adjacency of the last corner.

   We need to do this because the adjacent corners turn into
   two-pixel-long curves, which can only be fit by straight lines.  */

static void remove_adjacent_corners(index_list_type * list, unsigned last_index, gboolean remove_adj_corners, at_exception_type * exception)
{
  unsigned j;
  unsigned last;
  index_list_type new_list = new_index_list();

  for (j = INDEX_LIST_LENGTH(*list) - 1; j > 0; j--) {
    unsigned search;
    unsigned temp;
    /* Find maximal element below `j'.  */
    unsigned max_index = j;

    for (search = 0; search < j; search++)
      if (GET_INDEX(*list, search) > GET_INDEX(*list, max_index))
        max_index = search;

    if (max_index != j) {
      temp = GET_INDEX(*list, j);
      GET_INDEX(*list, j) = GET_INDEX(*list, max_index);
      GET_INDEX(*list, max_index) = temp;

      /* xx -- really have to sort?  */
      LOG("needed exchange");
      at_exception_warning(exception, "needed exchange");
    }
  }

  /* The list is sorted.  Now look for adjacent entries.  Each time
     through the loop we insert the current entry and, if appropriate,
     the next entry.  */
  for (j = 0; j < INDEX_LIST_LENGTH(*list) - 1; j++) {
    unsigned current = GET_INDEX(*list, j);
    unsigned next = GET_INDEX(*list, j + 1);

    /* We should never have inserted the same element twice.  */
    /* assert (current != next); */

    if ((remove_adj_corners) && ((next == current + 1) || (next == current)))
      j++;

    append_index(&new_list, current);
  }

  /* Don't append the last element if it is 1) adjacent to the previous
     one; or 2) adjacent to the very first one.  */
  last = GET_LAST_INDEX(*list);
  if (INDEX_LIST_LENGTH(new_list) == 0 || !(last == GET_LAST_INDEX(new_list) + 1 || (last == last_index && GET_INDEX(*list, 0) == 0)))
    append_index(&new_list, last);

  free_index_list(list);
  *list = new_list;
}

/* A ``knee'' is a point which forms a ``right angle'' with its
   predecessor and successor.  See the documentation (the `Removing
   knees' section) for an example and more details.

   The argument CLOCKWISE tells us which direction we're moving.  (We
   can't figure that information out from just the single segment with
   which we are given to work.)

   We should never find two consecutive knees.

   Since the first and last points are corners (unless the curve is
   cyclic), it doesn't make sense to remove those.  */

/* This evaluates to TRUE if the vector V is zero in one direction and
   nonzero in the other.  */
#define ONLY_ONE_ZERO(v)                                                \
  (((v).dx == 0.0 && (v).dy != 0.0) || ((v).dy == 0.0 && (v).dx != 0.0))

/* There are four possible cases for knees, one for each of the four
   corners of a rectangle; and then the cases differ depending on which
   direction we are going around the curve.  The tests are listed here
   in the order of upper left, upper right, lower right, lower left.
   Perhaps there is some simple pattern to the
   clockwise/counterclockwise differences, but I don't see one.  */
#define CLOCKWISE_KNEE(prev_delta, next_delta)                                                  \
  ((prev_delta.dx == -1.0 && next_delta.dy == 1.0)                                              \
   || (prev_delta.dy == 1.0 && next_delta.dx == 1.0)                                    \
   || (prev_delta.dx == 1.0 && next_delta.dy == -1.0)                                   \
   || (prev_delta.dy == -1.0 && next_delta.dx == -1.0))

#define COUNTERCLOCKWISE_KNEE(prev_delta, next_delta)                                   \
  ((prev_delta.dy == 1.0 && next_delta.dx == -1.0)                                              \
   || (prev_delta.dx == 1.0 && next_delta.dy == 1.0)                                    \
   || (prev_delta.dy == -1.0 && next_delta.dx == 1.0)                                   \
   || (prev_delta.dx == -1.0 && next_delta.dy == -1.0))

static void remove_knee_points(curve_type curve, gboolean clockwise)
{
  unsigned i;
  unsigned offset = (CURVE_CYCLIC(curve) == TRUE) ? 0 : 1;
  at_coord previous = real_to_int_coord(CURVE_POINT(curve, CURVE_PREV(curve, offset)));
  curve_type trimmed_curve = copy_most_of_curve(curve);

  if (CURVE_CYCLIC(curve) == FALSE)
    append_pixel(trimmed_curve, real_to_int_coord(CURVE_POINT(curve, 0)));

  for (i = offset; i < CURVE_LENGTH(curve) - offset; i++) {
    at_coord current = real_to_int_coord(CURVE_POINT(curve, i));
    at_coord next = real_to_int_coord(CURVE_POINT(curve, CURVE_NEXT(curve, i)));
    vector_type prev_delta = IPsubtract(previous, current);
    vector_type next_delta = IPsubtract(next, current);

    if (ONLY_ONE_ZERO(prev_delta) && ONLY_ONE_ZERO(next_delta)
        && ((clockwise && CLOCKWISE_KNEE(prev_delta, next_delta))
            || (!clockwise && COUNTERCLOCKWISE_KNEE(prev_delta, next_delta))))
      LOG(" (%d,%d)", current.x, current.y);
    else {
      previous = current;
      append_pixel(trimmed_curve, current);
    }
  }

  if (CURVE_CYCLIC(curve) == FALSE)
    append_pixel(trimmed_curve, real_to_int_coord(LAST_CURVE_POINT(curve)));

  if (CURVE_LENGTH(trimmed_curve) == CURVE_LENGTH(curve))
    LOG(" (none)");

  LOG(".\n");

  free_curve(curve);
  *curve = *trimmed_curve;
  free(trimmed_curve);          /* free_curve? --- Masatake */
}

/* Smooth the curve by adding in neighboring points.  Do this
   `filter_iterations' times.  But don't change the corners.  */

static void filter(curve_type curve, fitting_opts_type * fitting_opts)
{
  unsigned iteration, this_point;
  unsigned offset = (CURVE_CYCLIC(curve) == TRUE) ? 0 : 1;
  at_real_coord prev_new_point;

  /* We must have at least three points---the previous one, the current
     one, and the next one.  But if we don't have at least five, we will
     probably collapse the curve down onto a single point, which means
     we won't be able to fit it with a spline.  */
  if (CURVE_LENGTH(curve) < 5) {
    LOG("Length is %u, not enough to filter.\n", CURVE_LENGTH(curve));
    return;
  }

  prev_new_point.x = FLT_MAX;
  prev_new_point.y = FLT_MAX;
  prev_new_point.z = FLT_MAX;

  for (iteration = 0; iteration < fitting_opts->filter_iterations; iteration++) {
    curve_type newcurve = copy_most_of_curve(curve);
    gboolean collapsed = FALSE;

    /* Keep the first point on the curve.  */
    if (offset)
      append_point(newcurve, CURVE_POINT(curve, 0));

    for (this_point = offset; this_point < CURVE_LENGTH(curve) - offset; this_point++) {
      vector_type in, out, sum;
      at_real_coord new_point;

      /* Calculate the vectors in and out, computed by looking at n points
         on either side of this_point. Experimental it was found that 2 is
         optimal. */

      signed int prev, prevprev;  /* have to be signed */
      unsigned int next, nextnext;
      at_real_coord candidate = CURVE_POINT(curve, this_point);

      prev = CURVE_PREV(curve, this_point);
      prevprev = CURVE_PREV(curve, prev);
      next = CURVE_NEXT(curve, this_point);
      nextnext = CURVE_NEXT(curve, next);

      /* Add up the differences from p of the `surround' points
         before p.  */
      in.dx = in.dy = in.dz = 0.0;

      in = Vadd(in, Psubtract(CURVE_POINT(curve, prev), candidate));
      if (prevprev >= 0)
        in = Vadd(in, Psubtract(CURVE_POINT(curve, prevprev), candidate));

      /* And the points after p.  Don't use more points after p than we
         ended up with before it.  */
      out.dx = out.dy = out.dz = 0.0;

      out = Vadd(out, Psubtract(CURVE_POINT(curve, next), candidate));
      if (nextnext < CURVE_LENGTH(curve))
        out = Vadd(out, Psubtract(CURVE_POINT(curve, nextnext), candidate));

      /* Start with the old point.  */
      new_point = candidate;
      sum = Vadd(in, out);
      /* We added 2*n+2 points, so we have to divide the sum by 2*n+2 */
      new_point.x += sum.dx / 6;
      new_point.y += sum.dy / 6;
      new_point.z += sum.dz / 6;
      if (fabs(prev_new_point.x - new_point.x) < 0.3 && fabs(prev_new_point.y - new_point.y) < 0.3 && fabs(prev_new_point.z - new_point.z) < 0.3) {
        collapsed = TRUE;
        break;
      }

      /* Put the newly computed point into a separate curve, so it
         doesn't affect future computation (on this iteration).  */
      append_point(newcurve, prev_new_point = new_point);
    }

    if (collapsed)
      free_curve(newcurve);
    else {
      /* Just as with the first point, we have to keep the last point.  */
      if (offset)
        append_point(newcurve, LAST_CURVE_POINT(curve));

      /* Set the original curve to the newly filtered one, and go again.  */
      free_curve(curve);
      *curve = *newcurve;
    }
    free(newcurve);
  }

  if (logging)
    log_curve(curve, FALSE);
}

/* This routine returns the curve fitted to a straight line in a very
   simple way: make the first and last points on the curve be the
   endpoints of the line.  This simplicity is justified because we are
   called only on very short curves.  */

static spline_list_type *fit_with_line(curve_type curve)
{
  spline_type line;

  LOG("Fitting with straight line:\n");

  SPLINE_DEGREE(line) = LINEARTYPE;
  START_POINT(line) = CONTROL1(line) = CURVE_POINT(curve, 0);
  END_POINT(line) = CONTROL2(line) = LAST_CURVE_POINT(curve);

  /* Make sure that this line is never changed to a cubic.  */
  SPLINE_LINEARITY(line) = 0;

  if (logging) {
    LOG("  ");
    print_spline(line);
  }

  return new_spline_list_with_spline(line);
}

/* The least squares method is well described in Schneider's thesis.
   Briefly, we try to fit the entire curve with one spline. If that
   fails, we subdivide the curve.  */

static spline_list_type *fit_with_least_squares(curve_type curve, fitting_opts_type * fitting_opts, at_exception_type * exception)
{
  gfloat error = 0, best_error = FLT_MAX;
  spline_type spline, best_spline;
  spline_list_type *spline_list = NULL;
  unsigned worst_point = 0;
  gfloat previous_error = FLT_MAX;

  LOG("\nFitting with least squares:\n");

  /* Phoenix reduces the number of points with a ``linear spline
     technique''.  But for fitting letterforms, that is
     inappropriate.  We want all the points we can get.  */

  /* It makes no difference whether we first set the `t' values or
     find the tangents.  This order makes the documentation a little
     more coherent.  */

  LOG("Finding tangents:\n");
  find_tangent(curve, /* to_start */ TRUE, /* cross_curve */ FALSE,
               fitting_opts->tangent_surround);
  find_tangent(curve, /* to_start */ FALSE, /* cross_curve */ FALSE,
               fitting_opts->tangent_surround);

  set_initial_parameter_values(curve);

  /* Now we loop, subdividing, until CURVE has
     been fit.  */
  while (TRUE) {
    spline = best_spline = fit_one_spline(curve, exception);
    if (at_exception_got_fatal(exception))
      goto cleanup;

    if (SPLINE_DEGREE(spline) == LINEARTYPE)
      LOG("  fitted to line:\n");
    else
      LOG("  fitted to spline:\n");

    if (logging) {
      LOG("    ");
      print_spline(spline);
    }

    if (SPLINE_DEGREE(spline) == LINEARTYPE)
      break;

    error = find_error(curve, spline, &worst_point, exception);
    if (error <= previous_error) {
      best_error = error;
      best_spline = spline;
    }
    break;
  }

  if (SPLINE_DEGREE(spline) == LINEARTYPE) {
    spline_list = new_spline_list_with_spline(spline);
    LOG("Accepted error of %.3f.\n", error);
    return (spline_list);
  }

  /* Go back to the best fit.  */
  spline = best_spline;
  error = best_error;

  if (error < fitting_opts->error_threshold && CURVE_CYCLIC(curve) == FALSE) {
    /* The points were fitted with a
       spline.  We end up here whenever a fit is accepted.  We have
       one more job: see if the ``curve'' that was fit should really
       be a straight line. */
    if (spline_linear_enough(&spline, curve, fitting_opts)) {
      SPLINE_DEGREE(spline) = LINEARTYPE;
      LOG("Changed to line.\n");
    }
    spline_list = new_spline_list_with_spline(spline);
    LOG("Accepted error of %.3f.\n", error);
  } else {
    /* We couldn't fit the curve acceptably, so subdivide.  */
    unsigned subdivision_index;
    spline_list_type *left_spline_list;
    spline_list_type *right_spline_list;
    curve_type left_curve = new_curve();
    curve_type right_curve = new_curve();

    /* Keep the linked list of curves intact.  */
    NEXT_CURVE(right_curve) = NEXT_CURVE(curve);
    PREVIOUS_CURVE(right_curve) = left_curve;
    NEXT_CURVE(left_curve) = right_curve;
    PREVIOUS_CURVE(left_curve) = curve;
    NEXT_CURVE(curve) = left_curve;

    LOG("\nSubdividing (error %.3f):\n", error);
    LOG("  Original point: (%.3f,%.3f), #%u.\n", CURVE_POINT(curve, worst_point).x, CURVE_POINT(curve, worst_point).y, worst_point);
    subdivision_index = worst_point;
    LOG("  Final point: (%.3f,%.3f), #%u.\n", CURVE_POINT(curve, subdivision_index).x, CURVE_POINT(curve, subdivision_index).y, subdivision_index);

    /* The last point of the left-hand curve will also be the first
       point of the right-hand curve.  */
    CURVE_LENGTH(left_curve) = subdivision_index + 1;
    CURVE_LENGTH(right_curve) = CURVE_LENGTH(curve) - subdivision_index;
    left_curve->point_list = curve->point_list;
    right_curve->point_list = curve->point_list + subdivision_index;

    /* We want to use the tangents of the curve which we are
       subdividing for the start tangent for left_curve and the
       end tangent for right_curve.  */
    CURVE_START_TANGENT(left_curve) = CURVE_START_TANGENT(curve);
    CURVE_END_TANGENT(right_curve) = CURVE_END_TANGENT(curve);

    /* We have to set up the two curves before finding the tangent at
       the subdivision point.  The tangent at that point must be the
       same for both curves, or noticeable bumps will occur in the
       character.  But we want to use information on both sides of the
       point to compute the tangent, hence cross_curve = true.  */
    find_tangent(left_curve, /* to_start_point: */ FALSE,
                 /* cross_curve: */ TRUE, fitting_opts->tangent_surround);
    CURVE_START_TANGENT(right_curve) = CURVE_END_TANGENT(left_curve);

    /* Now that we've set up the curves, we can fit them.  */
    left_spline_list = fit_curve(left_curve, fitting_opts, exception);
    if (at_exception_got_fatal(exception))
      /* TODO: Memory allocated for left_curve and right_curve
         will leak. */
      goto cleanup;

    right_spline_list = fit_curve(right_curve, fitting_opts, exception);
    /* TODO: Memory allocated for left_curve and right_curve
       will leak. */
    if (at_exception_got_fatal(exception))
      goto cleanup;

    /* Neither of the subdivided curves could be fit, so fail.  */
    if (left_spline_list == NULL && right_spline_list == NULL)
      return NULL;

    /* Put the two together (or whichever of them exist).  */
    spline_list = new_spline_list();

    if (left_spline_list == NULL) {
      LOG("Could not fit spline to left curve (%lx).\n", (unsigned long)left_curve);
      at_exception_warning(exception, "Could not fit left spline list");
    } else {
      concat_spline_lists(spline_list, *left_spline_list);
      free_spline_list(*left_spline_list);
      free(left_spline_list);
    }

    if (right_spline_list == NULL) {
      LOG("Could not fit spline to right curve (%lx).\n", (unsigned long)right_curve);
      at_exception_warning(exception, "Could not fit right spline list");
    } else {
      concat_spline_lists(spline_list, *right_spline_list);
      free_spline_list(*right_spline_list);
      free(right_spline_list);
    }
    if (CURVE_END_TANGENT(left_curve))
      free(CURVE_END_TANGENT(left_curve));
    free(left_curve);
    free(right_curve);
  }
cleanup:
  return spline_list;
}

/* Our job here is to find alpha1 (and alpha2), where t1_hat (t2_hat) is
   the tangent to CURVE at the starting (ending) point, such that:

   control1 = alpha1*t1_hat + starting point
   control2 = alpha2*t2_hat + ending_point

   and the resulting spline (starting_point .. control1 and control2 ..
   ending_point) minimizes the least-square error from CURVE.

   See pp.57--59 of the Phoenix thesis.

   The B?(t) here corresponds to B_i^3(U_i) there.
   The Bernshte\u in polynomials of degree n are defined by
   B_i^n(t) = { n \choose i } t^i (1-t)^{n-i}, i = 0..n  */

#define B0(t) CUBE ((gfloat) 1.0 - (t))
#define B1(t) ((gfloat) 3.0 * (t) * SQUARE ((gfloat) 1.0 - (t)))
#define B2(t) ((gfloat) 3.0 * SQUARE (t) * ((gfloat) 1.0 - (t)))
#define B3(t) CUBE (t)

static spline_type fit_one_spline(curve_type curve, at_exception_type * exception)
{
  /* Since our arrays are zero-based, the `C0' and `C1' here correspond
     to `C1' and `C2' in the paper.  */
  gfloat X_C1_det, C0_X_det, C0_C1_det;
  gfloat alpha1, alpha2;
  spline_type spline;
  vector_type start_vector, end_vector;
  unsigned i;
  vector_type *A;
  vector_type t1_hat = *CURVE_START_TANGENT(curve);
  vector_type t2_hat = *CURVE_END_TANGENT(curve);
  gfloat C[2][2] = { {0.0, 0.0}, {0.0, 0.0} };
  gfloat X[2] = { 0.0, 0.0 };

  XMALLOC(A, CURVE_LENGTH(curve) * 2 * sizeof(vector_type));  /* A dynamically allocated array. */

  START_POINT(spline) = CURVE_POINT(curve, 0);
  END_POINT(spline) = LAST_CURVE_POINT(curve);
  start_vector = make_vector(START_POINT(spline));
  end_vector = make_vector(END_POINT(spline));

  for (i = 0; i < CURVE_LENGTH(curve); i++) {
    A[(i << 1) + 0] = Vmult_scalar(t1_hat, B1(CURVE_T(curve, i)));
    A[(i << 1) + 1] = Vmult_scalar(t2_hat, B2(CURVE_T(curve, i)));
  }

  for (i = 0; i < CURVE_LENGTH(curve); i++) {
    vector_type temp, temp0, temp1, temp2, temp3;
    vector_type *Ai = A + (i << 1);

    C[0][0] += Vdot(Ai[0], Ai[0]);
    C[0][1] += Vdot(Ai[0], Ai[1]);
    /* C[1][0] = C[0][1] (this is assigned outside the loop)  */
    C[1][1] += Vdot(Ai[1], Ai[1]);

    /* Now the right-hand side of the equation in the paper.  */
    temp0 = Vmult_scalar(start_vector, B0(CURVE_T(curve, i)));
    temp1 = Vmult_scalar(start_vector, B1(CURVE_T(curve, i)));
    temp2 = Vmult_scalar(end_vector, B2(CURVE_T(curve, i)));
    temp3 = Vmult_scalar(end_vector, B3(CURVE_T(curve, i)));

    temp = make_vector(Vsubtract_point(CURVE_POINT(curve, i), Vadd(temp0, Vadd(temp1, Vadd(temp2, temp3)))));

    X[0] += Vdot(temp, Ai[0]);
    X[1] += Vdot(temp, Ai[1]);
  }
  free(A);

  C[1][0] = C[0][1];

  X_C1_det = X[0] * C[1][1] - X[1] * C[0][1];
  C0_X_det = C[0][0] * X[1] - C[0][1] * X[0];
  C0_C1_det = C[0][0] * C[1][1] - C[1][0] * C[0][1];
  if (C0_C1_det == 0.0) {
    /* Zero determinant */
    alpha1 = 0;
    alpha2 = 0;
  } else {
    alpha1 = X_C1_det / C0_C1_det;
    alpha2 = C0_X_det / C0_C1_det;
  }
  CONTROL1(spline) = Vadd_point(START_POINT(spline), Vmult_scalar(t1_hat, alpha1));
  CONTROL2(spline) = Vadd_point(END_POINT(spline), Vmult_scalar(t2_hat, alpha2));
  SPLINE_DEGREE(spline) = CUBICTYPE;

  return spline;
}

/* Find reasonable values for t for each point on CURVE.  The method is
   called chord-length parameterization, which is described in Plass &
   Stone.  The basic idea is just to use the distance from one point to
   the next as the t value, normalized to produce values that increase
   from zero for the first point to one for the last point.  */

static void set_initial_parameter_values(curve_type curve)
{
  unsigned p;

  LOG("\nAssigning initial t values:\n  ");

  CURVE_T(curve, 0) = 0.0;

  for (p = 1; p < CURVE_LENGTH(curve); p++) {
    at_real_coord point = CURVE_POINT(curve, p), previous_p = CURVE_POINT(curve, p - 1);
    gfloat d = distance(point, previous_p);
    CURVE_T(curve, p) = CURVE_T(curve, p - 1) + d;
  }

  if (LAST_CURVE_T(curve) == 0.0)
    LAST_CURVE_T(curve) = 1.0;

  for (p = 1; p < CURVE_LENGTH(curve); p++)
    CURVE_T(curve, p) = CURVE_T(curve, p) / LAST_CURVE_T(curve);

  if (logging)
    log_entire_curve(curve);
}

/* Find an approximation to the tangent to an endpoint of CURVE (to the
   first point if TO_START_POINT is TRUE, else the last).  If
   CROSS_CURVE is TRUE, consider points on the adjacent curve to CURVE.

   It is important to compute an accurate approximation, because the
   control points that we eventually decide upon to fit the curve will
   be placed on the half-lines defined by the tangents and
   endpoints...and we never recompute the tangent after this.  */

static void find_tangent(curve_type curve, gboolean to_start_point, gboolean cross_curve, unsigned tangent_surround)
{
  vector_type tangent;
  vector_type **curve_tangent = (to_start_point == TRUE) ? &(CURVE_START_TANGENT(curve))
      : &(CURVE_END_TANGENT(curve));
  unsigned n_points = 0;

  LOG("  tangent to %s: ", (to_start_point == TRUE) ? "start" : "end");

  if (*curve_tangent == NULL) {
    XMALLOC(*curve_tangent, sizeof(vector_type));
    do {
      tangent = find_half_tangent(curve, to_start_point, &n_points, tangent_surround);

      if ((cross_curve == TRUE) || (CURVE_CYCLIC(curve) == TRUE)) {
        curve_type adjacent_curve = (to_start_point == TRUE) ? PREVIOUS_CURVE(curve) : NEXT_CURVE(curve);
        vector_type tangent2 = (to_start_point == FALSE) ? find_half_tangent(adjacent_curve, TRUE, &n_points,
                                                                             tangent_surround) : find_half_tangent(adjacent_curve, TRUE, &n_points,
                                                                                                                   tangent_surround);

        LOG("(adjacent curve half tangent (%.3f,%.3f,%.3f)) ", tangent2.dx, tangent2.dy, tangent2.dz);
        tangent = Vadd(tangent, tangent2);
      }
      tangent_surround--;

    }
    while (tangent.dx == 0.0 && tangent.dy == 0.0);

    assert(n_points > 0);
    **curve_tangent = Vmult_scalar(tangent, (gfloat) (1.0 / n_points));
    if ((CURVE_CYCLIC(curve) == TRUE) && CURVE_START_TANGENT(curve))
      *CURVE_START_TANGENT(curve) = **curve_tangent;
    if ((CURVE_CYCLIC(curve) == TRUE) && CURVE_END_TANGENT(curve))
      *CURVE_END_TANGENT(curve) = **curve_tangent;
  } else
    LOG("(already computed) ");

  LOG("(%.3f,%.3f,%.3f).\n", (*curve_tangent)->dx, (*curve_tangent)->dy, (*curve_tangent)->dz);
}

/* Find the change in y and change in x for `tangent_surround' (a global)
   points along CURVE.  Increment N_POINTS by the number of points we
   actually look at.  */

static vector_type find_half_tangent(curve_type c, gboolean to_start_point, unsigned *n_points, unsigned tangent_surround)
{
  unsigned p;
  int factor = to_start_point ? 1 : -1;
  unsigned tangent_index = to_start_point ? 0 : c->length - 1;
  at_real_coord tangent_point = CURVE_POINT(c, tangent_index);
  vector_type tangent = { 0.0, 0.0 };
  unsigned int surround;

  if ((surround = CURVE_LENGTH(c) / 2) > tangent_surround)
    surround = tangent_surround;

  for (p = 1; p <= surround; p++) {
    int this_index = p * factor + tangent_index;
    at_real_coord this_point;

    if (this_index < 0 || this_index >= (int)c->length)
      break;

    this_point = CURVE_POINT(c, p * factor + tangent_index);

    /* Perhaps we should weight the tangent from `this_point' by some
       factor dependent on the distance from the tangent point.  */
    tangent = Vadd(tangent, Vmult_scalar(Psubtract(this_point, tangent_point), (gfloat) factor));
    (*n_points)++;
  }

  return tangent;
}

/* When this routine is called, we have computed a spline representation
   for the digitized curve.  The question is, how good is it?  If the
   fit is very good indeed, we might have an error of zero on each
   point, and then WORST_POINT becomes irrelevant.  But normally, we
   return the error at the worst point, and the index of that point in
   WORST_POINT.  The error computation itself is the Euclidean distance
   from the original curve CURVE to the fitted spline SPLINE.  */

static gfloat find_error(curve_type curve, spline_type spline, unsigned *worst_point, at_exception_type * exception)
{
  unsigned this_point;
  gfloat total_error = 0.0;
  gfloat worst_error = FLT_MIN;

  *worst_point = CURVE_LENGTH(curve) + 1; /* A sentinel value.  */

  for (this_point = 0; this_point < CURVE_LENGTH(curve); this_point++) {
    at_real_coord curve_point = CURVE_POINT(curve, this_point);
    gfloat t = CURVE_T(curve, this_point);
    at_real_coord spline_point = evaluate_spline(spline, t);
    gfloat this_error = distance(curve_point, spline_point);
    if (this_error >= worst_error) {
      *worst_point = this_point;
      worst_error = this_error;
    }
    total_error += this_error;
  }

  if (*worst_point == CURVE_LENGTH(curve) + 1) {  /* Didn't have any ``worst point''; the error should be zero.  */
    if (epsilon_equal(total_error, 0.0))
      LOG("  Every point fit perfectly.\n");
    else {
      LOG("No worst point found; something is wrong");
      at_exception_warning(exception, "No worst point found; something is wrong");
    }
  } else {
    if (epsilon_equal(total_error, 0.0))
      LOG("  Every point fit perfectly.\n");
    else {
      LOG("  Worst error (at (%.3f,%.3f,%.3f), point #%u) was %.3f.\n", CURVE_POINT(curve, *worst_point).x, CURVE_POINT(curve, *worst_point).y, CURVE_POINT(curve, *worst_point).z, *worst_point, worst_error);
      LOG("  Total error was %.3f.\n", total_error);
      LOG("  Average error (over %u points) was %.3f.\n", CURVE_LENGTH(curve), total_error / CURVE_LENGTH(curve));
    }
  }

  return worst_error;
}

/* Supposing that we have accepted the error, another question arises:
   would we be better off just using a straight line?  */

static gboolean spline_linear_enough(spline_type * spline, curve_type curve, fitting_opts_type * fitting_opts)
{
  gfloat A, B, C;
  unsigned this_point;
  gfloat dist = 0.0, start_end_dist, threshold;

  LOG("Checking linearity:\n");

  A = END_POINT(*spline).x - START_POINT(*spline).x;
  B = END_POINT(*spline).y - START_POINT(*spline).y;
  C = END_POINT(*spline).z - START_POINT(*spline).z;

  start_end_dist = (gfloat) (SQUARE(A) + SQUARE(B) + SQUARE(C));
  LOG("start_end_distance is %.3f.\n", sqrt(start_end_dist));

  LOG("  Line endpoints are (%.3f, %.3f, %.3f) and ", START_POINT(*spline).x, START_POINT(*spline).y, START_POINT(*spline).z);
  LOG("(%.3f, %.3f, %.3f)\n", END_POINT(*spline).x, END_POINT(*spline).y, END_POINT(*spline).z);

  /* LOG ("  Line is %.3fx + %.3fy + %.3f = 0.\n", A, B, C); */

  for (this_point = 0; this_point < CURVE_LENGTH(curve); this_point++) {
    gfloat a, b, c, w;
    gfloat t = CURVE_T(curve, this_point);
    at_real_coord spline_point = evaluate_spline(*spline, t);

    a = spline_point.x - START_POINT(*spline).x;
    b = spline_point.y - START_POINT(*spline).y;
    c = spline_point.z - START_POINT(*spline).z;
    w = (A * a + B * b + C * c) / start_end_dist;

    dist += (gfloat) sqrt(SQUARE(a - A * w) + SQUARE(b - B * w) + SQUARE(c - C * w));
  }
  LOG("  Total distance is %.3f, ", dist);

  dist /= (CURVE_LENGTH(curve) - 1);
  LOG("which is %.3f normalized.\n", dist);

  /* We want reversion of short curves to splines to be more likely than
     reversion of long curves, hence the second division by the curve
     length, for use in `change_bad_lines'.  */
  SPLINE_LINEARITY(*spline) = dist;
  LOG("  Final linearity: %.3f.\n", SPLINE_LINEARITY(*spline));
  if (start_end_dist * (gfloat) 0.5 > fitting_opts->line_threshold)
    threshold = fitting_opts->line_threshold;
  else
    threshold = start_end_dist * (gfloat) 0.5;
  LOG("threshold is %.3f .\n", threshold);
  if (dist < threshold)
    return TRUE;
  else
    return FALSE;
}

/* Unfortunately, we cannot tell in isolation whether a given spline
   should be changed to a line or not.  That can only be known after the
   entire curve has been fit to a list of splines.  (The curve is the
   pixel outline between two corners.)  After subdividing the curve, a
   line may very well fit a portion of the curve just as well as the
   spline---but unless a spline is truly close to being a line, it
   should not be combined with other lines.  */

static void change_bad_lines(spline_list_type * spline_list, fitting_opts_type * fitting_opts)
{
  unsigned this_spline;
  gboolean found_cubic = FALSE;
  unsigned length = SPLINE_LIST_LENGTH(*spline_list);

  LOG("\nChecking for bad lines (length %u):\n", length);

  /* First see if there are any splines in the fitted shape.  */
  for (this_spline = 0; this_spline < length; this_spline++) {
    if (SPLINE_DEGREE(SPLINE_LIST_ELT(*spline_list, this_spline)) == CUBICTYPE) {
      found_cubic = TRUE;
      break;
    }
  }

  /* If so, change lines back to splines (we haven't done anything to
     their control points, so we only have to change the degree) unless
     the spline is close enough to being a line.  */
  if (found_cubic)
    for (this_spline = 0; this_spline < length; this_spline++) {
      spline_type s = SPLINE_LIST_ELT(*spline_list, this_spline);

      if (SPLINE_DEGREE(s) == LINEARTYPE) {
        LOG("  #%u: ", this_spline);
        if (SPLINE_LINEARITY(s) > fitting_opts->line_reversion_threshold) {
          LOG("reverted, ");
          SPLINE_DEGREE(SPLINE_LIST_ELT(*spline_list, this_spline))
              = CUBICTYPE;
        }
        LOG("linearity %.3f.\n", SPLINE_LINEARITY(s));
      }
  } else
    LOG("  No lines.\n");
}

/* Lists of array indices (well, that is what we use it for).  */

static index_list_type new_index_list(void)
{
  index_list_type index_list;

  index_list.data = NULL;
  INDEX_LIST_LENGTH(index_list) = 0;

  return index_list;
}

static void free_index_list(index_list_type * index_list)
{
  if (INDEX_LIST_LENGTH(*index_list) > 0) {
    free(index_list->data);
    index_list->data = NULL;
    INDEX_LIST_LENGTH(*index_list) = 0;
  }
}

static void append_index(index_list_type * list, unsigned new_index)
{
  INDEX_LIST_LENGTH(*list)++;
  XREALLOC(list->data, INDEX_LIST_LENGTH(*list) * sizeof(unsigned));
  list->data[INDEX_LIST_LENGTH(*list) - 1] = new_index;
}

/* Turn an real point into a integer one.  */

static at_coord real_to_int_coord(at_real_coord real_coord)
{
  at_coord int_coord;

  int_coord.x = lround(real_coord.x);
  int_coord.y = lround(real_coord.y);

  return int_coord;
}

/* Return the Euclidean distance between P1 and P2.  */

static gfloat distance(at_real_coord p1, at_real_coord p2)
{
  gfloat x = p1.x - p2.x, y = p1.y - p2.y, z = p1.z - p2.z;
  return (gfloat) sqrt(SQUARE(x)
                       + SQUARE(y) + SQUARE(z));
}
