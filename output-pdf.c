/* output-pdf.c: utility routines for PDF output */

#include "ptypes.h"
#include "spline.h"
#include "color.h"
#include "output-pdf.h"
#include "xstd.h"
#include "autotrace.h"
#include <time.h>
#include <string.h>

extern string version_string;

static string now (void);

#define SIGN(x) ((x) > 0 ? 1 : (x) < 0 ? -1 : 0)
#define ROUND(x) ((int) ((int) (x) + .5 * SIGN (x)))

/* Output macros.  */

/* This should be used for outputting a string S on a line by itself.  */
#define OUT_LINE(s)                                 \
  fprintf (file, "%s\n", s)

/* These output their arguments, preceded by the indentation.  */
#define OUT1(s, e)                                  \
  fprintf (file, s, e)

#define OUT2(s, e1, e2)                             \
  fprintf (file, s, e1, e2)

#define OUT3(s, e1, e2, e3)                         \
  fprintf (file, s, e1, e2, e3)

#define OUT4(s, e1, e2, e3, e4)                     \
  fprintf (file, s, e1, e2, e3, e4)

/* These macros just output their arguments.  */
#define OUT_STRING(s)	fprintf (file, "%s", s)
#define OUT_REAL(r)	fprintf (file, r == (ROUND (r = ROUND((real)6.0*r)/(real)6.0))				\
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

/* This should be called before the others in this file. It opens the
   output file `OUTPUT_NAME.pdf', and writes some preliminary boilerplate. */

static int output_pdf_header(FILE* file, string name,
			     int llx, int lly, int urx, int ury)
{
  OUT_LINE ("%PDF-1.2");
  OUT_LINE ("1 0 obj");
  OUT_LINE ("<< /Type /Catalog");
  OUT_LINE ("/Outlines 2 0 R");
  OUT_LINE ("/Pages 3 0 R");
  OUT_LINE (">>");
  OUT_LINE ("endobj");
  OUT_LINE ("2 0 obj");
  OUT_LINE ("<< /Type /Outlines");
  OUT_LINE ("/Count 0");
  OUT_LINE (">>");
  OUT_LINE ("endobj");
  OUT_LINE ("3 0 obj");
  OUT_LINE ("<< /Type /Pages");
  OUT_LINE ("/Kids [4 0 R]");
  OUT_LINE ("/Count 1");
  OUT_LINE (">>");
  OUT_LINE ("endobj");
  OUT_LINE ("4 0 obj");
  OUT_LINE ("<< /Type /Page");
  OUT_LINE ("/Parent 3 0 R");
  OUT4     ("/MediaBox [%d %d %d %d]\n", llx, lly, urx, ury);
  OUT_LINE ("/Contents 5 0 R");
  OUT_LINE ("/Resources << /ProcSet 6 0 R >>");
  OUT_LINE (">>");
  OUT_LINE ("endobj");

  return 0;
}


/* This should be called after the others in this file. It writes some
   last informations. */

static int output_pdf_tailor(FILE* file)
{
  OUT_LINE ("6 0 obj");
  OUT_LINE ("[/PDF]");
  OUT_LINE ("endobj");
  OUT_LINE ("trailer");
  OUT_LINE ("<< /Size 7");
  OUT_LINE ("/Root 1 0 R");
  OUT_LINE (">>");
  OUT_LINE ("%%EOF");

  return 0;
}


/* This outputs the PDF code which produces the shape in
   SHAPE. */

static void
out_splines (FILE *pdf_file, spline_list_array_type shape)
{
  FILE *file, *tempfile = tmpfile ();
  unsigned this_list;

  color_type last_color = {0,0,0};
  file = tempfile;

  for (this_list = 0; this_list < SPLINE_LIST_ARRAY_LENGTH (shape);
       this_list++)
    {
      unsigned this_spline;

      spline_list_type list = SPLINE_LIST_ARRAY_ELT (shape, this_list);
      spline_type first = SPLINE_LIST_ELT (list, 0);

      if (this_list == 0 || !COLOR_EQUAL(list.color, last_color))
        {
          if (this_list > 0)
              OUT_LINE("h");
          OUT4 ("%f %f %f %s\n", (double) list.color.r / 255.0,
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
      OUT_LINE ((shape.centerline || list.open) ? "S" : "f");

    }

  file = pdf_file;

  OUT_LINE ("5 0 obj");
  OUT1 ("<< /Length %d >>\n", ftell(tempfile));
  OUT_LINE ("stream");

  last_color.r = 0;
  last_color.g = 0;
  last_color.b = 0;

  for (this_list = 0; this_list < SPLINE_LIST_ARRAY_LENGTH (shape);
       this_list++)
    {
      unsigned this_spline;

      spline_list_type list = SPLINE_LIST_ARRAY_ELT (shape, this_list);
      spline_type first = SPLINE_LIST_ELT (list, 0);

      if (this_list == 0 || !COLOR_EQUAL(list.color, last_color))
        {
          if (this_list > 0)
              OUT_LINE("h");
          OUT4 ("%f %f %f %s\n", (double) list.color.r / 255.0,
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
      OUT_LINE ((shape.centerline || list.open) ? "S" : "f");

    }

  OUT_LINE ("endstream");
  OUT_LINE ("endobj");

}


int output_pdf_writer(FILE* pdf_file, string name,
		      int llx, int lly, int urx, int ury,
		      spline_list_array_type shape)
{
    int result;

    result = output_pdf_header(pdf_file, name, llx, lly, urx, ury);
    if (result != 0)
	return result;

    out_splines(pdf_file, shape);

	output_pdf_tailor(pdf_file);

    return 0;
}
