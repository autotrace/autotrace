/* output-plt.c --- output in HPGL (plt) format

   Copyright (C) 2004 Steven P. Hirshman

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

#include <stdio.h>
#include <string.h>
#include "spline.h"
#include "xstd.h"

#define NUM_SPLINES 8
#define WriteInitialize(fp) (fputs("IN;",fp))
#define WriteInitPt(fp,left,bottom,right,top) (fprintf(fp,"IP %d %d %d %d;", left,bottom,right,top))
#define WriteScale(fp,left,right,bottom,top) (fprintf(fp,"SC %d %d %d %d;", left,right,bottom,top))
#define WritePenDown(fp,x,y) (fprintf(fp,"PD%d %d;", X_FLOAT_TO_UI32(x), Y_FLOAT_TO_UI32(y)))
#define WritePenUp(fp,x,y) (fprintf(fp,"PU%d %d;", X_FLOAT_TO_UI32(x), Y_FLOAT_TO_UI32(y)))
#define WriteSelectPen(fp,iColor) (fprintf(fp,"SP%d;",iColor))

#define WDEVPIXEL 1280
#define HDEVPIXEL 1024
#define WDEVMLMTR 320
#define HDEVMLMTR 240

#define SCALE (gfloat) 1.0

#define MAKE_COLREF(r,g,b) (((r) & 0x0FF) | (((g) & 0x0FF) << 8) | (((b) & 0x0FF) << 16))
#define X_FLOAT_TO_UI32(num) ((uint32_t)(num * SCALE))
#define Y_FLOAT_TO_UI32(num) ((uint32_t)(num * SCALE))

/*
  Searches color by closest rgb values (distance**2 of 2 3D points)
  returns: color value
*/
static int GetIndexFromRGBValue(int red, int green, int blue)
{
  int i, index = 0;
  int psav = 3 * (0xFF * 0xFF), pnew, px, py, pz;
  int nred, ngreen, nblue;
  const at_color HPGL_COLORS[] = { {0, 0, 0}, {0, 0, 0}, {0xFF, 0, 0},
  {0, 0xFF, 0}, {0xFF, 0xFF, 0}, {0, 0, 0xFF},
  {0xB8, 0, 0x80}, {0, 0xFF, 0xFF}, {0xFF, 0x84, 00}
  };
  for (i = 1; i < sizeof(HPGL_COLORS) / sizeof(at_color); i++) {
    nred = HPGL_COLORS[i].r;
    ngreen = HPGL_COLORS[i].g;
    nblue = HPGL_COLORS[i].b;

    /* compute shortest distance between rgb colors */
    px = (red - nred) * (red - nred);
    py = (green - ngreen) * (green - ngreen);
    pz = (blue - nblue) * (blue - nblue);
    pnew = px + py + pz;
    if (pnew < psav) {
      psav = pnew;
      index = i;
    }
  }

  return index;
}

static void GetSplinePts(at_real_coord * BezierPts, at_real_coord * Splines, int numsplines)
{
  /*first point == P0; last point = BezierPts[3]
     P1, P2 are the control points
     use formula for points along Bezier:

     G(t) = a + b t + c t**2 + d t**3,   0 <= t <= 1

     a = P0
     b = 3(P1 - P0)
     c = 3(P2 - P1) - b
     d = P3 - P0 - (b + c)
   */

  gfloat t;
  int count;
  at_real_coord a, b, c, d;

  a.x = BezierPts[0].x;
  b.x = 3 * (BezierPts[1].x - BezierPts[0].x);
  c.x = 3 * (BezierPts[2].x - BezierPts[1].x) - b.x;
  d.x = (BezierPts[3].x - BezierPts[0].x) - (b.x + c.x);

  a.y = BezierPts[0].y;
  b.y = 3 * (BezierPts[1].y - BezierPts[0].y);
  c.y = 3 * (BezierPts[2].y - BezierPts[1].y) - b.y;
  d.y = (BezierPts[3].y - BezierPts[0].y) - (b.y + c.y);

  //Assume Splines was allocated externally with enough space
  for (count = 0; count < numsplines; count++) {
    t = (float)count / (numsplines - 1);
    if (count == 0)
      Splines[count] = BezierPts[0];
    else if (count == (numsplines - 1))
      Splines[count] = BezierPts[3];
    else {
      Splines[count].x = a.x + t * (b.x + t * (c.x + t * d.x));
      Splines[count].y = a.y + t * (b.y + t * (c.y + t * d.y));
    }
  }
}

static void WriteBezier(FILE * fdes, spline_type sp1, at_real_coord * BeginPt)
{
  //Bezier from begin point
  at_real_coord Splines[NUM_SPLINES];
  at_real_coord BezierPts[4], LastPoint;
  int i;
  BezierPts[0] = (*BeginPt);
  BezierPts[1] = CONTROL1(sp1);
  BezierPts[2] = CONTROL2(sp1);
  BezierPts[3] = END_POINT(sp1);

  GetSplinePts(BezierPts, Splines, NUM_SPLINES);

  for (i = 1; i < NUM_SPLINES; i++) {
    LastPoint = Splines[i];
    WritePenDown(fdes, LastPoint.x, LastPoint.y);
  }

  *BeginPt = LastPoint;
}

static void OutputPlt(FILE * fdes, int llx, int lly, int urx, int ury, spline_list_array_type shape)
{
/*
    Parses the spline data and writes out HPGL (*.PLT) formatted file
*/
  const int plu_inch = 1016;
  const int LOGXPIXELS = 120;

  unsigned int this_list, this_spline;
  unsigned char red, green, blue;
  uint32_t last_color = 0xFFFFFFFF, curr_color;
  spline_list_type curr_list;
  spline_type curr_spline;
  int last_degree, index;
  float Scale = ((float)plu_inch) / LOGXPIXELS;
  at_real_coord StartPoint;
  at_real_coord LastPoint;

  if (fdes == NULL)
    return;

  //output PLT header and sizing information
  WriteInitialize(fdes);
  //            CView *pView=GetNextView(pos);
  //            int LOGXPIXELS = pView->GetDC()->GetDeviceCaps(LOGPIXELSX);
  WriteInitPt(fdes, (uint32_t) (Scale * llx), (uint32_t) (Scale * lly), (uint32_t) (Scale * urx), (uint32_t) (Scale * ury));
  WriteScale(fdes, llx, urx, lly, ury);

  LastPoint.x = 0;
  LastPoint.y = 0;
  StartPoint = LastPoint;

  // visit each spline-list
  for (this_list = 0; this_list < SPLINE_LIST_ARRAY_LENGTH(shape); this_list++) {
    curr_list = SPLINE_LIST_ARRAY_ELT(shape, this_list);

    // output pen selection
    curr_color = MAKE_COLREF(curr_list.color.r, curr_list.color.g, curr_list.color.b);
    if (this_list == 0 || curr_color != last_color) {
      //set new color
      red = curr_list.color.r, green = curr_list.color.g, blue = curr_list.color.b;
      index = GetIndexFromRGBValue(red, green, blue);
      WriteSelectPen(fdes, index);
      last_color = curr_color;
    }
    //output MoveTo first point
    curr_spline = SPLINE_LIST_ELT(curr_list, 0);
    LastPoint = START_POINT(curr_spline);
    WritePenUp(fdes, LastPoint.x, LastPoint.y);
    StartPoint = LastPoint;

    //visit each spline
    for (this_spline = 0; this_spline < SPLINE_LIST_LENGTH(curr_list); this_spline++) {
      curr_spline = SPLINE_LIST_ELT(curr_list, this_spline);
      last_degree = ((int)SPLINE_DEGREE(curr_spline));

      switch ((polynomial_degree) last_degree) {
      case LINEARTYPE:
        //output Line
        LastPoint = END_POINT(curr_spline);
        WritePenDown(fdes, LastPoint.x, LastPoint.y);
        break;
      default:
        //output Bezier curve
        WriteBezier(fdes, curr_spline, &LastPoint);
        break;
      }
    }
  }

  WritePenUp(fdes, LastPoint.x, LastPoint.y);

}

//PLT output

int output_plt_writer(FILE * file, gchar * name, int llx, int lly, int urx, int ury, at_output_opts_type * opts, at_spline_list_array_type shape, at_msg_func msg_func, gpointer msg_data, gpointer user_data)
{
#ifdef _WINDOWS
  if (file == stdout) {
    fprintf(stderr, "This driver couldn't write to stdout!\n");
    return -1;
  }
#endif

  /* Output PLT */
  OutputPlt(file, llx, lly, urx, ury, shape);

  return 0;
}
