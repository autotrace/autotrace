/* vector.h: operations on vectors and points. */

#ifndef VECTOR_H
#define VECTOR_H

#include "types.h"
#include "exception.h"

/* Our vectors are represented as displacements along the x and y axes.  */

typedef struct {
  gfloat dx, dy, dz;
} vector_type;

/* Consider a point as a vector from the origin.  */
extern vector_type make_vector(const at_real_coord);

/* And a vector as a point, i.e., a displacement from the origin.  */
extern at_real_coord vector_to_point(const vector_type);

/* Definitions for these common operations can be found in any decent
   linear algebra book, and most calculus books.  */

extern gfloat magnitude(const vector_type);
extern vector_type normalize(const vector_type);

extern vector_type Vadd(const vector_type, const vector_type);
extern gfloat Vdot(const vector_type, const vector_type);
extern vector_type Vmult_scalar(const vector_type, const gfloat);
extern gfloat Vangle(const vector_type in, const vector_type out, at_exception_type * exp);

/* These operations could have been named `P..._vector' just as well as
   V..._point, so we may as well allow both names.  */
#define Padd_vector Vadd_point
extern at_real_coord Vadd_point(const at_real_coord, const vector_type);

#define Psubtract_vector Vsubtract_point
extern at_real_coord Vsubtract_point(const at_real_coord, const vector_type);

/* This returns the rounded sum.  */
#define IPadd_vector Vadd_int_point
extern at_coord Vadd_int_point(const at_coord, const vector_type);

/* Take the absolute value of both components.  */
extern vector_type Vabs(const vector_type);

/* Operations on points with real coordinates.  It is not orthogonal,
   but more convenient, to have the subtraction operator return a
   vector, and the addition operator return a point.  */
extern vector_type Psubtract(const at_real_coord, const at_real_coord);

/* These are heavily used in spline fitting.  */
extern at_real_coord Padd(const at_real_coord, const at_real_coord);
extern at_real_coord Pmult_scalar(const at_real_coord, const gfloat);

/* Similarly, for points with integer coordinates; here, a subtraction
   operator that does return another point is useful.  */
extern vector_type IPsubtract(const at_coord, const at_coord);
extern at_coord IPsubtractP(const at_coord, const at_coord);
extern at_coord IPadd(const at_coord, const at_coord);
extern at_coord IPmult_scalar(const at_coord, const int);
extern at_real_coord IPmult_real(const at_coord, const gfloat);
extern gboolean IPequal(const at_coord, const at_coord);

#endif /* not VECTOR_H */
