/* fit.h: convert the pixel representation to splines. */

#ifndef FIT_H
#define FIT_H

#include "types.h"
#include "pxl-outline.h"
#include "spline.h"

/* See fit.c for descriptions of these variables, all of which can be
   set using options.  */

typedef struct
{
/* If two endpoints are closer than this, they are made to be equal.
   (-align-threshold)  */
  real align_threshold;

/* Background color, the color of the background that should be ignored */
  color_type *bgColor;

/* To how many colors the bitmap is reduced */
  unsigned color_count;

/* If the angle defined by a point and its predecessors and successors
   is smaller than this, it's a corner, even if it's within
   `corner_surround' pixels of a point with a smaller angle.
   (-corner-always-threshold)  */
  real corner_always_threshold;

/* Number of points to consider when determining if a point is a corner
   or not.  (-corner-surround)  */
  unsigned corner_surround;

/* If a point, its predecessors, and its successors define an angle
    smaller than this, it's a corner.  Should be in range 0..180.
   (-corner-threshold)  */
  real corner_threshold;

/* Amount of error at which a fitted spline is unacceptable.  If any
   pixel is further away than this from the fitted curve, we try again.
   (-error-threshold) */
  real error_threshold;

/* A second number of adjacent points to consider when filtering.
   (-filter-alternative-surround)  */
  unsigned filter_alternative_surround;

/* If the angles between the vectors produced by filter_surround and
   filter_alternative_surround points differ by more than this, use
   the one from filter_alternative_surround.  (-filter-epsilon)  */
  real filter_epsilon;

/* Number of times to smooth original data points.  Increasing this
   number dramatically---to 50 or so---can produce vastly better
   results.  But if any points that ``should'' be corners aren't found,
   the curve goes to hell around that point.  (-filter-iterations)  */
  unsigned filter_iteration_count;

/* To produce the new point, use the old point plus this times the
   neighbors.  (-filter-percent)  */
  real filter_percent;

/* Number of adjacent points to consider if `filter_surround' points
   defines a straight line.  (-filter-secondary-surround)  */
  unsigned filter_secondary_surround;

/* Number of adjacent points to consider when filtering.
  (-filter-surround)  */
  unsigned filter_surround;

/* If a spline is closer to a straight line than this, it remains a
   straight line, even if it would otherwise be changed back to a curve.
   This is weighted by the square of the curve length, to make shorter
   curves more likely to be reverted.  (-line-reversion-threshold)  */
  real line_reversion_threshold;

/* How many pixels (on the average) a spline can diverge from the line
   determined by its endpoints before it is changed to a straight line.
   (-line-threshold) */
  real line_threshold;

/* Should adjacent corners be removed?  */
  bool remove_adj_corners;

/* If reparameterization doesn't improve the fit by this much percent,
   stop doing it.  (-reparameterize-improve)  */
  real reparameterize_improvement;

/* Amount of error at which it is pointless to reparameterize.  This
   happens, for example, when we are trying to fit the outline of the
   outside of an `O' with a single spline.  The initial fit is not good
   enough for the Newton-Raphson iteration to improve it.  It may be
   that it would be better to detect the cases where we didn't find any
   corners.  (-reparameterize-threshold)  */
  real reparameterize_threshold;

/* Percentage of the curve away from the worst point to look for a
   better place to subdivide.  (-subdivide-search)  */
  real subdivide_search;

/* Number of points to consider when deciding whether a given point is a
   better place to subdivide.  (-subdivide-surround)  */
  unsigned subdivide_surround;

/* How many pixels a point can diverge from a straight line and still be
   considered a better place to subdivide.  (-subdivide-threshold) */
  real subdivide_threshold;

/* Number of points to look at on either side of a point when computing
   the approximation to the tangent at that point.  (-tangent-surround)  */
  unsigned tangent_surround;

/* Thin all the lines in the image prior to fitting. */
  bool thin;
} fitting_opts_type;


#ifdef _EXPORTING

/* Fit splines and lines to LIST.  */
extern spline_list_array_type __declspec(dllexport) __stdcall
  fitted_splines (pixel_outline_list_type, fitting_opts_type *);

/* Get a new set of fitting options */
extern fitting_opts_type __declspec(dllexport)
__stdcall new_fitting_opts (void);

#elif _IMPORTING

/* Fit splines and lines to LIST.  */
extern spline_list_array_type __declspec(dllimport) __stdcall
  fitted_splines (pixel_outline_list_type, fitting_opts_type *);

/* Get a new set of fitting options */
extern fitting_opts_type __declspec(dllimport)
__stdcall new_fitting_opts (void);

#else

/* Fit splines and lines to LIST.  */
extern spline_list_array_type fitted_splines
  (pixel_outline_list_type, fitting_opts_type *);

/* Get a new set of fitting options */
extern fitting_opts_type new_fitting_opts (void);

#endif

#endif /* not FIT_H */

/* version 0.24 */
