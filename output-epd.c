/* output-epd.c: utility routines for EPD output
   (http://epd.sourceforge.net)

   Copyright (C) 1999, 2000, 2001 Martin Weber.

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
#include "output-epd.h"
#include "xstd.h"
#include "autotrace.h"
#include <time.h>
#include <string.h>

static at_string now (void);

#define SIGN(x) ((x) > 0 ? 1 : (x) < 0 ? -1 : 0)
#define ROUND(x) ((int) ((int) (x) + .5 * SIGN (x)))

/* Output macros.  */

/* This should be used for outputting a string S on a line by itself.  */
#define OUT_LINE(s)                                 \
  fprintf (epd_file, "%s\n", s)

/* These output their arguments, preceded by the indentation.  */
#define OUT1(s, e)                                  \
  fprintf (epd_file, s, e)

#define OUT2(s, e1, e2)                             \
  fprintf (epd_file, s, e1, e2)

#define OUT3(s, e1, e2, e3)                         \
  fprintf (epd_file, s, e1, e2, e3)

#define OUT4(s, e1, e2, e3, e4)                     \
  fprintf (epd_file, s, e1, e2, e3, e4)

/* These macros just output their arguments.  */
#define OUT_STRING(s)	fprintf (epd_file, "%s", s)
#define OUT_REAL(r)	fprintf (epd_file, r == (ROUND (r = ROUND((at_real)6.0*r)/(at_real)6.0))				\
                                  ? "%.0f " : "%.3f ", r)

/* For a PostScript command with two real arguments, e.g., lineto.  OP
   should be a constant string.  */
#define OUT_COMMAND2(first, second, op)             \
  do                                                \
    {                                               \
      OUT_REAL (first);                             \
      OUT_REAL (second);                            \
      OUT_STRING (op "\n");                         \
    }                                               \
  while (0)

/* For a PostScript command with six real arguments, e.g., curveto.
   Again, OP should be a constant string.  */
#define OUT_COMMAND6(first, second, third, fourth, fifth, sixth, op)	\
  do                                                \
    {                                               \
      OUT_REAL (first);                             \
      OUT_REAL (second);                            \
      OUT_STRING (" ");                             \
      OUT_REAL (third);                             \
      OUT_REAL (fourth);                            \
      OUT_STRING (" ");                             \
      OUT_REAL (fifth);                             \
      OUT_REAL (sixth);                             \
      OUT_STRING (" " op " \n");                    \
    }                                               \
  while (0)

/* This should be called before the others in this file.  It opens the
   output file `OUTPUT_NAME.ps', and writes some preliminary boilerplate. */

static int output_epd_header(FILE* epd_file, at_string name,
			     int llx, int lly, int urx, int ury)
{
  at_string time;

  OUT_LINE ("%EPD-1.0");
  OUT1 ("%% Created by %s\n", at_version(true));
  OUT1 ("%% Title: %s\n", name);
  OUT1 ("%% CreationDate: %s\n", time = now ());
  OUT4 ("%%BBox(%d,%d,%d,%d)\n", llx, lly, urx, ury);

  free (time);

  return 0;
}

/* This outputs the PostScript code which produces the shape in
   SHAPE.  */

static void
out_splines (FILE * epd_file, spline_list_array_type shape)
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
          if (this_list > 0)
              {
                OUT_LINE ((shape.centerline || list.open) ? "S" : "f");
                OUT_LINE("h");
              }
          OUT4 ("%.3f %.3f %.3f %s\n", (double) list.color.r / 255.0,
            (double) list.color.g / 255.0, (double) list.color.b / 255.0,
            (shape.centerline || list.open) ? "RG" : "rg");
          last_color = list.color;
        }    
      OUT_COMMAND2 (START_POINT (first).x, START_POINT (first).y, "m");

      for (this_spline = 0; this_spline < SPLINE_LIST_LENGTH (list);
           this_spline++)
        {
          spline_type s = SPLINE_LIST_ELT (list, this_spline);

          if (SPLINE_DEGREE (s) == LINEARTYPE)
            OUT_COMMAND2 (END_POINT (s).x, END_POINT (s).y, "l");
          else
            OUT_COMMAND6 (CONTROL1 (s).x, CONTROL1 (s).y,
                          CONTROL2 (s).x, CONTROL2 (s).y,
                          END_POINT (s).x, END_POINT (s).y,
                          "c");
        }
    }
  if (SPLINE_LIST_ARRAY_LENGTH(shape) > 0)
    OUT_LINE ((shape.centerline || list.open) ? "S" : "f");
}


int output_epd_writer(FILE* epd_file, at_string name,
		      int llx, int lly, int urx, int ury, 
		      at_output_opts_type * opts,
		      spline_list_array_type shape,
		      at_msg_func msg_func, 
		      at_address msg_data)
{
    int result;

    result = output_epd_header(epd_file, name, llx, lly, urx, ury);
    if (result != 0)
	return result;

    out_splines(epd_file, shape);

    return 0;
}


static at_string
now (void)
{
  at_string time_string;
  time_t t = time (0);

  XMALLOC (time_string, 26); /* not 25 ! */
  strcpy (time_string, ctime (&t));
  time_string[24] = 0; /* No newline. */

  return time_string;
}
