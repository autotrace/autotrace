/*
 * SPDX-FileCopyrightText: © 2000-2001 Martin Weber
 * SPDX-FileCopyrightText: © 2001-2003 Masatake YAMATO
 * SPDX-FileCopyrightText: © 2017-2025 Peter Lemenkov
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

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

/* Definitions for these common operations can be found in any decent
   linear algebra book, and most calculus books.  */

extern vector_type Vadd(const vector_type, const vector_type);
extern gfloat Vdot(const vector_type, const vector_type);
extern vector_type Vmult_scalar(const vector_type, const gfloat);
extern gfloat Vangle(const vector_type in, const vector_type out, at_exception_type *exp);

extern at_real_coord Vadd_point(const at_real_coord, const vector_type);
extern at_real_coord Vsubtract_point(const at_real_coord, const vector_type);

/* Operations on points with real coordinates.  It is not orthogonal,
   but more convenient, to have the subtraction operator return a
   vector, and the addition operator return a point.  */
extern vector_type Psubtract(const at_real_coord, const at_real_coord);

/* These are heavily used in spline fitting.  */
extern at_real_coord Padd(const at_real_coord, const at_real_coord);
extern at_real_coord Pmult_scalar(const at_real_coord, const gfloat);

/* Similarly, for points with integer coordinates.  */
extern vector_type IPsubtract(const at_coord, const at_coord);

#endif /* not VECTOR_H */
