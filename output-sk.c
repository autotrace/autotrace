/* output-sk.c - output in sketch format

   Copyright (C) 1999, 2000, 2001 Bernhard Herzog

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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* Def: HAVE_CONFIG_H */

#include "spline.h"
#include "color.h"
#include "output-sk.h"

static void
out_splines (FILE * file, spline_list_array_type shape)
{
  unsigned this_list;
  spline_list_type list;
  
  color_type last_color = {0,0,0};

  for (this_list = 0; this_list < SPLINE_LIST_ARRAY_LENGTH (shape);
    this_list++)
    {
      unsigned this_spline;
      spline_type first;

      list = SPLINE_LIST_ARRAY_ELT (shape, this_list);
      first = SPLINE_LIST_ELT (list, 0);

      if (this_list == 0 || !COLOR_EQUAL(list.color, last_color))
        {
          if (this_list > 0 && !shape.centerline)
            fputs("bC()\n", file);
          fprintf(file, (shape.centerline || list.open) ? "lp((%g,%g,%g))\n"
            : "fp((%g,%g,%g))\n", list.color.r / 255.0,
            list.color.g / 255.0, list.color.b / 255.0); 
          fputs((shape.centerline || list.open) ? "fe()\n" : "le()\n", file); /* no outline */
            last_color = list.color;
          fputs("b()\n", file); /* the beginning of a bezier object */
	    }
    
      fprintf(file, "bs(%g,%g,0)\n",
        START_POINT(first).x, START_POINT(first).y);

      for (this_spline = 0; this_spline < SPLINE_LIST_LENGTH (list);
	     this_spline++)
        {
          spline_type s = SPLINE_LIST_ELT (list, this_spline);

          if (SPLINE_DEGREE(s) == LINEARTYPE)
            fprintf(file, "bs(%g,%g,0)\n", END_POINT(s).x, END_POINT(s).y);
          else
            fprintf(file, "bc(%g,%g,%g,%g,%g,%g,0)\n",
              CONTROL1(s).x, CONTROL1(s).y,
              CONTROL2(s).x, CONTROL2(s).y,
              END_POINT(s).x, END_POINT(s).y);
        }
    }
  if (SPLINE_LIST_ARRAY_LENGTH(shape) > 0 && !shape.centerline)
	  fputs("bC()\n", file);
}


int output_sk_writer(FILE* file, at_string name,
		     int llx, int lly, int urx, int ury, 
		     at_output_opts_type * opts,
		     spline_list_array_type shape,
		     at_msg_func msg_func, at_address msg_data)
{
  fputs("##Sketch 1 0\n", file);
  fputs("document()\n", file);
  fputs("layer('Layer 1',1,1,0,0)\n", file);
  fputs("guess_cont()\n", file);

  out_splines(file, shape);
  return 0;
}
