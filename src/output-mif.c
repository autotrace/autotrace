/* output-mif.c: utility routines for FrameMaker MIF output

   Copyright (C) 2001 Per Grahn

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
#include "output-mif.h"
#include "xstd.h"
#include "autotrace.h"
#include <time.h>
#include <math.h>
#include <string.h>

typedef struct {
  char *tag;
  at_color c;
} ColorT;

typedef struct {
  int llx;
  int lly;
  int urx;
  int ury;
  gfloat dpi;
} BboxT;

BboxT cbox;

/*===========================================================================
  Return a color name based on RGB value
===========================================================================*/
static const char *colorstring(int r, int g, int b)
{
  static char buffer[15];
  if (r == 0 && g == 0 && b == 0)
    return "Black";
  else if (r == 255 && g == 0 && b == 0)
    return "Red";
  else if (r == 0 && g == 255 && b == 0)
    return "Green";
  else if (r == 0 && g == 0 && b == 255)
    return "Blue";
  else if (r == 255 && g == 255 && b == 0)
    return "Yellow";
  else if (r == 255 && g == 0 && b == 255)
    return "Magenta";
  else if (r == 0 && g == 255 && b == 255)
    return "Cyan";
  else if (r == 255 && g == 255 && b == 255)
    return "White";
  else {
    sprintf(buffer, "R%.3dG%.3dB%.3d", r, g, b);
  }
  return buffer;
}

/*===========================================================================
 Convert Bezier Spline
===========================================================================*/
static gfloat bezpnt(gfloat t, gfloat z1, gfloat z2, gfloat z3, gfloat z4)
{
  gfloat temp, t1;
  /* Determine ordinate on Bezier curve at length "t" on curve */
  if (t < (gfloat) 0.0) {
    t = (gfloat) 0.0;
  }
  if (t > (gfloat) 1.0) {
    t = (gfloat) 1.0;
  }
  t1 = ((gfloat) 1.0 - t);
  temp = t1 * t1 * t1 * z1 + (gfloat) 3.0 *t * t1 * t1 * z2 + (gfloat) 3.0 *t * t * t1 * z3 + t * t * t * z4;
  return (temp);
}

/*===========================================================================
  Print a point
===========================================================================*/
static void print_coord(FILE * f, gfloat x, gfloat y)
{
  fprintf(f, "  <Point %.2f %.2f>\n", x * 72.0 / cbox.dpi, (cbox.ury - y + 1) * 72.0 / cbox.dpi);
}

/*===========================================================================
  Main conversion routine
===========================================================================*/
int output_mif_writer(FILE * ps_file, gchar * name, int llx, int lly, int urx, int ury, at_output_opts_type * opts, spline_list_array_type shape, at_msg_func msg_func, gpointer msg_data, gpointer user_data)
{
  unsigned this_list;
  int i;
  ColorT col_tbl[256];
  int n_ctbl = 0;
  at_color curr_color = { 0, 0, 0 };

  cbox.llx = llx;
  cbox.lly = lly;
  cbox.urx = urx;
  cbox.ury = ury;
  cbox.dpi = (gfloat) opts->dpi;

  fprintf(ps_file, "<MIFFile 4.00> #%s\n<Units Upt>\n<ColorCatalog\n", at_version(TRUE));

  for (this_list = 0; this_list < SPLINE_LIST_ARRAY_LENGTH(shape); this_list++) {
    spline_list_type list = SPLINE_LIST_ARRAY_ELT(shape, this_list);
    curr_color = (list.clockwise && shape.background_color != NULL) ? *(shape.background_color) : list.color;

    for (i = 0; i < n_ctbl; i++)
      if (at_color_equal(&curr_color, &col_tbl[i].c))
        break;

    if (i >= n_ctbl) {
      col_tbl[n_ctbl].tag = strdup(colorstring(curr_color.r, curr_color.g, curr_color.b));
      col_tbl[n_ctbl].c = curr_color;
      n_ctbl++;
    }
  }
  for (i = 0; i < n_ctbl; i++) {
    int c, m, y, k;
    c = k = 255 - col_tbl[i].c.r;
    m = 255 - col_tbl[i].c.g;
    if (m < k)
      k = m;
    y = 255 - col_tbl[i].c.b;
    if (y < k)
      k = y;
    c -= k;
    m -= k;
    y -= k;
    fprintf(ps_file, " <Color <ColorTag %s><ColorCyan %d><ColorMagenta %d>" "<ColorYellow %d><ColorBlack %d>>\n", col_tbl[i].tag, c * 100 / 255, m * 100 / 255, y * 100 / 255, k * 100 / 255);
  }
  fprintf(ps_file, ">\n");

  fprintf(ps_file, "<Frame\n" " <Pen 15>\n" " <Fill 15>\n" " <PenWidth  0.2 pt>\n" " <Separation 0>\n" " <BRect  0.0 pt 0.0 pt %.1f pt %.1f pt>\n", (urx - llx) * 72.0 / cbox.dpi, (ury - lly) * 72.0 / cbox.dpi);

  for (this_list = 0; this_list < SPLINE_LIST_ARRAY_LENGTH(shape); this_list++) {
    unsigned this_spline;
    gboolean smooth;

    spline_list_type list = SPLINE_LIST_ARRAY_ELT(shape, this_list);
    spline_type first = SPLINE_LIST_ELT(list, 0);

    for (i = 0; i < n_ctbl; i++)
      if (at_color_equal(&curr_color, &col_tbl[i].c))
        break;

    fprintf(ps_file, " %s\n", (shape.centerline || list.open) ? "<PolyLine <Fill 15><Pen 0>" : "<Polygon <Fill 0><Pen 15>");
    fprintf(ps_file, "  <ObColor `%s'>\n", col_tbl[i].tag);

    print_coord(ps_file, START_POINT(first).x, START_POINT(first).y);
    smooth = FALSE;
    for (this_spline = 0; this_spline < SPLINE_LIST_LENGTH(list); this_spline++) {
      spline_type s = SPLINE_LIST_ELT(list, this_spline);

      if (SPLINE_DEGREE(s) == LINEARTYPE) {
        print_coord(ps_file, END_POINT(s).x, END_POINT(s).y);
      } else {
        gfloat temp;
        gfloat dt = (gfloat) (1.0 / 7.0);
        /*smooth = TRUE; */
        for (temp = dt; fabs(temp - (gfloat) 1.0) > dt; temp += dt) {
          print_coord(ps_file, bezpnt(temp, START_POINT(s).x, CONTROL1(s).x, CONTROL2(s).x, END_POINT(s).x), bezpnt(temp, START_POINT(s).y, CONTROL1(s).y, CONTROL2(s).y, END_POINT(s).y));
        }
      }
    }
    fprintf(ps_file, "  <Smoothed %s>\n", smooth ? "Yes" : "No");
    fprintf(ps_file, " >\n");
  }
  fprintf(ps_file, ">\n");
  return 0;
}
