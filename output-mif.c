/* output-mif.c: utility routines for FrameMaker MIF output.
   Author: Per Grahn, 2001 */

#include "types.h"
#include "spline.h"
#include "color.h"
#include "output-mif.h"
#include "xstd.h"
#include "autotrace.h"
#include <time.h>
#include <math.h>
#include <string.h>

extern string version_string;

typedef struct {
  char *tag;
  color_type c;
} ColorT;

typedef struct {
  int llx;
  int lly;
  int urx;
  int ury;
  real dpi;
} BboxT;

BboxT cbox;

/*===========================================================================
  Return a color name based on RGB value
===========================================================================*/
static const char * colorstring(int r, int g, int b)
{
  static char buffer[15];
  if( r==0 && g==0 && b==0 )
    return "Black";
  else if( r==255 && g==0 && b==0 )
    return "Red";
  else if( r==0 && g==255 && b==0 )
    return "Green";
  else if( r==0 && g==0 && b==255 )
    return "Blue";
  else if( r==255 && g==255 && b==0 )
    return "Yellow";
  else if( r==255 && g==0 && b==255 )
    return "Magenta";
  else if( r==0 && g==255 && b==255 )
    return "Cyan";
  else if( r==255 && g==255 && b==255 )
    return "White";
  else {
    sprintf(buffer,"R%.3dG%.3dB%.3d", r, g, b);
  }
  return buffer;
}

/*===========================================================================
 Convert Bezier Spline
===========================================================================*/
static real bezpnt(real t, real z1, real z2, real z3, real z4)
{
  real temp, t1;
  /* Determine ordinate on Bezier curve at length "t" on curve */
  if (t < (real) 0.0) { t = (real) 0.0; }
  if (t > (real) 1.0) { t = (real) 1.0; }
  t1 = ((real) 1.0 - t);
  temp = t1*t1*t1*z1 + (real)3.0*t*t1*t1*z2 + (real)3.0*t*t*t1*z3 + t*t*t*z4;
  return(temp);
}

/*===========================================================================
  Print a point
===========================================================================*/
static void print_coord(FILE* f, real x, real y)
{
  fprintf(f, "  <Point %.2f %.2f>\n",
    x*72.0/cbox.dpi, (cbox.ury-y+1)*72.0/cbox.dpi);
}

/*===========================================================================
  Main conversion routine
===========================================================================*/
int output_mif_writer(FILE* ps_file, string name,
		      int llx, int lly, int urx, int ury, int dpi,
		      spline_list_array_type shape)
{
  unsigned this_list;
  int i;
  ColorT col_tbl[256];
  int n_ctbl = 0;

  cbox.llx = llx;
  cbox.lly = lly;
  cbox.urx = urx;
  cbox.ury = ury;
  cbox.dpi = (real) dpi;

  fprintf(ps_file, "<MIFFile 4.00> #%s\n<Units Upt>\n<ColorCatalog\n", version_string);

  for( this_list=0; this_list < SPLINE_LIST_ARRAY_LENGTH(shape); this_list++ ){
    spline_list_type list = SPLINE_LIST_ARRAY_ELT (shape, this_list);
    
    for( i=0; i<n_ctbl; i++ )
      if( COLOR_EQUAL(list.color, col_tbl[i].c) ) break;

    if( i >= n_ctbl ){
      col_tbl[n_ctbl].tag = strdup(colorstring(list.color.r, list.color.g, list.color.b));
      col_tbl[n_ctbl].c = list.color;
      n_ctbl++;
    }
  }
  for( i=0; i<n_ctbl; i++ ){
    int c, m, y, k;
    c = k = 255 - col_tbl[i].c.r;
    m = 255 - col_tbl[i].c.g;
    if( m < k ) k = m;
    y = 255 - col_tbl[i].c.b;
    if( y < k ) k = y;
    c -= k;
    m -= k;
    y -= k;
    fprintf(ps_file,
      " <Color <ColorTag %s><ColorCyan %d><ColorMagenta %d>"
      "<ColorYellow %d><ColorBlack %d>>\n",
      col_tbl[i].tag, c*100/255, m*100/255, y*100/255, k*100/255);
  }
  fprintf(ps_file, ">\n");

  fprintf(ps_file, "<Frame\n"
    " <Pen 15>\n"
    " <Fill 15>\n"
    " <PenWidth  0.2 pt>\n"
    " <Separation 0>\n"
    " <BRect  0.0 pt 0.0 pt %.1f pt %.1f pt>\n",
    (urx-llx)*72.0/cbox.dpi, (ury-lly)*72.0/cbox.dpi);


  for( this_list=0; this_list < SPLINE_LIST_ARRAY_LENGTH(shape); this_list++ ){
    unsigned this_spline;
    bool smooth;

    spline_list_type list = SPLINE_LIST_ARRAY_ELT (shape, this_list);
    spline_type first = SPLINE_LIST_ELT (list, 0);

    for( i=0; i<n_ctbl; i++ )
      if( COLOR_EQUAL(list.color, col_tbl[i].c) ) break;

    fprintf(ps_file, " %s\n",
      (shape.centerline || list.open) ? 
      "<PolyLine <Fill 15><Pen 0>" :
      "<Polygon <Fill 0><Pen 15>");
    fprintf(ps_file, "  <ObColor `%s'>\n", col_tbl[i].tag);

    print_coord(ps_file, START_POINT (first).x, START_POINT (first).y);
    smooth = false;
    for( this_spline=0; this_spline < SPLINE_LIST_LENGTH(list); this_spline++ ){
      spline_type s = SPLINE_LIST_ELT (list, this_spline);

      if( SPLINE_DEGREE (s) == LINEARTYPE ){
        print_coord(ps_file, END_POINT(s).x, END_POINT(s).y);
      } else {
        real temp;
	real dt = (real) (1.0/7.0);
	/*smooth = true;*/
	for( temp=dt; fabs(temp-(real)1.0)<dt; temp+=dt ){
	  print_coord(ps_file,
	    bezpnt(temp,START_POINT(s).x,CONTROL1(s).x,CONTROL2(s).x,END_POINT(s).x),
	    bezpnt(temp,START_POINT(s).y,CONTROL1(s).y,CONTROL2(s).y,END_POINT(s).y));
	}
      }
    }
    fprintf(ps_file, "  <Smoothed %s>\n", smooth ? "Yes": "No");
    fprintf(ps_file, " >\n");
  }
  fprintf(ps_file, ">\n");
  return 0;
}
