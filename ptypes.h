/* ptypes.h: types used in automake code private
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

#ifndef P_TYPES_H
#define P_TYPES_H

#include "autotrace.h"

/* Curvetype */
#define LINE 1
#define QUAD_BEZIER 2
#define CUB_BEZIER 4
#define CIRCLE 8
#define PARALLEL_ELLIPSE 16
#define ELLIPSE 32

typedef at_coordinate_type coordinate_type;
typedef at_real_coordinate_type real_coordinate_type ;

typedef at_progress_func progress_func;
typedef at_testcancel_func testcancel_func;

#endif /* not P_TYPES_H */
