/* 
output-p2e.c: utility routines for pstoedit intermediate output. 
use the generated file to run "pstoedit -bo -f format input output"
Initial author: Wolfgang Glunz - wglunz@geocities.com
Copyright (C) 2000 Wolfgang Glunz, Martin Weber 
*/

#include "types.h"
#include "spline.h"
#include "output-p2e.h"

#if old
#include "xmem.h"
#include <time.h>
#include <string.h>
#include <malloc.h>
#endif

#define SIGN(x) ((x) > 0 ? 1 : (x) < 0 ? -1 : 0)
#define ROUND(x) ((int) ((int) (x) + .5 * SIGN (x)))

/* Output macros.  */

/* This should be used for outputting a string S on a line by itself.  */
#define OUT_LINE(s)							\
  fprintf (ps_file, "%s\n", s)

/* These output their arguments, preceded by the indentation.  */
#define OUT1(s, e)							\
  fprintf (ps_file, s, e)

#define OUT2(s, e1, e2)							\
  fprintf (ps_file, s, e1, e2)

#define OUT4(s, e1, e2, e3, e4)						\
  fprintf (ps_file, s, e1, e2, e3, e4)

#define OUT5(s, e1, e2, e3, e4, e5)					\
  fprintf (ps_file, s, e1, e2, e3, e4, e5)

/* These macros just output their arguments.  */
#define OUT_STRING(s)	fprintf (ps_file, "%s", s)
#define OUT_REAL(r)	fprintf (ps_file,				\
                                 r == ROUND (r) ? "%.0f " : "%.3f ", r)

/* For a PostScript command with two real arguments, e.g., lineto.  OP
   should be a constant string.  */
#define OUT_COMMAND2(first, second, op)					\
  do									\
	{                                          \
      OUT_STRING (" ");							\
      OUT_REAL (first);							\
      OUT_REAL (second);						\
      OUT_STRING (op "\n");						\
    }									\
  while (0)

/* For a PostScript command with six real arguments, e.g., curveto.
   Again, OP should be a constant string.  */
#define OUT_COMMAND6(first, second, third, fourth, fifth, sixth, op)	\
  do									\
    {									\
	  OUT_STRING (" ");							\
      OUT_REAL (first);							\
      OUT_REAL (second);						\
      OUT_STRING (" ");							\
      OUT_REAL (third);							\
      OUT_REAL (fourth);						\
      OUT_STRING (" ");							\
      OUT_REAL (fifth);							\
      OUT_REAL (sixth);							\
      OUT_STRING (" " op "\n");					\
    }									\
   while (0)

/* This should be called before the others in this file.  It opens the
   output file `OUTPUT_NAME.ps', and writes some preliminary boilerplate. */

static int output_p2e_header(FILE* ps_file, string name,
			     int llx, int lly, int urx, int ury)
{


OUT_LINE ("%!PS-Adobe-3.0");
OUT_LINE ("%%Creator: pstoedit");
OUT_LINE ("%%Pages: (atend)");
OUT_LINE ("%%EndComments");
OUT_LINE ("%%BeginProlog");
OUT_LINE ("/setPageSize { pop pop } def");
OUT_LINE ("/ntranslate { neg exch neg exch translate } def");
OUT_LINE ("/setshowparams { pop pop pop} def");
OUT_LINE ("/backendconstraints { pop pop } def");
OUT_LINE ("/pstoedit.newfont { 80 string cvs  findfont  dup length dict begin {1 index /FID ne {def} {pop pop} ifelse} forall  /Encoding ISOLatin1Encoding def   dup 80 string cvs /FontName exch def  currentdict end  definefont pop } def");
OUT_LINE ("/imagestring 1 string def");
OUT_LINE ("%%EndProlog");
OUT_LINE ("%%BeginSetup");
OUT_LINE ("% textastext doflatten backendconstraints  "); 
OUT_LINE ("0 0 backendconstraints");
OUT_LINE ("%%EndSetup");
OUT_LINE ("%%Page: 1 1");
OUT_LINE ("% 1 pathnumber");
OUT_LINE (" 612 792 setPageSize");
OUT_LINE (" 0 setlinecap");
OUT_LINE (" 10.0 setmiterlimit");
OUT_LINE (" 0 setlinejoin");
OUT_LINE (" [ ] 0.0 setdash");
OUT_LINE (" 1.0 setlinewidth");
OUT_LINE (" 0.0 0.0 0.0 setrgbcolor");

  return 0;
}

/* This outputs the PostScript code which produces the shape in
   SHAPE.  */

static void
out_splines (FILE * ps_file, spline_list_array_type shape)
{
  unsigned this_list;

  OUT_LINE ("% filledpath");
  OUT_LINE ("newpath");

  for (this_list = 0; this_list < SPLINE_LIST_ARRAY_LENGTH (shape);
       this_list++)
    {
      unsigned this_spline;
      spline_list_type list = SPLINE_LIST_ARRAY_ELT (shape, this_list);
      spline_type first = SPLINE_LIST_ELT (list, 0);

      OUT_COMMAND2 (START_POINT (first).x, START_POINT (first).y, "moveto");

      for (this_spline = 0; this_spline < SPLINE_LIST_LENGTH (list);
           this_spline++)
        {
          spline_type s = SPLINE_LIST_ELT (list, this_spline);

          if (SPLINE_DEGREE (s) == LINEARTYPE)
            OUT_COMMAND2 (END_POINT (s).x, END_POINT (s).y, "lineto");
          else
            OUT_COMMAND6 (CONTROL1 (s).x, CONTROL1 (s).y,
                          CONTROL2 (s).x, CONTROL2 (s).y,
                          END_POINT (s).x, END_POINT (s).y,
                          "curveto");
        }
      OUT_LINE (" closepath");
    }
	OUT_LINE ("fill");
}


int output_p2e_writer(FILE* ps_file, string name,
		      int llx, int lly, int urx, int ury,
		      spline_list_array_type shape)
{
    int result;

    result = output_p2e_header(ps_file, name, llx, lly, urx, ury);
    if (result != 0)  return result;

    out_splines(ps_file, shape);

    OUT_LINE ("% normal end reached by pstoedit.pro");
    OUT_LINE ("%%Trailer");
    OUT_LINE ("%%Pages: 1");
    OUT_LINE ("%%EOF");

    return 0;
}
/* version 0.26 */
