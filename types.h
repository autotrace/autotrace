/* types.h: general types
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

#ifndef TYPES_H
#define TYPES_H

#ifndef __cplusplus
/* Booleans.  */
#ifndef bool
typedef enum { false = 0, true = 1 } bool;
#endif
#endif

/* The usual null-terminated string.  */
typedef char *string;

/* A generic pointer in ANSI C.  */
typedef void *address;

/* We use `real' for our floating-point variables.  */
typedef float real;

/* Cartesian points.  */
typedef struct
{
  short x, y;
} coordinate_type;

typedef struct
{
  real x, y;
} real_coordinate_type;

/* Curvetype */
#define LINE 1
#define QUAD_BEZIER 2
#define CUB_BEZIER 4
#define CIRCLE 8
#define PARALLEL_ELLIPSE 16
#define ELLIPSE 32

/* See autotrace.h::at_progress_func */
typedef void           (* progress_func)    (real percentage,
					     address client_data);

/* See autotrace.h::at_testcancel_func */
typedef bool          (*  testcancel_func) (address client_data);

#endif /* not TYPES_H */
