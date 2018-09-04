/* output-eps.c: utility routines for PostScript output.

   Copyright (C) 2000, 2001 Martin Weber.

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
#include "output-eps.h"
#include "xstd.h"
#include "autotrace.h"
#include "datetime.h"
#include <string.h>
#include <math.h>
#include <glib.h>

/* Output macros.  */

/* This should be used for outputting a string S on a line by itself.  */
#define OUT_LINE(s)							\
  fprintf (ps_file, "%s\n", s)

/* These output their arguments, preceded by the indentation.  */
#define OUT(...)							\
  fprintf (ps_file, __VA_ARGS__)

/* These macros just output their arguments.  */
#define OUT_REAL(r)	fprintf (ps_file, r == (lround (r = lround((gfloat)6.0*r)/(gfloat)6.0))				\
                                  ? "%.0f " : "%.3f ", r)

/* For a PostScript command with two real arguments, e.g., lineto.  OP
   should be a constant string.  */
#define OUT_COMMAND2(first, second, op)					\
  do									\
    {									\
      OUT_REAL (first);							\
      OUT_REAL (second);						\
      OUT (op "\n");						\
    }									\
  while (0)

/* For a PostScript command with six real arguments, e.g., curveto.
   Again, OP should be a constant string.  */
#define OUT_COMMAND6(first, second, third, fourth, fifth, sixth, op)	\
  do									\
    {									\
      OUT_REAL (first);							\
      OUT_REAL (second);						\
      OUT (" ");							\
      OUT_REAL (third);							\
      OUT_REAL (fourth);						\
      OUT (" ");							\
      OUT_REAL (fifth);							\
      OUT_REAL (sixth);							\
      OUT (" " op " \n");						\
    }									\
  while (0)

/* This should be called before the others in this file.  It opens the
   output file `OUTPUT_NAME.ps', and writes some preliminary boilerplate. */

static int output_eps_header(FILE * ps_file, gchar * name, int llx, int lly, int urx, int ury)
{
  gchar *time;

  OUT_LINE("%!PS-Adobe-3.0 EPSF-3.0");
  OUT("%%%%Creator: Adobe Illustrator by %s\n", at_version(TRUE));
  OUT("%%%%Title: %s\n", name);
  OUT("%%%%CreationDate: %s\n", time = at_time_string());
  OUT("%%%%BoundingBox: %d %d %d %d\n", llx, lly, urx, ury);
  OUT_LINE("%%DocumentData: Clean7Bit");
  OUT_LINE("%%EndComments");

  g_free(time);
  /* Prolog to define Illustrator commands.
   *
   * The s and S commands are not used at the moment and could be
   * removed or commented out.
   *
   * Calling f in *U is not really the right thing to do, but since all
   * paths are simply filled currently, this is the easiest solution.
   */

  OUT_LINE("%%BeginProlog");
  OUT_LINE("/bd { bind def } bind def");
  OUT_LINE("/incompound false def");
  OUT_LINE("/m { moveto } bd");
  OUT_LINE("/l { lineto } bd");
  OUT_LINE("/c { curveto } bd");
  OUT_LINE("/F { incompound not {fill} if } bd");
  OUT_LINE("/f { closepath F } bd");
  OUT_LINE("/S { stroke } bd");
  OUT_LINE("/*u { /incompound true def } bd");
  OUT_LINE("/*U { /incompound false def f} bd");
  OUT_LINE("/k { setcmykcolor } bd"); /* must symbol k for CorelDraw 3/4 */
  OUT_LINE("/K { k } bd");
  OUT_LINE("%%EndProlog");
  OUT_LINE("%%BeginSetup");     /* needed for CorelDraw 3/4 */
  OUT_LINE("%%EndSetup");       /* needed for CorelDraw 3/4 */

  return 0;
}

/* This outputs the PostScript code which produces the shape in
   SHAPE.  */

static void out_splines(FILE * ps_file, spline_list_array_type shape)
{
  unsigned this_list;
  spline_list_type list;

  at_color last_color = { 0, 0, 0 };

  for (this_list = 0; this_list < SPLINE_LIST_ARRAY_LENGTH(shape); this_list++) {
    unsigned this_spline;
    int c, m, y, k;
    spline_type first;

    list = SPLINE_LIST_ARRAY_ELT(shape, this_list);
    first = SPLINE_LIST_ELT(list, 0);

    if (this_list == 0 || !at_color_equal(&list.color, &last_color)) {
      if (this_list > 0)
        OUT_LINE("*U");
      c = k = 255 - list.color.r;
      m = 255 - list.color.g;
      if (m < k)
        k = m;
      y = 255 - list.color.b;
      if (y < k)
        k = y;
      c -= k;
      m -= k;
      y -= k;
      /* symbol k is used for CorelDraw 3/4 compatibility */
      OUT("%.3f %.3f %.3f %.3f %s\n", (double)c / 255.0, (double)m / 255.0, (double)y / 255.0, (double)k / 255.0, (shape.centerline || list.open) ? "K" : "k");
      OUT_LINE("*u");
      last_color = list.color;
    }
    OUT_COMMAND2(START_POINT(first).x, START_POINT(first).y, "m");

    for (this_spline = 0; this_spline < SPLINE_LIST_LENGTH(list); this_spline++) {
      spline_type s = SPLINE_LIST_ELT(list, this_spline);

      if (SPLINE_DEGREE(s) == LINEARTYPE)
        OUT_COMMAND2(END_POINT(s).x, END_POINT(s).y, "l");
      else
        OUT_COMMAND6(CONTROL1(s).x, CONTROL1(s).y, CONTROL2(s).x, CONTROL2(s).y, END_POINT(s).x, END_POINT(s).y, "c");
    }
    if (SPLINE_LIST_ARRAY_LENGTH(shape) > 0)
      OUT_LINE((shape.centerline || list.open) ? "S" : "f");
  }
  if (SPLINE_LIST_ARRAY_LENGTH(shape) > 0)
    OUT_LINE("*U");
}

int output_eps_writer(FILE * ps_file, gchar * name, int llx, int lly, int urx, int ury, at_output_opts_type * opts, spline_list_array_type shape, at_msg_func msg_func, gpointer msg_data, gpointer user_data)
{
  int result;

  result = output_eps_header(ps_file, name, llx, lly, urx, ury);
  if (result != 0)
    return result;

  out_splines(ps_file, shape);

  OUT_LINE("%%Trailer");
  OUT_LINE("%%EOF");
  return 0;
}
