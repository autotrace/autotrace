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

#ifndef AT_TYPES_H
#define AT_TYPES_H

#include <stdint.h>
#include <glib.h>

#ifdef __cplusplus
extern "C" {
#endif                          /* __cplusplus */

/* Cartesian points.  */
  typedef struct _at_coord {
    gushort x, y;
  } at_coord;

  typedef struct _at_real_coord {
    gfloat x, y, z;
  } at_real_coord;

#ifdef __cplusplus
}
#endif                          /* __cplusplus */
#endif                          /* not AT_TYPES_H */
