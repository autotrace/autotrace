/*
 * Copyright (C) 1999, 2000, 2001 Martin Weber
 * SPDX-FileCopyrightText: © 2001 Per Grahn
 * SPDX-FileCopyrightText: © 2001-2002 Martin Weber
 * SPDX-FileCopyrightText: © 2001-2003 Masatake YAMATO
 * SPDX-FileCopyrightText: © 2011 Edgar Antonio Palma de la Cruz
 * SPDX-FileCopyrightText: © 2017-2025 Peter Lemenkov
 * SPDX-FileCopyrightText: © 2020 Han Mertens
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "spline.h"
#include "color.h"
#include "output-pdf.h"
#include "autotrace.h"
#include <math.h>
#include <stdarg.h>
#include <string.h>

/* Everything is written through pdf_out() so that the number of bytes
   written so far is known: the cross-reference table at the end of the
   file lists the byte offset of every object.  */
typedef struct {
  FILE *file;
  size_t pos;
  size_t offset[7]; /* byte offset of "N 0 obj" for N = 1..6 */
} pdf_writer;

static void pdf_out(pdf_writer *pdf, const char *format, ...) G_GNUC_PRINTF(2, 3);

static void pdf_out(pdf_writer *pdf, const char *format, ...)
{
  va_list args;
  int n;

  va_start(args, format);
  n = vfprintf(pdf->file, format, args);
  va_end(args);
  if (n > 0)
    pdf->pos += n;
}

static void pdf_begin_object(pdf_writer *pdf, int number)
{
  pdf->offset[number] = pdf->pos;
  pdf_out(pdf, "%d 0 obj\n", number);
}

/* Output macros.  */

/* This should be used for outputting a string S on a line by itself.  */
#define OUT_LINE(s) pdf_out(pdf, "%s\n", s)

/* These output their arguments, preceded by the indentation.  */
#define OUT(...) pdf_out(pdf, __VA_ARGS__)

/* The content stream is built in memory first, because its length has to
   be written before it.  */
#define SOUT_LINE(s) g_string_append_printf(stream, "%s\n", s)
#define SOUT(...) g_string_append_printf(stream, __VA_ARGS__)
#define SOUT_REAL(r)                                                                               \
  g_string_append_printf(stream, (r) == lround(r) ? "%.0f " : "%.3f ", (double)(r))

/* For a PostScript command with two real arguments, e.g., lineto.  OP
   should be a constant string.  */
#define SOUT_COMMAND2(first, second, op)                                                           \
  do {                                                                                             \
    SOUT_REAL(first);                                                                              \
    SOUT_REAL(second);                                                                             \
    SOUT(op "\n");                                                                                 \
  } while (0)

/* For a PostScript command with six real arguments, e.g., curveto.
   Again, OP should be a constant string.  */
#define SOUT_COMMAND6(first, second, third, fourth, fifth, sixth, op)                              \
  do {                                                                                             \
    SOUT_REAL(first);                                                                              \
    SOUT_REAL(second);                                                                             \
    SOUT(" ");                                                                                     \
    SOUT_REAL(third);                                                                              \
    SOUT_REAL(fourth);                                                                             \
    SOUT(" ");                                                                                     \
    SOUT_REAL(fifth);                                                                              \
    SOUT_REAL(sixth);                                                                              \
    SOUT(" " op " \n");                                                                            \
  } while (0)

/* This should be called before the others in this file.  It writes some
   preliminary boilerplate: the catalog, outlines, pages and page objects. */

static void output_pdf_header(pdf_writer *pdf, int llx, int lly, int urx, int ury)
{
  OUT_LINE("%PDF-1.2");
  pdf_begin_object(pdf, 1);
  OUT_LINE("   << /Type /Catalog");
  OUT_LINE("      /Outlines 2 0 R");
  OUT_LINE("      /Pages 3 0 R");
  OUT_LINE("   >>");
  OUT_LINE("endobj");
  pdf_begin_object(pdf, 2);
  OUT_LINE("   << /Type /Outlines");
  OUT_LINE("      /Count 0");
  OUT_LINE("   >>");
  OUT_LINE("endobj");
  pdf_begin_object(pdf, 3);
  OUT_LINE("   << /Type /Pages");
  OUT_LINE("      /Kids [4 0 R]");
  OUT_LINE("      /Count 1");
  OUT_LINE("   >>");
  OUT_LINE("endobj");
  pdf_begin_object(pdf, 4);
  OUT_LINE("   << /Type /Page");
  OUT_LINE("      /Parent 3 0 R");
  OUT("      /MediaBox [%d %d %d %d]\n", llx, lly, urx, ury);
  OUT_LINE("      /Contents 5 0 R");
  OUT_LINE("      /Resources << /ProcSet 6 0 R >>");
  OUT_LINE("   >>");
  OUT_LINE("endobj");
}

/* This outputs the content stream (object 5) which draws the shape in
   SHAPE.  */

static void output_pdf_content(pdf_writer *pdf, spline_list_array_type shape)
{
  GString *stream = g_string_new(NULL);
  unsigned this_list;
  spline_list_type list;

  at_color last_color = {0, 0, 0};

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
      SOUT("%.3f %.3f %.3f %s\n", (double)list.color.r / 255.0, (double)list.color.g / 255.0,
           (double)list.color.b / 255.0, (shape.centerline || list.open) ? "RG" : "rg");
      last_color = list.color;
    }
    SOUT_COMMAND2(START_POINT(first).x, START_POINT(first).y, "m");

    for (this_spline = 0; this_spline < SPLINE_LIST_LENGTH(list); this_spline++) {
      spline_type s = SPLINE_LIST_ELT(list, this_spline);

      if (SPLINE_DEGREE(s) == LINEARTYPE)
        SOUT_COMMAND2(END_POINT(s).x, END_POINT(s).y, "l");
      else
        SOUT_COMMAND6(CONTROL1(s).x, CONTROL1(s).y, CONTROL2(s).x, CONTROL2(s).y, END_POINT(s).x,
                      END_POINT(s).y, "c");
    }
  }
  if (SPLINE_LIST_ARRAY_LENGTH(shape) > 0)
    SOUT_LINE((shape.centerline || list.open) ? "S" : "f");

  pdf_begin_object(pdf, 5);
  OUT("   << /Length %zu >>\n", stream->len);
  OUT_LINE("stream");
  fwrite(stream->str, 1, stream->len, pdf->file);
  pdf->pos += stream->len;
  OUT_LINE("endstream");
  OUT_LINE("endobj");

  g_string_free(stream, TRUE);
}

/* This should be called after the others in this file.  It writes the
   procedure set, the cross-reference table and the trailer.  */

static void output_pdf_tailor(pdf_writer *pdf)
{
  size_t xref;
  int number;

  pdf_begin_object(pdf, 6);
  OUT_LINE("   [/PDF]");
  OUT_LINE("endobj");

  xref = pdf->pos;
  OUT_LINE("xref");
  OUT_LINE("0 7");
  OUT_LINE("0000000000 65535 f ");
  for (number = 1; number <= 6; number++)
    OUT("%010zu 00000 n \n", pdf->offset[number]);
  OUT_LINE("trailer");
  OUT_LINE("   << /Size 7");
  OUT_LINE("      /Root 1 0 R");
  OUT_LINE("   >>");
  OUT_LINE("startxref");
  OUT("%zu\n", xref);
  OUT_LINE("%%EOF");
}

int output_pdf_writer(FILE *pdf_file, gchar *name, int llx, int lly, int urx, int ury,
                      at_output_opts_type *opts, spline_list_array_type shape, at_msg_func msg_func,
                      gpointer msg_data, gpointer user_data)
{
  pdf_writer writer = {pdf_file, 0, {0}};
  pdf_writer *pdf = &writer;

#ifdef _WINDOWS
  if (pdf_file == stdout) {
    fprintf(stderr, "This driver couldn't write to stdout!\n");
    return -1;
  }
#endif

  output_pdf_header(pdf, llx, lly, urx, ury);
  output_pdf_content(pdf, shape);
  output_pdf_tailor(pdf);

  return 0;
}
