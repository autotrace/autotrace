/* output-pdf.c: utility routines for PDF output

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
#include "output-pdf.h"
#include "xstd.h"
#include "autotrace.h"
#include <time.h>
#include <math.h>
#include <string.h>

/* Output macros.  */

/* This should be used for outputting a string S on a line by itself.  */
#define OUT_LINE(s)                                 \
  fprintf (pdf_file, "%s\n", s)

/* These output their arguments, preceded by the indentation.  */
#define OUT(...)                                  \
  fprintf (pdf_file, __VA_ARGS__)

/* These macros just output their arguments.  */
#define OUT_REAL(r)	fprintf (pdf_file, r == (lround (r = lround((gfloat)6.0*r)/(gfloat)6.0))				\
                                  ? "%.0f " : "%.3f ", r)

/* For a PostScript command with two real arguments, e.g., lineto.  OP
   should be a constant string.  */
#define OUT_COMMAND2(first, second, op)             \
  do                                                \
    {                                               \
      OUT_REAL (first);                             \
      OUT_REAL (second);                            \
      OUT (op "\n");                         \
    }                                               \
  while (0)

/* For a PostScript command with six real arguments, e.g., curveto.
   Again, OP should be a constant string.  */
#define OUT_COMMAND6(first, second, third, fourth, fifth, sixth, op)	\
  do                                                \
    {                                               \
      OUT_REAL (first);                             \
      OUT_REAL (second);                            \
      OUT (" ");                             \
      OUT_REAL (third);                             \
      OUT_REAL (fourth);                            \
      OUT (" ");                             \
      OUT_REAL (fifth);                             \
      OUT_REAL (sixth);                             \
      OUT (" " op " \n");                    \
    }                                               \
  while (0)

/* This should be used for outputting a string S on a line by itself.  */
#define SOUT_LINE(s)                                 \
  sprintf (temp, "%s\n", s), *length += strlen(temp)

/* These output their arguments, preceded by the indentation.  */
#define SOUT(...)                                  \
  sprintf (temp, __VA_ARGS__), *length += strlen(temp)

/* These macros just output their arguments.  */
#define SOUT_REAL(r)	sprintf (temp, r == (lround (r = lround((gfloat)6.0*r)/(gfloat)6.0))				\
                                  ? "%.0f " : "%.3f ", r), *length += strlen(temp)

/* For a PostScript command with two real arguments, e.g., lineto.  OP
   should be a constant string.  */
#define SOUT_COMMAND2(first, second, op)             \
  do                                                \
    {                                               \
      SOUT_REAL (first);                             \
      SOUT_REAL (second);                            \
      SOUT (op "\n");                         \
    }                                               \
  while (0)

/* For a PostScript command with six real arguments, e.g., curveto.
   Again, OP should be a constant string.  */
#define SOUT_COMMAND6(first, second, third, fourth, fifth, sixth, op)	\
  do                                                \
    {                                               \
      SOUT_REAL (first);                             \
      SOUT_REAL (second);                            \
      SOUT (" ");                             \
      SOUT_REAL (third);                             \
      SOUT_REAL (fourth);                            \
      SOUT (" ");                             \
      SOUT_REAL (fifth);                             \
      SOUT_REAL (sixth);                             \
      SOUT (" " op " \n");                    \
    }                                               \
  while (0)

/* This should be called before the others in this file. It opens the
   output file `OUTPUT_NAME.pdf', and writes some preliminary boilerplate. */

static int output_pdf_header(FILE * pdf_file, gchar * name, int llx, int lly, int urx, int ury)
{
  OUT_LINE("%PDF-1.2");
  OUT_LINE("1 0 obj");
  OUT_LINE("   << /Type /Catalog");
  OUT_LINE("      /Outlines 2 0 R");
  OUT_LINE("      /Pages 3 0 R");
  OUT_LINE("   >>");
  OUT_LINE("endobj");
  OUT_LINE("2 0 obj");
  OUT_LINE("   << /Type /Outlines");
  OUT_LINE("      /Count 0");
  OUT_LINE("   >>");
  OUT_LINE("endobj");
  OUT_LINE("3 0 obj");
  OUT_LINE("   << /Type /Pages");
  OUT_LINE("      /Kids [4 0 R]");
  OUT_LINE("      /Count 1");
  OUT_LINE("   >>");
  OUT_LINE("endobj");
  OUT_LINE("4 0 obj");
  OUT_LINE("   << /Type /Page");
  OUT_LINE("      /Parent 3 0 R");
  OUT("      /MediaBox [%d %d %d %d]\n", llx, lly, urx, ury);
  OUT_LINE("      /Contents 5 0 R");
  OUT_LINE("      /Resources << /ProcSet 6 0 R >>");
  OUT_LINE("   >>");
  OUT_LINE("endobj");

  return 0;
}

/* This should be called after the others in this file. It writes some
   last informations. */

static int output_pdf_tailor(FILE * pdf_file, size_t length, int llx, int lly, int urx, int ury)
{
  char temp[40];
  size_t tmp;

  OUT_LINE("6 0 obj");
  OUT_LINE("   [/PDF]");
  OUT_LINE("endobj");
  OUT_LINE("xref");
  OUT_LINE("0 7");
  OUT_LINE("0000000000 65535 f ");
  OUT_LINE("0000000009 00000 n ");
  OUT_LINE("0000000092 00000 n ");
  OUT_LINE("0000000150 00000 n ");
  OUT_LINE("0000000225 00000 n ");
  sprintf(temp, "%d", llx);
  tmp = 366;
  tmp += (strlen(temp));
  sprintf(temp, "%d", lly);
  tmp += (strlen(temp));
  sprintf(temp, "%d", urx);
  tmp += (strlen(temp));
  sprintf(temp, "%d", ury);
  tmp += (strlen(temp));
  OUT("%010zu 00000 n \n", tmp);
  sprintf(temp, "%zu", length);
  tmp += 50 + length + strlen(temp);
  OUT("%010zu 00000 n \n", tmp);
  OUT_LINE("trailer");
  OUT_LINE("   << /Size 7");
  OUT_LINE("      /Root 1 0 R");
  OUT_LINE("   >>");
  OUT_LINE("startxref");
  OUT("%zu\n", tmp + 25);
  OUT_LINE("%%EOF");

  return 0;
}

/* This outputs the PDF code which produces the shape in
   SHAPE. */

static void out_splines(FILE * pdf_file, spline_list_array_type shape, size_t * length)
{
  char temp[40];
  unsigned this_list;
  spline_list_type list;

  at_color last_color = { 0, 0, 0 };

  for (this_list = 0; this_list < SPLINE_LIST_ARRAY_LENGTH(shape); this_list++) {
    unsigned this_spline;
    spline_type first;

    list = SPLINE_LIST_ARRAY_ELT(shape, this_list);
    first = SPLINE_LIST_ELT(list, 0);

    if (this_list == 0 || !at_color_equal(&list.color, &last_color)) {
      if (this_list > 0) {
        SOUT_LINE((shape.centerline || list.open) ? "S" : "f");
        /* In PDF a Stroke (S) or fill (f) causes an implicit closepath (h) -Paul Sladen */
        /* SOUT_LINE("h"); */
      }
      SOUT("%.3f %.3f %.3f %s\n", (double)list.color.r / 255.0, (double)list.color.g / 255.0, (double)list.color.b / 255.0, (shape.centerline || list.open) ? "RG" : "rg");
      last_color = list.color;
    }
    SOUT_COMMAND2(START_POINT(first).x, START_POINT(first).y, "m");

    for (this_spline = 0; this_spline < SPLINE_LIST_LENGTH(list); this_spline++) {
      spline_type s = SPLINE_LIST_ELT(list, this_spline);

      if (SPLINE_DEGREE(s) == LINEARTYPE)
        SOUT_COMMAND2(END_POINT(s).x, END_POINT(s).y, "l");
      else
        SOUT_COMMAND6(CONTROL1(s).x, CONTROL1(s).y, CONTROL2(s).x, CONTROL2(s).y, END_POINT(s).x, END_POINT(s).y, "c");
    }
  }
  if (SPLINE_LIST_ARRAY_LENGTH(shape) > 0)
    SOUT_LINE((shape.centerline || list.open) ? "S" : "f");

  OUT_LINE("5 0 obj");
  OUT("   << /Length %zu >>\n", *length);
  OUT_LINE("stream");

  last_color.r = 0;
  last_color.g = 0;
  last_color.b = 0;

  for (this_list = 0; this_list < SPLINE_LIST_ARRAY_LENGTH(shape); this_list++) {
    unsigned this_spline;
    spline_type first;

    list = SPLINE_LIST_ARRAY_ELT(shape, this_list);
    first = SPLINE_LIST_ELT(list, 0);

    if (this_list == 0 || !at_color_equal(&list.color, &last_color)) {
      if (this_list > 0) {
        OUT_LINE((shape.centerline || list.open) ? "S" : "f");
        /* In PDF a Stroke (S) or fill (f) causes an implicit closepath (h) -Paul Sladen */
        /* OUT_LINE("h"); */
      }
      OUT("%.3f %.3f %.3f %s\n", (double)list.color.r / 255.0, (double)list.color.g / 255.0, (double)list.color.b / 255.0, (shape.centerline || list.open) ? "RG" : "rg");
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
  }
  if (SPLINE_LIST_ARRAY_LENGTH(shape) > 0)
    OUT_LINE((shape.centerline || list.open) ? "S" : "f");
  OUT_LINE("endstream");
  OUT_LINE("endobj");

}

int output_pdf_writer(FILE * pdf_file, gchar * name, int llx, int lly, int urx, int ury, at_output_opts_type * opts, spline_list_array_type shape, at_msg_func msg_func, gpointer msg_data, gpointer user_data)
{
  int result;
  size_t length = 0;

#ifdef _WINDOWS
  if (pdf_file == stdout) {
    fprintf(stderr, "This driver couldn't write to stdout!\n");
    return -1;
  }
#endif

  result = output_pdf_header(pdf_file, name, llx, lly, urx, ury);
  if (result != 0)
    return result;

  out_splines(pdf_file, shape, &length);

  output_pdf_tailor(pdf_file, length, llx, lly, urx, ury);

  return 0;
}
