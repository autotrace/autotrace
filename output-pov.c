/* output-pov.c: utility routines for Povray output. 

   Copyright (C) 2003 Martin Weber.

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

#include "types.h"
#include "spline.h"
#include "color.h"
#include "output-pov.h"
#include "xstd.h"
#include "autotrace.h"
#include <string.h>

#define SIGN(x) ((x) > 0 ? 1 : (x) < 0 ? -1 : 0)
#define ROUND(x) ((int) ((int) (x) + .5 * SIGN (x)))

/* Output macros.  */

/* This should be used for outputting a string S on a line by itself.  */
#define OUT_LINE(s)									\
  fprintf (pov_file, "%s\n", s)

/* These output their arguments, preceded by the indentation.  */
#define OUT1(s, e)									\
  fprintf (pov_file, s, e)

#define OUT3(s, e1, e2, e3)							\
  fprintf (pov_file, s, e1, e2, e3)

#define OUT8(s, e1, e2, e3, e4, e5, e6, e7, e8)		\
  fprintf (pov_file, s, e1, e2, e3, e4, e5, e6, e7, e8)

/* These macros just output their arguments.  */
#define OUT_REAL(r)	fprintf (pov_file, r == (ROUND (r = ROUND((gfloat)6.0*r)/(gfloat)6.0))				\
                                  ? "%.0f " : "%.3f ", r)

/* This outputs the Povray code which produces the shape in
   SHAPE.  */

static void
out_splines (FILE * pov_file, spline_list_array_type shape)
{
  unsigned this_list;
  spline_list_type list;

  color_type last_color = {0,0,0};


  for (this_list = 0; this_list < SPLINE_LIST_ARRAY_LENGTH (shape);
       this_list++)
    {
      unsigned this_spline;
      spline_type first;
	  unsigned test_list;
	  unsigned number = 0;

      list = SPLINE_LIST_ARRAY_ELT (shape, this_list);

	  for (test_list = this_list; test_list < SPLINE_LIST_ARRAY_LENGTH (shape);
         test_list++)
	    {
	      spline_list_type testlist = SPLINE_LIST_ARRAY_ELT (shape, test_list);
          if (!COLOR_EQUAL(testlist.color, list.color))
            break;
		  number += SPLINE_LIST_LENGTH (testlist) * 4;
		}

      first = SPLINE_LIST_ELT (list, 0);

      if (this_list > 0)
	    {
			if (!COLOR_EQUAL(list.color, last_color))
			  {
		        OUT3 ("\n  pigment {rgb<%.3f, %.3f, %.3f>}\n", (double) last_color.r/255.0,
                  (double) last_color.g/255.0,(double) last_color.b/255.0);
	            OUT_LINE ("  translate <0.0, 0.0, 0.0>");    
                OUT_LINE ("}");
		      }
	        else
		      OUT_LINE (",");
		}

      if (this_list == 0 || !COLOR_EQUAL(list.color, last_color))
		{
          OUT_LINE ("prism {");
          OUT_LINE ("  bezier_spline");
          OUT1 ("  %.1f\n", 0.0);
          OUT1 ("  %.4f\n",0.0001);
		  OUT1 ("  %d\n", number);
	      last_color = list.color;
		}

      for (this_spline = 0; this_spline < SPLINE_LIST_LENGTH (list);
           this_spline++)
        {
          spline_type s = SPLINE_LIST_ELT (list, this_spline);

		  if (this_spline > 0) OUT_LINE (",");

		  if (SPLINE_DEGREE (s) == LINEARTYPE)
            OUT8 ("  <%.3f, %.3f>, <%.3f, %.3f>, <%.3f, %.3f>, <%.3f, %.3f>",
			  START_POINT (s).x, START_POINT (s).y,
			  START_POINT (s).x, START_POINT (s).y,
              END_POINT (s).x, END_POINT (s).y,
              END_POINT (s).x, END_POINT (s).y);
          else
            OUT8 ("  <%.3f, %.3f>, <%.3f, %.3f>, <%.3f, %.3f>, <%.3f, %.3f>",
			  START_POINT (s).x, START_POINT (s).y,
			  CONTROL1 (s).x, CONTROL1 (s).y,
              CONTROL2 (s).x, CONTROL2 (s).y,
              END_POINT (s).x, END_POINT (s).y);
        }
    }
  if (SPLINE_LIST_ARRAY_LENGTH(shape) > 0)
	{
	  OUT_LINE ("");
	  OUT3 ("  pigment {rgb<%.3f, %.3f, %.3f>}\n", (double) list.color.r/255.0,
        (double) list.color.g/255.0,(double) list.color.b/255.0);
	  OUT_LINE ("  translate <0.0, 0.0, 0.0>");    
      OUT_LINE ("}");
	}
}


int output_pov_writer(FILE* pov_file, gchar* name,
		      int llx, int lly, int urx, int ury, 
		      at_output_opts_type * opts,
		      spline_list_array_type shape,
		      at_msg_func msg_func, 
		      gpointer msg_data,
		      gpointer user_data)
{
    if (shape.centerline == TRUE)
      FATAL("Povray output currently not supported for centerline method");

    out_splines(pov_file, shape);

    return 0;
}


