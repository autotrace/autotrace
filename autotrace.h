/* autotrace.h */
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
  LINEARTYPE = 1, QUADRATICTYPE = 2, CUBICTYPE = 3, PARALLELELLIPSETYPE = 4,
  ELLIPSETYPE = 5, CIRCLETYPE = 6 /* not the real number of points to define a
  circle but to distinguish between a cubic spline */
};

/* A Bezier spline can be represented as four points in the real plane:
   a starting point, ending point, and two control points.  The
   curve always lies in the convex hull defined by the four points.  It
   is also convenient to save the divergence of the spline from the
   straight line defined by the endpoints.  */
struct _at_spline_type
{
  real_coordinate_type v[4];	/* The control points.  */
  at_polynomial_degree degree;
  real linearity;
};

/* Each outline in a character is typically represented by many
   splines.  So, here is a list structure for that:  */
struct _at_spline_list_type
{
  at_spline_type *data;
  unsigned length;
  bool clockwise;
  at_color_type color;
};

/* Each character is in general made up of many outlines. So here is one
   more list structure.  */
struct _at_spline_list_array_type
{
  at_spline_list_type *data;
  unsigned length;
};

struct _at_fitting_opts_type
{
/* If two endpoints are closer than this, they are made to be equal.
   (-align-threshold)  */
  real align_threshold;

/* Background color, the color of the background that should be ignored */
  at_color_type *bgColor;

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
};

struct _at_bitmap_type
{
  dimensions_type dimensions;
  unsigned char *bitmap;
  unsigned int np;
};


/*
 * IO Handler typedefs
 */
typedef 
at_bitmap_type (*at_input_read_func)   (string name);

typedef 
int            (*at_output_write_func) (FILE*, string name,
					int llx, int lly, 
					int urx, int ury,
					at_splines_type shape);

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
				string filename);
unsigned short at_bitmap_get_width (at_bitmap_type * bitmap);
unsigned short at_bitmap_get_height (at_bitmap_type * bitmap);
void at_bitmap_free (at_bitmap_type * bitmap);
/* TODO internal data access */

/* Input related */
at_input_read_func at_input_get_handler (string filename);
at_input_read_func at_input_get_handler_by_suffix (string suffix);
char ** at_input_list_new (void);
void at_input_list_free(char ** list);
/* at_input_read_add_handler (string suffix, at_input_read_func); */

/* Spline related */
at_splines_type * at_splines_new (at_bitmap_type * bitmap,
				  at_fitting_opts_type * opts);
void at_splines_free (at_splines_type * splines);
/* TODO internal data access */

/* Output related */
at_output_write_func at_output_get_handler (string suffix);
void 
at_output_write(at_output_write_func output_writer,
		FILE * writeto,
		char * name,
		int llx, int lly, int urx, int ury,
		at_splines_type * splines);
char ** at_output_list_new (void);
void at_output_list_free(char ** list);
/* at_output_write_add_handler (string suffix, at_output_write_func); */

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
