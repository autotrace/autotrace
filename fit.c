/* fit.c: turn a bitmap representation of a curve into a list of splines.
   Some of the ideas, but not the code, comes from the Phoenix thesis.
   See README for the reference. */

#include "fit.h"
#include "usefull.h"
#include "message.h"
#include "logreport.h"
#include "spline.h"
#include "vector.h"
#include "curve.h"
#include "pxl-outline.h"
#include "epsilon-equal.h"
#include "xmem.h"
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




/* We need to manipulate lists of array indices.  */

typedef struct index_list
{
  unsigned *data;
  unsigned length;
} index_list_type;

/* The usual accessor macros.  */
#define GET_INDEX(i_l, n)  ((i_l).data[n])
#define INDEX_LIST_LENGTH(i_l)  ((i_l).length)
#define GET_LAST_INDEX(i_l)  ((i_l).data[INDEX_LIST_LENGTH (i_l) - 1])

static void append_index (index_list_type *, unsigned);
static void free_index_list (index_list_type *);
static index_list_type new_index_list (void);
static void remove_adjacent_corners (index_list_type *, unsigned, boolean);
static void align (spline_list_type *, fitting_opts_type *);
static void change_bad_lines (spline_list_type *,
  fitting_opts_type *);
static void filter (curve_type, fitting_opts_type *);
static real filter_angle (vector_type, vector_type);
static void find_curve_vectors
  (unsigned, curve_type, unsigned, vector_type *, vector_type *, unsigned *);
static unsigned find_subdivision (curve_type, unsigned initial,
  fitting_opts_type *);
static void find_vectors
  (unsigned, pixel_outline_type, vector_type *, vector_type *, unsigned);
static index_list_type find_corners (pixel_outline_type,
  fitting_opts_type *);
static real find_error (curve_type, spline_type, unsigned *);
static vector_type find_half_tangent (curve_type, boolean start, unsigned *, unsigned);
static void find_tangent (curve_type, boolean, boolean, unsigned);
static spline_type fit_one_spline (curve_type);
static spline_list_type *fit_curve (curve_type,
  fitting_opts_type *);
static spline_list_type fit_curve_list (curve_list_type,
  fitting_opts_type *);
static spline_list_type *fit_with_least_squares (curve_type,
  fitting_opts_type *);
static spline_list_type *fit_with_line (curve_type);
static void remove_knee_points (curve_type, boolean);
static boolean reparameterize (curve_type, spline_type);
static void set_initial_parameter_values (curve_type);
static boolean spline_linear_enough (spline_type *, curve_type,
  fitting_opts_type *);
static curve_list_array_type split_at_corners (
  pixel_outline_list_type, fitting_opts_type *);
static boolean test_subdivision_point (curve_type, unsigned,
  vector_type *, fitting_opts_type *);
static coordinate_type real_to_int_coord (real_coordinate_type);
static const real distance (real_coordinate_type, real_coordinate_type);

/* Get a new set of fitting options */
#ifdef _EXPORTING
__declspec(dllexport) fitting_opts_type
__stdcall new_fitting_opts (void)
#else
fitting_opts_type new_fitting_opts (void)
#endif
{
  fitting_opts_type fitting_opts;
/* If two endpoints are closer than this, they are made to be equal.
   (-align-threshold)  */
  fitting_opts.align_threshold = 0.5;

/* To how many colors the bitmap is reduced */
  fitting_opts.color_count = 0;

/* If the angle defined by a point and its predecessors and successors
   is smaller than this, it's a corner, even if it's within
   `corner_surround' pixels of a point with a smaller angle.
   (-corner-always-threshold)  */
  fitting_opts.corner_always_threshold = 60.0;

/* Number of points to consider when determining if a point is a corner
   or not.  (-corner-surround)  */
  fitting_opts.corner_surround = 4;

/* If a point, its predecessors, and its successors define an angle
    smaller than this, it's a corner.  Should be in range 0..180.
   (-corner-threshold)  */
  fitting_opts.corner_threshold = 100.0;

/* Amount of error at which a fitted spline is unacceptable.  If any
   pixel is further away than this from the fitted curve, we try again.
   (-error-threshold) */
  fitting_opts.error_threshold = .8;

/* A second number of adjacent points to consider when filtering.
   (-filter-alternative-surround)  */
  fitting_opts.filter_alternative_surround = 1;

/* If the angles between the vectors produced by filter_surround and
   filter_alternative_surround points differ by more than this, use
   the one from filter_alternative_surround.  (-filter-epsilon)  */
  fitting_opts.filter_epsilon = 10.0;

/* Number of times to smooth original data points.  Increasing this
   number dramatically---to 50 or so---can produce vastly better
   results.  But if any points that ``should'' be corners aren't found,
   the curve goes to hell around that point.  (-filter-iterations)  */
  fitting_opts.filter_iteration_count = 4;

/* To produce the new point, use the old point plus this times the
   neighbors.  (-filter-percent)  */
  fitting_opts.filter_percent = .33;

/* Number of adjacent points to consider if `filter_surround' points
   defines a straight line.  (-filter-secondary-surround)  */
  fitting_opts.filter_secondary_surround = 3;

/* Number of adjacent points to consider when filtering.
  (-filter-surround)  */
  fitting_opts.filter_surround = 2;

/* If a spline is closer to a straight line than this, it remains a
   straight line, even if it would otherwise be changed back to a curve.
   This is weighted by the square of the curve length, to make shorter
   curves more likely to be reverted.  (-line-reversion-threshold)  */
  fitting_opts.line_reversion_threshold = .01;

/* How many pixels (on the average) a spline can diverge from the line
   determined by its endpoints before it is changed to a straight line.
   (-line-threshold) */
  fitting_opts.line_threshold = 1.0;

/* Should adjacent corners be removed?  */
  fitting_opts.remove_adj_corners = false;

/* If reparameterization doesn't improve the fit by this much percent,
   stop doing it.  (-reparameterize-improve)  */
  fitting_opts.reparameterize_improvement = .10;

/* Amount of error at which it is pointless to reparameterize.  This
   happens, for example, when we are trying to fit the outline of the
   outside of an `O' with a single spline.  The initial fit is not good
   enough for the Newton-Raphson iteration to improve it.  It may be
   that it would be better to detect the cases where we didn't find any
   corners.  (-reparameterize-threshold)  */
  fitting_opts.reparameterize_threshold = 30.0;

/* Percentage of the curve away from the worst point to look for a
   better place to subdivide.  (-subdivide-search)  */
  fitting_opts.subdivide_search = .1;

/* Number of points to consider when deciding whether a given point is a
   better place to subdivide.  (-subdivide-surround)  */
  fitting_opts.subdivide_surround = 4;

/* How many pixels a point can diverge from a straight line and still be
   considered a better place to subdivide.  (-subdivide-threshold) */
  fitting_opts.subdivide_threshold = .03;

/* Number of points to look at on either side of a point when computing
   the approximation to the tangent at that point.  (-tangent-surround)  */
  fitting_opts.tangent_surround = 3;

  return (fitting_opts);
}



/* The top-level call that transforms the list of pixels in the outlines
   of the original character to a list of spline lists fitted to those
   pixels.  */

#ifdef _EXPORTING
__declspec(dllexport) spline_list_array_type
__stdcall fitted_splines (pixel_outline_list_type pixel_outline_list,
  fitting_opts_type *fitting_opts)
#else
spline_list_array_type
fitted_splines (pixel_outline_list_type pixel_outline_list,
  fitting_opts_type *fitting_opts)
#endif
{
  unsigned this_list;
  unsigned total = 0;
  spline_list_array_type char_splines = new_spline_list_array ();
  curve_list_array_type curve_array = split_at_corners (pixel_outline_list, fitting_opts);

  for (this_list = 0; this_list < CURVE_LIST_ARRAY_LENGTH (curve_array);
       this_list++)
    {
      spline_list_type curve_list_splines;
      curve_list_type curves = CURVE_LIST_ARRAY_ELT (curve_array, this_list);

      LOG1 ("\nFitting curve list #%u:\n", this_list);

      curve_list_splines = fit_curve_list (curves, fitting_opts);
      curve_list_splines.clockwise = curves.clockwise;

      memcpy (&(curve_list_splines.color),
        &(O_LIST_OUTLINE(pixel_outline_list, this_list).color),
        sizeof (color_type));
      append_spline_list (&char_splines, curve_list_splines);
    }

  free_curve_list_array (&curve_array);

  for (this_list = 0; this_list < SPLINE_LIST_ARRAY_LENGTH (char_splines);
       this_list++)
    total
     += SPLINE_LIST_LENGTH (SPLINE_LIST_ARRAY_ELT (char_splines, this_list));

  flush_log_output ();

  return char_splines;
}


/* Fit the list of curves CURVE_LIST to a list of splines, and return
   it.  CURVE_LIST represents a single closed paths, e.g., either the
   inside or outside outline of an `o'.  */

static spline_list_type
fit_curve_list (curve_list_type curve_list,
  fitting_opts_type *fitting_opts)
{
  curve_type curve;
  unsigned this_curve, this_spline;
  unsigned curve_list_length = CURVE_LIST_LENGTH (curve_list);
  spline_list_type *temp = new_spline_list ();
  spline_list_type curve_list_splines = *temp;
  
  free (temp);

  /* Remove the extraneous ``knee'' points before filtering.  Since the
     corners have already been found, we don't need to worry about
     removing a point that should be a corner.  */

  LOG ("\nRemoving knees:\n");
  for (this_curve = 0; this_curve < curve_list_length; this_curve++)
    {
      LOG1 ("#%u:", this_curve);
      remove_knee_points (CURVE_LIST_ELT (curve_list, this_curve),
                          CURVE_LIST_CLOCKWISE (curve_list));
    }

  /* We filter all the curves in CURVE_LIST at once; otherwise, we would
     look at an unfiltered curve when computing tangents.  */

  LOG ("\nFiltering curves:\n");
  for (this_curve = 0; this_curve < curve_list.length; this_curve++)
    {
      LOG1 ("#%u: ", this_curve);
      filter (CURVE_LIST_ELT (curve_list, this_curve), fitting_opts);
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
  curve = CURVE_LIST_ELT (curve_list, 0);
  if (CURVE_CYCLIC (curve) && CURVE_LENGTH (curve) != 3)
    append_point (curve, CURVE_POINT (curve, 0));

  /* Finally, fit each curve in the list to a list of splines.  */
  for (this_curve = 0; this_curve < curve_list_length; this_curve++)
    {
      spline_list_type *curve_splines;
      curve_type current_curve = CURVE_LIST_ELT (curve_list, this_curve);

      LOG1 ("\nFitting curve #%u:\n", this_curve);

      curve_splines = fit_curve (current_curve, fitting_opts);
      if (curve_splines == NULL)
        WARNING1 ("Could not fit curve #%u", this_curve);
      else
        {
          LOG1 ("Fitted splines for curve #%u:\n", this_curve);
          for (this_spline = 0;
               this_spline < SPLINE_LIST_LENGTH (*curve_splines);
               this_spline++)
            {
              LOG1 ("  %u: ", this_spline);
              if (log_file)
                print_spline (log_file,
                              SPLINE_LIST_ELT (*curve_splines, this_spline));
            }

          /* After fitting, we may need to change some would-be lines
             back to curves, because they are in a list with other
             curves.  */
          change_bad_lines (curve_splines, fitting_opts);

          concat_spline_lists (&curve_list_splines, *curve_splines);
	  free_spline_list (*curve_splines);
	  free (curve_splines);
        }
    }

  if (log_file)
    {
      LOG ("\nFitted splines are:\n");
      for (this_spline = 0; this_spline < SPLINE_LIST_LENGTH (curve_list_splines);
           this_spline++)
        {
          LOG1 ("  %u: ", this_spline);
          print_spline (log_file, SPLINE_LIST_ELT (curve_list_splines,
                                                   this_spline));
        }
    }

  /* We do this for each outline's spline list because when a point
     is changed, it needs to be changed in both segments in which it
     appears---and the segments might be in different curves.  */
  align (&curve_list_splines, fitting_opts);

  return curve_list_splines;
}


/* Transform a set of locations to a list of splines (the fewer the
   better).  We are guaranteed that CURVE does not contain any corners.
   We return NULL if we cannot fit the points at all.  */

static spline_list_type *
fit_curve (curve_type curve, fitting_opts_type *fitting_opts)
{
  spline_list_type *fitted_splines;

  if (CURVE_LENGTH (curve) < 2)
    {
      WARNING ("Tried to fit curve with less than two points");
      return NULL;
    }

  /* Do we have enough points to fit with a spline?  */
  fitted_splines = CURVE_LENGTH (curve) < 4
                   ? fit_with_line (curve)
                   : fit_with_least_squares (curve, fitting_opts);

  return fitted_splines;
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

static curve_list_array_type
split_at_corners (pixel_outline_list_type pixel_list, fitting_opts_type *fitting_opts)
{
  unsigned this_pixel_o;
  curve_list_array_type curve_array = new_curve_list_array ();

  LOG ("\nFinding corners:\n");

  for (this_pixel_o = 0; this_pixel_o < O_LIST_LENGTH (pixel_list);
       this_pixel_o++)
    {
      curve_type curve, first_curve;
      index_list_type corner_list;
      unsigned p, this_corner;
      curve_list_type curve_list = new_curve_list ();
      pixel_outline_type pixel_o = O_LIST_OUTLINE (pixel_list, this_pixel_o);

      CURVE_LIST_CLOCKWISE (curve_list) = O_CLOCKWISE (pixel_o);

      LOG1 ("#%u:", this_pixel_o);

      /* If the outline does not have enough points, we can't do
         anything.  The endpoints of the outlines are automatically
         corners.  We need at least `corner_surround' more pixels on
         either side of a point before it is conceivable that we might
         want another corner.  */
      if (O_LENGTH (pixel_o) > fitting_opts->corner_surround * 2 + 2)
        corner_list = find_corners (pixel_o, fitting_opts);
      else {
        unsigned int surround;
        if ((surround = (O_LENGTH (pixel_o) - 3) / 2) >= 4)
          {
            unsigned save_corner_surround = fitting_opts->corner_surround;
            fitting_opts->corner_surround = surround;
            corner_list = find_corners (pixel_o, fitting_opts);
            fitting_opts->corner_surround = save_corner_surround;
           }
         else
	   {
             corner_list.length = 0;
	     corner_list.data = NULL;
	   }
      }

      /* Remember the first curve so we can make it be the `next' of the
         last one.  (And vice versa.)  */
      first_curve = new_curve ();

      curve = first_curve;

      if (corner_list.length == 0)
        { /* No corners.  Use all of the pixel outline as the curve.  */
          for (p = 0; p < O_LENGTH (pixel_o); p++)
            append_pixel (curve, O_COORDINATE (pixel_o, p));

          /* This curve is cyclic.  */
          CURVE_CYCLIC (curve) = true;
        }
      else
        { /* Each curve consists of the points between (inclusive) each pair
             of corners.  */
          for (this_corner = 0; this_corner < corner_list.length - 1;
               this_corner++)
            {
              curve_type previous_curve = curve;
              unsigned corner = GET_INDEX (corner_list, this_corner);
              unsigned next_corner = GET_INDEX (corner_list, this_corner + 1);

              for (p = corner; p <= next_corner; p++)
                append_pixel (curve, O_COORDINATE (pixel_o, p));

              append_curve (&curve_list, curve);
              curve = new_curve ();
              NEXT_CURVE (previous_curve) = curve;
              PREVIOUS_CURVE (curve) = previous_curve;
            }

          /* The last curve is different.  It consists of the points
             (inclusive) between the last corner and the end of the list,
             and the beginning of the list and the first corner.  */
          for (p = GET_LAST_INDEX (corner_list); p < O_LENGTH (pixel_o);
               p++)
            append_pixel (curve, O_COORDINATE (pixel_o, p));

          for (p = 0; p <= GET_INDEX (corner_list, 0); p++)
            append_pixel (curve, O_COORDINATE (pixel_o, p));
        }

      LOG1 (" [%u].\n", corner_list.length);
      free_index_list (&corner_list);

      /* Add `curve' to the end of the list, updating the pointers in
         the chain.  */
      append_curve (&curve_list, curve);
      NEXT_CURVE (curve) = first_curve;
      PREVIOUS_CURVE (first_curve) = curve;

      /* And now add the just-completed curve list to the array.  */
      append_curve_list (&curve_array, curve_list);
    }				/* End of considering each pixel outline.  */

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

#define APPEND_CORNER(index, angle, c)									\
  do																	\
    {																	\
      append_index (&corner_list, index);								\
      LOG4 (" (%d,%d)%c%.3f",											\
            O_COORDINATE (pixel_outline, index).x,						\
            O_COORDINATE (pixel_outline, index).y,						\
            c, angle);													\
    }																	\
  while (0)

static index_list_type
find_corners (pixel_outline_type pixel_outline,
  fitting_opts_type *fitting_opts)
{
  unsigned p;
  index_list_type corner_list = new_index_list ();

  /* Consider each pixel on the outline in turn.  */
  for (p = 0; p < O_LENGTH (pixel_outline); p++)
    {
      real corner_angle;
      vector_type in_vector, out_vector;

      /* Check if the angle is small enough.  */
      find_vectors (p, pixel_outline, &in_vector, &out_vector,
	    fitting_opts->corner_surround);
      corner_angle = Vangle (in_vector, out_vector);

      if (fabs (corner_angle) <= fitting_opts->corner_threshold)
        {
          /* We want to keep looking, instead of just appending the
             first pixel we find with a small enough angle, since there
             might be another corner within `corner_surround' pixels, with
             a smaller angle.  If that is the case, we want that one.  */
          real best_corner_angle = corner_angle;
          unsigned best_corner_index = p;
          index_list_type equally_good_list = new_index_list ();
          /* As we come into the loop, `p' is the index of the point
             that has an angle less than `corner_angle'.  We use `i' to
             move through the pixels next to that, and `q' for moving
             through the adjacent pixels to each `p'.  */
          unsigned q = p;
          unsigned i = p + 1;

          while (true)
            {
              /* Perhaps the angle is sufficiently small that we want to
                 consider this a corner, even if it's not the best
                 (unless we've already wrapped around in the search,
                 i.e., `q<i', in which case we have already added the
                 corner, and we don't want to add it again).  We want to
                 do this check on the first candidate we find, as well
                 as the others in the loop, hence this comes before the
                 stopping condition.  */
              if (corner_angle <= fitting_opts->corner_always_threshold && q >= p)
                APPEND_CORNER (q, corner_angle, '\\');

              /* Exit the loop if we've looked at `corner_surround'
                 pixels past the best one we found, or if we've looked
                 at all the pixels.  */
              if (i >= best_corner_index + fitting_opts->corner_surround
                  || i >= O_LENGTH (pixel_outline))
                break;

              /* Check the angle.  */
              q = i % O_LENGTH (pixel_outline);
              find_vectors (q, pixel_outline, &in_vector, &out_vector,
			    fitting_opts->corner_surround);
              corner_angle = Vangle (in_vector, out_vector);

              /* If we come across a corner that is just as good as the
                 best one, we should make it a corner, too.  This
                 happens, for example, at the points on the `W' in some
                 typefaces, where the ``points'' are flat.  */
              if (epsilon_equal (corner_angle, best_corner_angle))
                append_index (&equally_good_list, q);

              else if (corner_angle < best_corner_angle)
                {
                  best_corner_angle = corner_angle;
                  /* We want to check `corner_surround' pixels beyond the
                     new best corner.  */
                  i = best_corner_index = q;
                  free_index_list (&equally_good_list);
                  equally_good_list = new_index_list ();
                }

              i++;
            }

          /* After we exit the loop, `q' is the index of the last point
             we checked.  We have already added the corner if
             `best_corner_angle' is less than `corner_always_threshold'.
             Again, if we've already wrapped around, we don't want to
             add the corner again.  */
          if (best_corner_angle > fitting_opts->corner_always_threshold
              && best_corner_index >= p)
            {
              unsigned i;

              APPEND_CORNER (best_corner_index, best_corner_angle, '/');

              for (i = 0; i < INDEX_LIST_LENGTH (equally_good_list); i++)
                APPEND_CORNER (GET_INDEX (equally_good_list, i),
                               best_corner_angle, '@');
	    }
          free_index_list (&equally_good_list);

          /* If we wrapped around in our search, we're done; otherwise,
             we don't want the outer loop to look at the pixels that we
             already looked at in searching for the best corner.  */
          p = (q < p) ? O_LENGTH (pixel_outline) : q;
        }			/* End of searching for the best corner.  */
    }				/* End of considering each pixel.  */

  if (INDEX_LIST_LENGTH (corner_list) > 0)
    /* We never want two corners next to each other, since the
       only way to fit such a ``curve'' would be with a straight
       line, which usually interrupts the continuity dreadfully.  */
    remove_adjacent_corners (&corner_list, O_LENGTH (pixel_outline) - 1,
      fitting_opts->remove_adj_corners);
  return corner_list;
}


/* Return the difference vectors coming in and going out of the outline
   OUTLINE at the point whose index is TEST_INDEX.  In Phoenix,
   Schneider looks at a single point on either side of the point we're
   considering.  That works for him because his points are not touching.
   But our points *are* touching, and so we have to look at
   `corner_surround' points on either side, to get a better picture of
   the outline's shape.  */

static void
find_vectors (unsigned test_index, pixel_outline_type outline,
              vector_type *in, vector_type *out,
			  unsigned corner_surround)
{
  int i;
  unsigned n_done;
  coordinate_type candidate = O_COORDINATE (outline, test_index);

  in->dx = 0.0;
  in->dy = 0.0;
  out->dx = 0.0;
  out->dy = 0.0;

  /* Add up the differences from p of the `corner_surround' points
     before p.  */
  for (i = O_PREV (outline, test_index), n_done = 0; n_done
   < corner_surround;
       i = O_PREV (outline, i), n_done++)
    *in = Vadd (*in, IPsubtract (O_COORDINATE (outline, i), candidate));

  /* And the points after p.  */
  for (i = O_NEXT (outline, test_index), n_done = 0; n_done <
    corner_surround; i = O_NEXT (outline, i), n_done++)
    *out = Vadd (*out, IPsubtract (O_COORDINATE (outline, i), candidate));
}


/* Remove adjacent points from the index list LIST.  We do this by first
   sorting the list and then running through it.  Since these lists are
   quite short, a straight selection sort (e.g., p.139 of the Art of
   Computer Programming, vol.3) is good enough.  LAST_INDEX is the index
   of the last pixel on the outline, i.e., the next one is the first
   pixel. We need this for checking the adjacency of the last corner.

   We need to do this because the adjacent corners turn into
   two-pixel-long curves, which can only be fit by straight lines.  */

static void
remove_adjacent_corners (index_list_type *list, unsigned last_index,
  boolean remove_adj_corners)
{
  unsigned j;
  unsigned last;
  index_list_type new = new_index_list ();

  for (j = INDEX_LIST_LENGTH (*list) - 1; j > 0; j--)
    {
      unsigned search;
      unsigned temp;
      /* Find maximal element below `j'.  */
      unsigned max_index = j;

      for (search = 0; search < j; search++)
        if (GET_INDEX (*list, search) > GET_INDEX (*list, max_index))
          max_index = search;

      if (max_index != j)
        {
          temp = GET_INDEX (*list, j);
          GET_INDEX (*list, j) = GET_INDEX (*list, max_index);
          GET_INDEX (*list, max_index) = temp;
          WARNING ("needed exchange"); /* xx -- really have to sort?  */
        }
    }

  /* The list is sorted.  Now look for adjacent entries.  Each time
     through the loop we insert the current entry and, if appropriate,
     the next entry.  */
  for (j = 0; j < INDEX_LIST_LENGTH (*list) - 1; j++)
    {
      unsigned current = GET_INDEX (*list, j);
      unsigned next = GET_INDEX (*list, j + 1);

      /* We should never have inserted the same element twice.  */
      MYASSERT (current != next);

      if ((remove_adj_corners) && (next == current + 1))
        j++;

      append_index (&new, current);
    }

  /* Don't append the last element if it is 1) adjacent to the previous
     one; or 2) adjacent to the very first one.  */
  last = GET_LAST_INDEX (*list);
  if (INDEX_LIST_LENGTH (new) == 0
      || !(last == GET_LAST_INDEX (new) + 1
           || (last == last_index && GET_INDEX (*list, 0) == 0)))
    append_index (&new, last);

  free_index_list (list);
  *list = new;
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

/* This evaluates to true if the vector V is zero in one direction and
   nonzero in the other.  */
#define ONLY_ONE_ZERO(v)						\
  (((v).dx == 0.0 && (v).dy != 0.0) || ((v).dy == 0.0 && (v).dx != 0.0))


/* There are four possible cases for knees, one for each of the four
   corners of a rectangle; and then the cases differ depending on which
   direction we are going around the curve.  The tests are listed here
   in the order of upper left, upper right, lower right, lower left.
   Perhaps there is some simple pattern to the
   clockwise/counterclockwise differences, but I don't see one.  */
#define CLOCKWISE_KNEE(prev_delta, next_delta)							\
  ((prev_delta.dx == -1.0 && next_delta.dy == 1.0)						\
   || (prev_delta.dy == 1.0 && next_delta.dx == 1.0)					\
   || (prev_delta.dx == 1.0 && next_delta.dy == -1.0)					\
   || (prev_delta.dy == -1.0 && next_delta.dx == -1.0))

#define COUNTERCLOCKWISE_KNEE(prev_delta, next_delta)					\
  ((prev_delta.dy == 1.0 && next_delta.dx == -1.0)						\
   || (prev_delta.dx == 1.0 && next_delta.dy == 1.0)					\
   || (prev_delta.dy == -1.0 && next_delta.dx == 1.0)					\
   || (prev_delta.dx == -1.0 && next_delta.dy == -1.0))

static void
remove_knee_points (curve_type curve, boolean clockwise)
{
  unsigned i;
  unsigned offset = CURVE_CYCLIC (curve) ? 0 : 1;
  coordinate_type previous
    = real_to_int_coord (CURVE_POINT (curve, CURVE_PREV (curve, offset)));
  curve_type trimmed_curve = copy_most_of_curve (curve);

  if (!CURVE_CYCLIC (curve))
    append_pixel (trimmed_curve, real_to_int_coord (CURVE_POINT (curve, 0)));

  for (i = offset; i < CURVE_LENGTH (curve) - offset; i++)
    {
      coordinate_type current
        = real_to_int_coord (CURVE_POINT (curve, i));
      coordinate_type next
        = real_to_int_coord (CURVE_POINT (curve, CURVE_NEXT (curve, i)));
      vector_type prev_delta = IPsubtract (previous, current);
      vector_type next_delta = IPsubtract (next, current);

      if (ONLY_ONE_ZERO (prev_delta) && ONLY_ONE_ZERO (next_delta)
          && ((clockwise && CLOCKWISE_KNEE (prev_delta, next_delta))
              || (!clockwise
                  && COUNTERCLOCKWISE_KNEE (prev_delta, next_delta))))
        LOG2 (" (%d,%d)", current.x, current.y);
      else
        {
          previous = current;
          append_pixel (trimmed_curve, current);
        }
    }

  if (!CURVE_CYCLIC (curve))
    append_pixel (trimmed_curve, real_to_int_coord (LAST_CURVE_POINT (curve)));

  if (CURVE_LENGTH (trimmed_curve) == CURVE_LENGTH (curve))
    LOG (" (none)");

  LOG (".\n");

  free_curve (curve);
  *curve = *trimmed_curve;
  free (trimmed_curve);
}


/* Smooth the curve by adding in neighboring points.  Do this
   `filter_iteration_count' times.  But don't change the corners.  */


/* We sometimes need to change the information about the filtered point.
   This macro assigns to the relevant variables.  */
#define FILTER_ASSIGN(new)												\
  do																	\
    {																	\
      in = in_##new;													\
      out = out_##new;													\
      count = new##_count;												\
      angle = angle_##new;												\
    }																	\
  while (0)

static void
filter (curve_type curve, fitting_opts_type *fitting_opts)
{
  unsigned iteration, this_point;
  unsigned offset = CURVE_CYCLIC (curve) ? 0 : 1;

  /* We must have at least three points---the previous one, the current
     one, and the next one.  But if we don't have at least five, we will
     probably collapse the curve down onto a single point, which means
     we won't be able to fit it with a spline.  */
  if (CURVE_LENGTH (curve) < 5)
    {
      LOG1 ("Length is %u, not enough to filter.\n", CURVE_LENGTH (curve));
      return;
    }

  if (fitting_opts->filter_surround > 0
   && fitting_opts->filter_alternative_surround > 0)
    for (iteration = 0; iteration < fitting_opts->filter_iteration_count;
     iteration++)
      {
        curve_type new_curve = copy_most_of_curve (curve);

        /* Keep the first point on the curve.  */
        if (offset)
          append_point (new_curve, CURVE_POINT (curve, 0));

        for (this_point = offset; this_point < CURVE_LENGTH (curve) - offset;
             this_point++)
          {
            real angle, angle_alt;
            vector_type in, in_alt, out, out_alt, sum;
            unsigned count, alt_count;
            real_coordinate_type new_point;

            /* Find the angle using the usual number of surrounding points
             on the curve. */
            find_curve_vectors (this_point, curve, fitting_opts->filter_surround,
                                  &in, &out, &count);
            angle = filter_angle (in, out);

            /* Find the angle using the alternative (presumably smaller)
               number.  */
            find_curve_vectors (this_point, curve,
	        fitting_opts->filter_alternative_surround, &in_alt, &out_alt, &alt_count);
            angle_alt = filter_angle (in_alt, out_alt);

            /* If the alternate angle is enough larger than the usual one
               and neither of the components of the sum are zero, use it.
               (We don't use absolute value here, since if the alternate
               angle is smaller, we don't particularly care, since that
               means the curve is pretty flat around the current point,
               anyway.)  This helps keep small features from disappearing
               into the surrounding curve.  */
            sum = Vadd (in_alt, out_alt);
            if (angle_alt - angle >= fitting_opts->filter_epsilon
                && sum.dx != 0 && sum.dy != 0)
              FILTER_ASSIGN (alt);

            /* Start with the old point.  */
            new_point = CURVE_POINT (curve, this_point);
            sum = Vadd (in, out);
            new_point.x += sum.dx * fitting_opts->filter_percent / count;
            new_point.y += sum.dy * fitting_opts->filter_percent / count;

            /* Put the newly computed point into a separate curve, so it
               doesn't affect future computation (on this iteration).  */
            append_point (new_curve, new_point);
          }

        /* Just as with the first point, we have to keep the last point.  */
        if (offset)
          append_point (new_curve, LAST_CURVE_POINT (curve));

        /* Set the original curve to the newly filtered one, and go again.  */
        free_curve (curve);
        *curve = *new_curve;
	free (new_curve);
      }

  log_curve (curve, false);
}


/* Return the vectors IN and OUT, computed by looking at SURROUND points
   on either side of TEST_INDEX.  Also return the number of points in
   the vectors in COUNT (we make sure they are the same).  */

static void
find_curve_vectors (unsigned test_index, curve_type curve,
                    unsigned surround,
                    vector_type *in, vector_type *out, unsigned *count)
{
  signed int i; /* have to be signed */
  unsigned in_count, out_count;
  unsigned n_done;
  real_coordinate_type candidate = CURVE_POINT (curve, test_index);

  /* Add up the differences from p of the `surround' points
     before p.  */
  in->dx = 0.0;
  in->dy = 0.0;
  for (i = CURVE_PREV (curve, test_index), n_done = 0;
       i >= 0 && n_done < surround;  /* Do not wrap around.  */
       i = CURVE_PREV (curve, i), n_done++)
    *in = Vadd (*in, Psubtract (CURVE_POINT (curve, i), candidate));
  in_count = n_done;

  /* And the points after p.  Don't use more points after p than we
     ended up with before it.  */
  out->dx = 0.0;
  out->dy = 0.0;
  for (i = CURVE_NEXT (curve, test_index), n_done = 0;
       (i < (signed) CURVE_LENGTH (curve)) && (n_done < surround) && (n_done < in_count);
       i = CURVE_NEXT (curve, (unsigned) i), n_done++)
    *out = Vadd (*out, Psubtract (CURVE_POINT (curve, i), candidate));
  out_count = n_done;

  /* If we used more points before p than after p, we have to go back
     and redo it.  (We could just subtract the ones that were missing,
     but for this few of points, efficiency doesn't matter.)  */
  if (out_count < in_count)
    {
      in->dx = 0.0;
      in->dy = 0.0;
      for (i = CURVE_PREV (curve, test_index), n_done = 0;
           i >= 0 && n_done < out_count;
           i = CURVE_PREV (curve, i), n_done++)
        *in = Vadd (*in, Psubtract (CURVE_POINT (curve, i), candidate));
      in_count = n_done;
    }

  MYASSERT (in_count == out_count);
  *count = in_count;
}


/* Find the angle between the vectors IN and OUT, and bring it into the
   range [0,45].  */

static real
filter_angle (vector_type in, vector_type out)
{
  real angle = Vangle (in, out);

  /* What we want to do between 90 and 180 is the same as what we
     want to do between 0 and 90.  */
  angle = fmod (angle, 1990.0);

  /* And what we want to do between 45 and 90 is the same as
     between 0 and 45, only reversed.  */
  if (angle > 45.0) angle = 90.0 - angle;

  return angle;
}


/* This routine returns the curve fitted to a straight line in a very
   simple way: make the first and last points on the curve be the
   endpoints of the line.  This simplicity is justified because we are
   called only on very short curves.  */

static spline_list_type *
fit_with_line (curve_type curve)
{
  spline_type line;

  LOG ("Fitting with straight line:\n");

  SPLINE_DEGREE (line) = LINEARTYPE;
  START_POINT (line) = CONTROL1 (line) = CURVE_POINT (curve, 0);
  END_POINT (line) = CONTROL2 (line) = LAST_CURVE_POINT (curve);

  /* Make sure that this line is never changed to a cubic.  */
  SPLINE_LINEARITY (line) = 0;

  if (log_file)
    {
      LOG ("  ");
      print_spline (log_file, line);
    }

  return init_spline_list (line);
}


/* The least squares method is well described in Schneider's thesis.
   Briefly, we try to fit the entire curve with one spline.  If that fails,
   we try reparameterizing, i.e., finding new, and supposedly better,
   t values.  If that still fails, we subdivide the curve.  */

static spline_list_type *
fit_with_least_squares (curve_type curve, fitting_opts_type *fitting_opts)
{
  real error, best_error;
  spline_type spline, best_spline;
  spline_list_type *spline_list;
  unsigned worst_point;
  unsigned iteration = 0;
  real previous_error = FLT_MAX;
  real improvement = FLT_MAX;

  LOG ("\nFitting with least squares:\n");

  /* Phoenix reduces the number of points with a ``linear spline
     technique''.  But for fitting letterforms, that is
     inappropriate.  We want all the points we can get.  */

  /* It makes no difference whether we first set the `t' values or
     find the tangents.  This order makes the documentation a little
     more coherent.  */

  LOG ("Finding tangents:\n");
  find_tangent (curve, /* to_start */ true,  /* cross_curve */ false,
    fitting_opts->tangent_surround);
  find_tangent (curve, /* to_start */ false, /* cross_curve */ false,
    fitting_opts->tangent_surround);

  set_initial_parameter_values (curve);

  /* Now we loop, reparameterizing and/or subdividing, until CURVE has
     been fit.  */
  while (true)
    {
      LOG ("  fitted to spline:\n");

      spline = fit_one_spline (curve);

      if (log_file)
        {
          LOG ("    ");
          print_spline (log_file, spline);
        }

      error = find_error (curve, spline, &worst_point);
      if (error > previous_error)
        LOG ("Reparameterization made it worse.\n");
        /* Just fall through; we will subdivide.  */
      else
        {
          best_error = error;
          best_spline = spline;
        }

      if (epsilon_equal (error, 0.0))
        break;

      improvement = 1.0 - error / previous_error;

      /* Don't exit, even if the error is less than `error_threshold',
         since we might be able to improve the fit with further
         reparameterization.  If the reparameterization made it worse,
         we will exit here, since `improvement' will be negative.  */
      if (improvement < fitting_opts->reparameterize_improvement
          || error > fitting_opts->reparameterize_threshold)
	break;

      iteration++;
      LOG3 ("Reparameterization #%u (error was %.3f, a %u%% improvement):\n",
            iteration, error, ((unsigned) (improvement * 100.0)));

      /* The reparameterization might fail, if the initial fit was
         better than `reparameterize_threshold', yet worse than the
         Newton-Raphson algorithm could handle.  */
      if (!reparameterize (curve, spline))
	break;

      previous_error = error;
    }

  /* Go back to the best fit.  */
  spline = best_spline;
  error = best_error;

  if (error < fitting_opts->error_threshold)
    {
      /* The points were fitted with a (possibly reparameterized)
         spline.  We end up here whenever a fit is accepted.  We have
         one more job: see if the ``curve'' that was fit should really
         be a straight line. */
      if (spline_linear_enough (&spline, curve, fitting_opts))
        {
          SPLINE_DEGREE (spline) = LINEARTYPE;
          LOG ("Changed to line.\n");
        }
      spline_list = init_spline_list (spline);
      LOG1 ("Accepted error of %.3f.\n", error);
    }
  else
    {
      /* We couldn't fit the curve acceptably, so subdivide.  */
      unsigned subdivision_index;
      spline_list_type *left_spline_list;
      spline_list_type *right_spline_list;
      curve_type left_curve = new_curve ();
      curve_type right_curve = new_curve ();

      /* Keep the linked list of curves intact.  */
      NEXT_CURVE (right_curve) = NEXT_CURVE (curve);
      PREVIOUS_CURVE (right_curve) = left_curve;
      NEXT_CURVE (left_curve) = right_curve;
      PREVIOUS_CURVE (left_curve) = curve;
      NEXT_CURVE (curve) = left_curve;

      LOG1 ("\nSubdividing (error %.3f):\n", error);
      LOG3 ("  Original point: (%.3f,%.3f), #%u.\n",
            CURVE_POINT (curve, worst_point).x,
            CURVE_POINT (curve, worst_point).y, worst_point);
      subdivision_index = find_subdivision (curve, worst_point,
	    fitting_opts);
      LOG3 ("  Final point: (%.3f,%.3f), #%u.\n",
            CURVE_POINT (curve, subdivision_index).x,
            CURVE_POINT (curve, subdivision_index).y, subdivision_index);

      /* The last point of the left-hand curve will also be the first
         point of the right-hand curve.  */
      CURVE_LENGTH (left_curve) = subdivision_index + 1;
      CURVE_LENGTH (right_curve) = CURVE_LENGTH (curve) - subdivision_index;
      left_curve->point_list = curve->point_list;
      right_curve->point_list = curve->point_list + subdivision_index;

      /* We want to use the tangents of the curve which we are
         subdividing for the start tangent for left_curve and the
         end tangent for right_curve.  */
      CURVE_START_TANGENT (left_curve) = CURVE_START_TANGENT (curve);
      CURVE_END_TANGENT (right_curve) = CURVE_END_TANGENT (curve);

      /* We have to set up the two curves before finding the tangent at
         the subdivision point.  The tangent at that point must be the
         same for both curves, or noticeable bumps will occur in the
         character.  But we want to use information on both sides of the
         point to compute the tangent, hence cross_curve = true.  */
      find_tangent (left_curve, /* to_start_point: */ false,
                    /* cross_curve: */ true, fitting_opts->tangent_surround);
      CURVE_START_TANGENT (right_curve) = CURVE_END_TANGENT (left_curve);

      /* Now that we've set up the curves, we can fit them.  */
      left_spline_list = fit_curve (left_curve, fitting_opts);
      right_spline_list = fit_curve (right_curve, fitting_opts);

      /* Neither of the subdivided curves could be fit, so fail.  */
      if (left_spline_list == NULL && right_spline_list == NULL)
        return NULL;

      /* Put the two together (or whichever of them exist).  */
      spline_list = new_spline_list ();

      if (left_spline_list == NULL)
        {
          WARNING ("could not fit left spline list");
          LOG1 ("Could not fit spline to left curve (%lx).\n",
                (unsigned long) left_curve);
        }
      else
        {
          concat_spline_lists (spline_list, *left_spline_list);
	  free_spline_list (*left_spline_list);
 	  free (left_spline_list);
	}

      if (right_spline_list == NULL)
        {
          WARNING ("could not fit right spline list");
          LOG1 ("Could not fit spline to right curve (%lx).\n",
                (unsigned long) right_curve);
        }
      else
        {
          concat_spline_lists (spline_list, *right_spline_list);
	  free_spline_list (*right_spline_list);
	  free (right_spline_list);
	}
	if (CURVE_END_TANGENT (left_curve))
	  free (CURVE_END_TANGENT (left_curve));
	free (left_curve);
	free (right_curve);
    }

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

#define B0(t) CUBE (1 - (t))
#define B1(t) (3.0 * (t) * SQUARE (1 - (t)))
#define B2(t) (3.0 * SQUARE (t) * (1 - (t)))
#define B3(t) CUBE (t)

static spline_type
fit_one_spline (curve_type curve)
{
  /* Since our arrays are zero-based, the `C0' and `C1' here correspond
     to `C1' and `C2' in the paper.  */
  real X_C1_det, C0_X_det, C0_C1_det;
  real alpha1, alpha2;
  spline_type spline;
  vector_type start_vector, end_vector;
  unsigned i;
  vector_type *A;
  vector_type t1_hat = *CURVE_START_TANGENT (curve);
  vector_type t2_hat = *CURVE_END_TANGENT (curve);
  real C[2][2] = { { 0.0, 0.0 }, { 0.0, 0.0 } };
  real X[2] = { 0.0, 0.0 };

  XMALLOC (A, CURVE_LENGTH (curve) * 2
   * sizeof (vector_type ));  /* A dynamically allocated array. */

  START_POINT (spline) = CURVE_POINT (curve, 0);
  END_POINT (spline) = LAST_CURVE_POINT (curve);
  start_vector = make_vector (START_POINT (spline));
  end_vector = make_vector (END_POINT (spline));

  for (i = 0; i < CURVE_LENGTH (curve); i++)
    {
      A[(i << 1) + 0] = Vmult_scalar (t1_hat, B1 (CURVE_T (curve, i)));
      A[(i << 1) + 1] = Vmult_scalar (t2_hat, B2 (CURVE_T (curve, i)));
    }

  for (i = 0; i < CURVE_LENGTH (curve); i++)
    {
      vector_type temp, temp0, temp1, temp2, temp3;
      vector_type *Ai = A + (i << 1);

      C[0][0] += Vdot (Ai[0], Ai[0]);
      C[0][1] += Vdot (Ai[0], Ai[1]);
      /* C[1][0] = C[0][1] (this is assigned outside the loop)  */
      C[1][1] += Vdot (Ai[1], Ai[1]);

      /* Now the right-hand side of the equation in the paper.  */
      temp0 = Vmult_scalar (start_vector, B0 (CURVE_T (curve, i)));
      temp1 = Vmult_scalar (start_vector, B1 (CURVE_T (curve, i)));
      temp2 = Vmult_scalar (end_vector, B2 (CURVE_T (curve, i)));
      temp3 = Vmult_scalar (end_vector, B3 (CURVE_T (curve, i)));

      temp = make_vector (Vsubtract_point (CURVE_POINT (curve, i),
			  Vadd (temp0, Vadd (temp1, Vadd (temp2, temp3)))));

      X[0] += Vdot (temp, Ai[0]);
      X[1] += Vdot (temp, Ai[1]);
    }
  free (A);

  C[1][0] = C[0][1];

  X_C1_det = X[0] * C[1][1] - X[1] * C[0][1];
  C0_X_det = C[0][0] * X[1] - C[0][1] * X[0];
  C0_C1_det = C[0][0] * C[1][1] - C[1][0] * C[0][1];
  if (C0_C1_det == 0.0)
    FATAL ("zero determinant of C0*C1");

  alpha1 = X_C1_det / C0_C1_det;
  alpha2 = C0_X_det / C0_C1_det;

  CONTROL1 (spline) = Vadd_point (START_POINT (spline),
                                  Vmult_scalar (t1_hat, alpha1));
  CONTROL2 (spline) = Vadd_point (END_POINT (spline),
                                  Vmult_scalar (t2_hat, alpha2));
  SPLINE_DEGREE (spline) = CUBICTYPE;

  return spline;
}


/* Find closer-to-optimal t values for the given x,y's and control
   points, using Newton-Raphson iteration.  A good description of this
   is in Plass & Stone.  This routine performs one step in the
   iteration, not the whole thing.  */

static boolean
reparameterize (curve_type curve, spline_type S)
{
  unsigned p, i;
  spline_type S1, S2;		/* S' and S''.  */

  /* Find the first and second derivatives of S.  To make
     `evaluate_spline' work, we fill the beginning points (i.e., the first
     two for a linear spline and the first three for a quadratic one),
     even though this is at odds with the rest of the program.  */
  for (i = 0; i < 3; i++)
    {
      S1.v[i].x = 3.0 * (S.v[i + 1].x - S.v[i].x);
      S1.v[i].y = 3.0 * (S.v[i + 1].y - S.v[i].y);
    }
  S1.v[i].x = S1.v[i].y = -1.0;	/* These will never be accessed.  */
  SPLINE_DEGREE (S1) = QUADRATICTYPE;

  for (i = 0; i < 2; i++)
    {
      S2.v[i].x = 2.0 * (S1.v[i + 1].x - S1.v[i].x);
      S2.v[i].y = 2.0 * (S1.v[i + 1].y - S1.v[i].y);
    }
  S2.v[2].x = S2.v[2].y = S2.v[3].x = S2.v[3].y = -1.0;
  SPLINE_DEGREE (S2) = LINEARTYPE;

  for (p = 0; p < CURVE_LENGTH (curve); p++)
    {
      real new_distance, old_distance;
      real_coordinate_type new_point;
      real_coordinate_type point = CURVE_POINT (curve, p);
      real t = CURVE_T (curve, p);

      /* Find the points at this t on S, S', and S''.  */
      real_coordinate_type S_t = evaluate_spline (S, t);
      real_coordinate_type S1_t = evaluate_spline (S1, t);
      real_coordinate_type S2_t = evaluate_spline (S2, t);

      /* The step size, f(t)/f'(t).  */
      real numerator;
      real denominator;
      /* The differences in x and y (Q1(t) on p.62 of Schneider's thesis).  */
      real_coordinate_type d;
      d.x = S_t.x - point.x;
      d.y = S_t.y - point.y;
      numerator = d.x * S1_t.x + d.y * S1_t.y;
      denominator = (SQUARE (S1_t.x) + d.x * S2_t.x
			  + SQUARE (S1_t.y) + d.y * S2_t.y);

      /* We compute the distances only to be able to check that we
         really are moving closer.  I don't know how this condition can
         be reliably checked for in advance, but I know what it means in
         practice: the original fit was not good enough for the
         Newton-Raphson iteration to converge.  Therefore, we need to
         abort the reparameterization, and subdivide.  */
      old_distance = distance (S_t, point);
      CURVE_T (curve, p) -= numerator / denominator;
      new_point = evaluate_spline (S, CURVE_T (curve, p));
      new_distance = distance (new_point, point);

      if (new_distance > old_distance)
	{
	  LOG3 ("  Stopped reparameterizing; %.3f > %.3f at point %u.\n",
		new_distance, old_distance, p);
	  return false;
	}

      /* The t value might be negative or > 1, if the choice of control
         points wasn't so great.  We have no difficulty in evaluating
         a spline at a t outside the range zero to one, though, so it
         doesn't matter.  (Although it is a little unconventional.)  */
    }
  LOG ("  reparameterized curve:\n   ");
  log_curve (curve, true);

  return true;
}


/* This routine finds the best place to subdivide the curve CURVE,
   somewhere near to the point whose index is INITIAL.  Originally,
   curves were always subdivided at the point of worst error, which is
   intuitively appealing, but doesn't always give the best results.  For
   example, at the end of a serif that tapers into the stem, the best
   subdivision point is at the point where they join, even if the worst
   point is a little ways into the serif.

   We return the index of the point at which to subdivide.  */

static unsigned
find_subdivision (curve_type curve, unsigned initial,
  fitting_opts_type *fitting_opts)
{
  unsigned i, n_done;
  int best_point = -1;
  unsigned search = (unsigned) (fitting_opts->subdivide_search * (real) CURVE_LENGTH (curve));
  vector_type best;
  best.dx = FLT_MAX;
  best.dy = FLT_MAX;

  LOG1 ("  Number of points to search: %u.\n", search);

  /* We don't want to find the first (or last) point in the curve.  */
  for (i = initial, n_done = 0; i > 0 && n_done < search;
       i = CURVE_PREV (curve, i), n_done++)
    {
      if (test_subdivision_point (curve, i, &best, fitting_opts))
        {
          best_point = i;
          LOG3 ("    Better point: (%.3f,%.3f), #%u.\n",
               CURVE_POINT (curve, i).x, CURVE_POINT (curve, i).y, i);
        }
    }

  /* If we found a good one, let's take it.  */
  if (best_point != -1)
    return best_point;

  for (i = CURVE_NEXT (curve, initial), n_done = 0;
       i < CURVE_LENGTH (curve) - 1 && n_done < search;
       i = CURVE_NEXT (curve, i), n_done++)
    {
      if (test_subdivision_point (curve, i, &best, fitting_opts))
        {
          best_point = i;
          LOG3 ("    Better point at (%.3f,%.3f), #%u.\n",
               CURVE_POINT (curve, i).x, CURVE_POINT (curve, i).y, i);
        }
    }

  /* If we didn't find any better point, return the original.  */
  return best_point == -1 ? initial : best_point;
}


/* Here are some macros that decide whether or not we're at a
   ``join point'', as described above.  */
#define ONLY_ONE_LESS(v)												\
  (((v).dx < fitting_opts->subdivide_threshold && (v).dy >  			\
   fitting_opts->subdivide_threshold) || ((v).dy <          			\
   fitting_opts->subdivide_threshold && (v).dx              			\
    > fitting_opts->subdivide_threshold))

#define BOTH_GREATER(v)													\
  ((v).dx > fitting_opts->subdivide_threshold && (v).dy >   			\
   fitting_opts->subdivide_threshold)

/* We assume that the vectors V1 and V2 are nonnegative.  */
#define JOIN(v1, v2)													\
    ((ONLY_ONE_LESS (v1) && BOTH_GREATER (v2))							\
     || (ONLY_ONE_LESS (v2) && BOTH_GREATER (v1)))

/* If the component D of the vector V is smaller than the best so far,
   update the best point.  */
#define UPDATE_BEST(v, d)												\
  do																	\
    {																	\
      if ((v).d < fitting_opts->subdivide_threshold && (v).d < best->d)\
        best->d = (v).d;												\
    }																	\
  while (0)


/* If the point INDEX in the curve CURVE is the best subdivision point
   we've found so far, update the vector BEST.  */

static boolean
test_subdivision_point (curve_type curve, unsigned index, vector_type *best,
  fitting_opts_type *fitting_opts)
{
  unsigned count;
  vector_type in, out;
  boolean join = false;

  find_curve_vectors (index, curve, fitting_opts->subdivide_surround, &in, &out,
      &count);

  /* We don't want to subdivide at points which are very close to the
     endpoints, so if the vectors aren't computed from as many points as
     we asked for, don't bother checking this point.  */
  if (count == fitting_opts->subdivide_surround)
    {
      in = Vabs (in);
      out = Vabs (out);

      join = JOIN (in, out);
      if (join)
        {
          UPDATE_BEST (in, dx);
          UPDATE_BEST (in, dy);
          UPDATE_BEST (out, dx);
          UPDATE_BEST (out, dy);
        }
    }

  return join;
}


/* Find reasonable values for t for each point on CURVE.  The method is
   called chord-length parameterization, which is described in Plass &
   Stone.  The basic idea is just to use the distance from one point to
   the next as the t value, normalized to produce values that increase
   from zero for the first point to one for the last point.  */

static void
set_initial_parameter_values (curve_type curve)
{
  unsigned p;

  LOG ("\nAssigning initial t values:\n  ");

  CURVE_T (curve, 0) = 0.0;

  for (p = 1; p < CURVE_LENGTH (curve); p++)
    {
      real_coordinate_type point = CURVE_POINT (curve, p),
                           previous_p = CURVE_POINT (curve, p - 1);
      real d = distance (point, previous_p);
      CURVE_T (curve, p) = CURVE_T (curve, p - 1) + d;
    }

  MYASSERT (LAST_CURVE_T (curve) != 0.0);

  for (p = 1; p < CURVE_LENGTH (curve); p++)
    CURVE_T (curve, p) = CURVE_T (curve, p) / LAST_CURVE_T (curve);

  log_entire_curve (curve);
}


/* Find an approximation to the tangent to an endpoint of CURVE (to the
   first point if TO_START_POINT is true, else the last).  If
   CROSS_CURVE is true, consider points on the adjacent curve to CURVE.

   It is important to compute an accurate approximation, because the
   control points that we eventually decide upon to fit the curve will
   be placed on the half-lines defined by the tangents and
   endpoints...and we never recompute the tangent after this.  */

static void
find_tangent (curve_type curve, boolean to_start_point, boolean cross_curve,
  unsigned tangent_surround)
{
  vector_type tangent;
  vector_type **curve_tangent = to_start_point ? &(CURVE_START_TANGENT (curve))
                                               : &(CURVE_END_TANGENT (curve));
  unsigned n_points = 0;

  LOG1 ("  tangent to %s: ", to_start_point ? "start" : "end");

  if (*curve_tangent == NULL)
    {
      XMALLOC (*curve_tangent, sizeof (vector_type));
      tangent = find_half_tangent (curve, to_start_point, &n_points,
	    tangent_surround);

      if ((cross_curve) || (CURVE_CYCLIC (curve) == true))
        {
          curve_type adjacent_curve
            = to_start_point ? PREVIOUS_CURVE (curve) : NEXT_CURVE (curve);
          vector_type tangent2
            = find_half_tangent (adjacent_curve, !to_start_point, &n_points,
		        tangent_surround);

          LOG2 ("(adjacent curve half tangent (%.3f,%.3f)) ",
                tangent2.dx, tangent2.dy);
          tangent = Vadd (tangent, tangent2);
        }

      MYASSERT (n_points > 0);
      **curve_tangent = Vmult_scalar (tangent, 1.0 / n_points);
    }
  else
    LOG ("(already computed) ");

  LOG2 ("(%.3f,%.3f).\n", (*curve_tangent)->dx, (*curve_tangent)->dy);
}


/* Find the change in y and change in x for `tangent_surround' (a global)
   points along CURVE.  Increment N_POINTS by the number of points we
   actually look at.  */

static vector_type
find_half_tangent (curve_type c, boolean to_start_point, unsigned *n_points,
  unsigned tangent_surround)
{
  unsigned p;
  int factor = to_start_point ? 1 : -1;
  unsigned tangent_index = to_start_point ? 0 : c->length - 1;
  real_coordinate_type tangent_point = CURVE_POINT (c, tangent_index);
  vector_type tangent = { 0.0, 0.0 };
  unsigned int surround;

  if ((surround = CURVE_LENGTH (c) / 2) > tangent_surround)
    surround = tangent_surround;

  for (p = 1; p <= surround; p++)
    {
      int this_index = p * factor + tangent_index;
      real_coordinate_type this_point;

      if (this_index < 0 || this_index >= (int) c->length)
	break;

      this_point = CURVE_POINT (c, p * factor + tangent_index);

      /* Perhaps we should weight the tangent from `this_point' by some
         factor dependent on the distance from the tangent point.  */
      tangent = Vadd (tangent,
		      Vmult_scalar (Psubtract (this_point, tangent_point),
				    factor));
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

static real
find_error (curve_type curve, spline_type spline, unsigned *worst_point)
{
  unsigned this_point;
  real total_error = 0.0;
  real worst_error = FLT_MIN;

  *worst_point = CURVE_LENGTH (curve) + 1;   /* A sentinel value.  */

  for (this_point = 0; this_point < CURVE_LENGTH (curve); this_point++)
    {
      real_coordinate_type curve_point = CURVE_POINT (curve, this_point);
      real t = CURVE_T (curve, this_point);
      real_coordinate_type spline_point = evaluate_spline (spline, t);
      real this_error = distance (curve_point, spline_point);
      if (this_error >= worst_error)
        {
         *worst_point = this_point;
          worst_error = this_error;
        }
      total_error += this_error;
    }

  if (*worst_point == CURVE_LENGTH (curve) + 1)
    { /* Didn't have any ``worst point''; the error should be zero.  */
      if (epsilon_equal (total_error, 0.0))
	LOG ("  Every point fit perfectly.\n");
      else
	WARNING ("No worst point found; something is wrong");
    }
  else
    {
      if (epsilon_equal (total_error, 0.0))
	LOG ("  Every point fit perfectly.\n");
      else
        {
          LOG4 ("  Worst error (at (%.3f,%.3f), point #%u) was %.3f.\n",
              CURVE_POINT (curve, *worst_point).x,
              CURVE_POINT (curve, *worst_point).y, *worst_point, worst_error);
          LOG1 ("  Total error was %.3f.\n", total_error);
          LOG2 ("  Average error (over %u points) was %.3f.\n",
              CURVE_LENGTH (curve), total_error / CURVE_LENGTH (curve));
        }
    }

  return worst_error;
}


/* Supposing that we have accepted the error, another question arises:
   would we be better off just using a straight line?  */

static boolean
spline_linear_enough (spline_type *spline, curve_type curve,
  fitting_opts_type *fitting_opts)
{
  real A, B, C, slope;
  unsigned this_point;
  real distance = 0.0, start_end_distance, threshold;

  LOG ("Checking linearity:\n");

  start_end_distance = sqrt (SQUARE (END_POINT (*spline).x - START_POINT
    (*spline).x) + SQUARE (END_POINT (*spline).y - START_POINT (*spline).y));
LOG1 ("start_end_distance is %.3f.\n", start_end_distance);
  /* For a line described by Ax + By + C = 0, the distance d from a
     point (x0,y0) to that line is:

     d = | Ax0 + By0 + C | / sqrt (A^2 + B^2).

     We can find A, B, and C from the starting and ending points,
     unless the line defined by those two points is vertical.  */

  if (epsilon_equal (START_POINT (*spline).x, END_POINT (*spline).x))
    {
      A = 1;
      B = 0;
      C = -START_POINT (*spline).x;
    }
  else
    {
      /* Plug the spline's starting and ending points into the two-point
	 equation for a line, that is,

	 (y - y1) = ((y2 - y1)/(x2 - x1)) * (x - x1)

	 to get the values for A, B, and C.  */

      slope = ((END_POINT (*spline).y - START_POINT (*spline).y)
	       / (END_POINT (*spline).x - START_POINT (*spline).x));
      A = -slope;
      B = 1;
      C = slope * START_POINT (*spline).x - START_POINT (*spline).y;
    }
  LOG3 ("  Line is %.3fx + %.3fy + %.3f = 0.\n", A, B, C);

  for (this_point = 0; this_point < CURVE_LENGTH (curve); this_point++)
    {
      real t = CURVE_T (curve, this_point);
      real_coordinate_type spline_point = evaluate_spline (*spline, t);

      distance += fabs (A * spline_point.x + B * spline_point.y + C)
                   / sqrt (A * A + B * B);
    }
  LOG1 ("  Total distance is %.3f, ", distance);

  distance /= (CURVE_LENGTH (curve) - 1);
  LOG1 ("which is %.3f normalized.\n", distance);

  /* We want reversion of short curves to splines to be more likely than
     reversion of long curves, hence the second division by the curve
     length, for use in `change_bad_lines'.  */
  SPLINE_LINEARITY (*spline) = distance;
  LOG1 ("  Final linearity: %.3f.\n", SPLINE_LINEARITY (*spline));
  if (start_end_distance * 0.5 > fitting_opts->line_threshold)
    threshold = fitting_opts->line_threshold;
  else
    threshold = start_end_distance * 0.5;
  LOG1 ("threshold is %.3f .\n", threshold);
  return distance < threshold;
}


/* Unfortunately, we cannot tell in isolation whether a given spline
   should be changed to a line or not.  That can only be known after the
   entire curve has been fit to a list of splines.  (The curve is the
   pixel outline between two corners.)  After subdividing the curve, a
   line may very well fit a portion of the curve just as well as the
   spline---but unless a spline is truly close to being a line, it
   should not be combined with other lines.  */

static void
change_bad_lines (spline_list_type *spline_list,
  fitting_opts_type *fitting_opts)
{
  unsigned this_spline;
  boolean found_cubic = false;
  unsigned length = SPLINE_LIST_LENGTH (*spline_list);

  LOG1 ("\nChecking for bad lines (length %u):\n", length);

  /* First see if there are any splines in the fitted shape.  */
  for (this_spline = 0; this_spline < length; this_spline++)
    {
      if (SPLINE_DEGREE (SPLINE_LIST_ELT (*spline_list, this_spline)) ==
       CUBICTYPE)
        {
          found_cubic = true;
          break;
        }
    }

  /* If so, change lines back to splines (we haven't done anything to
     their control points, so we only have to change the degree) unless
     the spline is close enough to being a line.  */
  if (found_cubic)
    for (this_spline = 0; this_spline < length; this_spline++)
      {
        spline_type s = SPLINE_LIST_ELT (*spline_list, this_spline);

        if (SPLINE_DEGREE (s) == LINEARTYPE)
          {
            LOG1 ("  #%u: ", this_spline);
            if (SPLINE_LINEARITY (s) > fitting_opts->line_reversion_threshold)
              {
                LOG ("reverted, ");
                SPLINE_DEGREE (SPLINE_LIST_ELT (*spline_list, this_spline))
                  = CUBICTYPE;
              }
            LOG1 ("linearity %.3f.\n", SPLINE_LINEARITY (s));
          }
      }
    else
      LOG ("  No lines.\n");
}


/* When we have finished fitting an entire pixel outline to a spline
   list L, we go through L to ensure that any endpoints that are ``close
   enough'' (i.e., within `align_threshold') to being the same really
   are the same.  */

/* This macro adjusts the AXIS axis on the starting and ending points on
   a particular spline if necessary.  */
#define TRY_AXIS(axis)													\
  do																	\
    {																	\
      real delta = fabs (end.axis - start.axis);						\
																		\
      if (!epsilon_equal (delta, 0.0) && delta <=              			\
       fitting_opts->align_threshold)                                         		\
        {																\
          spline_type *next = &NEXT_SPLINE_LIST_ELT (*l, this_spline);	\
          spline_type *prev = &PREV_SPLINE_LIST_ELT (*l, this_spline);	\
																		\
          START_POINT (*s).axis = END_POINT (*s).axis					\
            = END_POINT (*prev).axis = START_POINT (*next).axis			\
            = (start.axis + end.axis) / 2;								\
          spline_change = true;											\
        }																\
    }																	\
  while (0)

static void
align (spline_list_type *l, fitting_opts_type *fitting_opts)
{
  boolean change;
  unsigned this_spline;
  unsigned length = SPLINE_LIST_LENGTH (*l);

  LOG1 ("\nAligning spline list (length %u):\n", length);

  do
    {
      change = false;

      LOG ("  ");

      for (this_spline = 0; this_spline < length; this_spline++)
        {
          boolean spline_change = false;
          spline_type *s = &SPLINE_LIST_ELT (*l, this_spline);
          real_coordinate_type start = START_POINT (*s);
          real_coordinate_type end = END_POINT (*s);

          TRY_AXIS (x);
          TRY_AXIS (y);
          if (spline_change)
            {
              LOG1 ("%u ", this_spline);
              change |= spline_change;
            }
        }
      LOG ("\n");
    }
  while (change);
}


/* Lists of array indices (well, that is what we use it for).  */

static index_list_type
new_index_list (void)
{
  index_list_type index_list;

  index_list.data = NULL;
  INDEX_LIST_LENGTH (index_list) = 0;

  return index_list;
}


static void
free_index_list (index_list_type *index_list)
{
  if (INDEX_LIST_LENGTH (*index_list) > 0)
    {
      free (index_list->data);
      index_list->data = NULL;
      INDEX_LIST_LENGTH (*index_list) = 0;
    }
}


static void
append_index (index_list_type *list, unsigned new_index)
{
  INDEX_LIST_LENGTH (*list)++;
  XREALLOC (list->data, INDEX_LIST_LENGTH (*list) * sizeof (unsigned));
  list->data[INDEX_LIST_LENGTH (*list) - 1] = new_index;
}


/* Turn an real point into a integer one.  */

static coordinate_type
real_to_int_coord (real_coordinate_type real_coord)
{
  coordinate_type int_coord;

  int_coord.x = ROUND (real_coord.x);
  int_coord.y = ROUND (real_coord.y);

  return int_coord;
}


/* Return the Euclidean distance between P1 and P2.  */

static const real
distance (real_coordinate_type p1, real_coordinate_type p2)
{
  return hypot (p1.x - p2.x, p1.y - p2.y);
}

/* version 0.24 */
