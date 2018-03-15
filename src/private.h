/* private.h --- Autotrace library private decls

  Copyright (C) 2003 Martin Weber
  Copyright (C) 2003 Masatake YAMATO

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

#ifndef PRIVATE_H
#define PRIVATE_H

#include "autotrace.h"
#include "input.h"
#include "output.h"

struct _at_bitmap_reader {
  at_input_func func;
  gpointer data;
};

struct _at_spline_writer {
  at_output_func func;
  gpointer data;
};

int at_input_init(void);
int at_output_init(void);
int at_param_init(void);
int at_module_init(void);
#endif /* Not def: PRIVATE_H */
