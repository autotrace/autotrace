/* input-png.h: import png files. 
   
   Copyright (C) 2000 MenTaLguY <mental@rydia.net>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public License
   as published by the Free Software Foundation; either version 2.1 of
   the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with this library; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
   USA. */

/* $Id: input-png.h,v 1.7 2002/10/05 19:38:25 masata-y Exp $ */

#ifndef INPUT_PNG_H
#define INPUT_PNG_H

#include "input.h"

at_bitmap_type input_png_reader (at_string filename,
				 at_input_opts_type * opts,
				 at_msg_func msg_func, 
				 at_address msg_data);

#endif /* not INPUT_PNG_H */
