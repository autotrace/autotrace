/* output-dxf.c: utility routines for DXF output. */

/* mail comments and suggestions to kovar@t-online.de */

#include "ptypes.h"
#include "spline.h"
#include "color.h"
#include "output-dxf.h"
#include "xstd.h"
#include "autotrace.h"
#include <math.h>
#include <time.h>
#include <string.h>

#define SIGN(x) ((x) > 0 ? 1 : (x) < 0 ? -1 : 0)
#define ROUND(x) ((int) ((int) (x) + .5 * SIGN (x)))

/* Output macros.  */

/* This should be used for outputting a string S on a line by itself.  */
#define OUT_LINE(s)							\
  fprintf (ps_file, "%s\n", s)


/**************************************************************************************
Definitions for spline to line transformation
**************************************************************************************/

typedef enum { NATURAL, TANGENT, PERIODIC, CYCLIC, ANTICYCLIC
             } SPLINE_END_TYPE;


#define MAX_VERTICES 10000
#define RESOLUTION   10000  /* asume no pixels bigger than 1000000.0 */
#define RADIAN                  57.295779513082

typedef struct xypnt_t { 
                         int xp, yp;
                       } xypnt;


typedef struct xypnt_point_t { 
                               xypnt point;
                               struct xypnt_point_t *next_point;
                             } xypnt_point_rec;

typedef struct xypnt_head_t { 
                              xypnt_point_rec *first_point, *last_point,
                                                *current_point;
                              struct xypnt_head_t *next_head;
                            } xypnt_head_rec;

typedef struct Colors_t { 
  int red, green, blue;
} Colors;


/* RGB to layer transformation table follows */

#define MAX_COLORS 9

struct Colors_t dxftable[MAX_COLORS] =
{
/*   0 */  {255,   0,   0},
/*   1 */  {255, 255,   0},
/*   2 */  {  0, 255,   0},
/*   3 */  {  0, 255, 255},
/*   4 */  {  0,   0, 255},
/*   5 */  {255,   0, 255},
/*   6 */  {255, 255, 255},
/*   7 */  {100, 100, 100},
/*   8 */  {192, 192, 192}
};

/******************************************************************************
* Moves the current_point pointer to the next point in the list.
* If current_point is at last point then it becomes NULL.
* finished is 1 if coord_point has not been set, that is current_point is NULL.
*/
void xypnt_next_pnt (xypnt_head_rec *head_xypnt     /*  */,
                     xypnt *coord_point             /*  */,
                     char *finished                 /*  */)
{
  if (head_xypnt && head_xypnt->current_point)
    {
      head_xypnt->current_point = head_xypnt->current_point->next_point;
      if (head_xypnt->current_point == NULL)
        *finished = 1;
      else
        { 
          *coord_point = head_xypnt->current_point->point;
          *finished = 0;
        }
    }
  else 
    *finished = 1;
}


/******************************************************************************
* Moves the current_point pointer to the begining of the list 
*/
void xypnt_first_pnt (xypnt_head_rec *head_xypnt    /*  */,
                      xypnt *coord_point            /*  */,
                      char *finished                /*  */)
{
  if (head_xypnt)
    {
      head_xypnt->current_point = head_xypnt->first_point;
      if (head_xypnt->current_point == NULL)
        *finished = 1;
      else
        { 
          *coord_point = head_xypnt->current_point->point;
          *finished = 0;
        }
    }
  else 
    *finished = 1;
}


/******************************************************************************
* This routine will add the "coord_point" to the end of the xypnt list
* which is specified by the "head_xypnt". Does not change current_point.
*/
void xypnt_add_pnt (xypnt_head_rec *head_xypnt      /*  */,
                    xypnt coord_point               /*  */)
{
  xypnt_point_rec *temp_point;

  if (!head_xypnt)
    return;
  temp_point=(struct xypnt_point_t *)calloc(1,sizeof(struct xypnt_point_t));
  temp_point->point = coord_point;
  temp_point->next_point = NULL;
  if (head_xypnt->first_point == NULL)
    head_xypnt->first_point = temp_point;
  else
    head_xypnt->last_point->next_point = temp_point;
  head_xypnt->last_point = temp_point;
}


/******************************************************************************
* This routine will dispose a list of points and the head pointer to
* which they are connected to. The pointer is returned as a NIL. 
*/
void xypnt_dispose_list (xypnt_head_rec **head_xypnt       /*  */)
{
  xypnt_point_rec *p, *old;
  if (head_xypnt && *head_xypnt)
    { 
      if ((*head_xypnt)->last_point && (*head_xypnt)->first_point)
        {
              p = (*head_xypnt)->first_point;
              while (p)
                {
                  old = p;
                  p = p->next_point;
                  free(old);
                }
        }
    }
}


/******************************************************************************
* Searches color by closest rgb values (distance of 2 3D points)
* not os specific. 
* returns: index of color
*/
int GetIndexByRGBValue(int red                   /*  */,
                       int green                 /*  */,
                       int blue                  /*  */)
{
  int savdis=1, i;
  double psav = 10000000, pnew, px, py, pz;
  int nred, ngreen, nblue;

  for (i = 0; i < MAX_COLORS; i++)
    {
      nred = dxftable[i].red;
      ngreen = dxftable[i].green;
      nblue = dxftable[i].blue;
      /* compute shortest distance between rgb colors */
      px = red*red - 2*nred*red + nred*nred;
      py = green*green - 2*ngreen*green + ngreen*ngreen;
      pz = blue*blue - 2*nblue*blue + nblue*nblue;
      pnew = sqrt(px+py+pz);
      if (pnew < psav)
        {
          psav = pnew;
          savdis = i;
        }
    }
  return(savdis+1);
}



/******************************************************************************
* Moves the current_point pointer to the end of the list 
*/
void xypnt_last_pnt (xypnt_head_rec *head_xypnt     /*  */,
                     xypnt *coord_point             /*  */,
                     char *finished                 /*  */)
{
  if (head_xypnt)
    {
      head_xypnt->current_point = head_xypnt->last_point;
      if (head_xypnt->current_point == NULL)
        *finished = 1;
      else
        { 
          *coord_point = head_xypnt->current_point->point;
          *finished = 0;
        }
    }
  else 
    *finished = 1;
}




/******************************************************************************
* computes the distance between p1 and p2
* 
* returns: 
*/
double distpt2pt(xypnt p1                           /*  */,
                 xypnt p2                           /*  */)
{
  double dx, dy;

  dx = p2.xp - p1.xp;
  dy = p2.yp - p1.yp;
  if (p1.xp==p2.xp)
    return (fabs (dy));
  else if (p1.yp==p2.yp)
    return (fabs (dx));
  else
    return (sqrt(dx*dx + dy*dy));
}


/******************************************************************************
* returns: length of all vectors in vertex list
*/
static double get_total_length(xypnt_head_rec *vtx_list          /*  */)
{
  double total_length;
  xypnt curr_pnt, next_pnt;
  char end_of_list;

  total_length = 0.0;
  xypnt_first_pnt(vtx_list, &curr_pnt, &end_of_list);
  while (!end_of_list)
    {
      xypnt_next_pnt(vtx_list, &next_pnt, &end_of_list);
      total_length += distpt2pt(curr_pnt, next_pnt);
      curr_pnt = next_pnt;
    }
  return (total_length);
}




/******************************************************************************
* Convert B-Spline to list of lines.
*/
int bspline_to_lines(xypnt_head_rec *vtx_list          /*  */,
                     xypnt_head_rec **new_vtx_list     /*  */,
                     int vtx_count                     /*  */,
                     int spline_order                  /*  */,
                     int spline_resolution             /*  */)
{                    
  int i, j, knot_index, number_of_segments, knot[MAX_VERTICES + 1],n,m;
  double spline_step, total_length, t, spline_pnt_x, spline_pnt_y, r,
    *weight;
  xypnt curr_pnt, spline_pnt;
  char end_of_list;

  *new_vtx_list = (struct xypnt_head_t *) calloc(1, sizeof (struct xypnt_head_t));
  if (vtx_list)
    { 
      n = vtx_count + spline_order+1; 
      m = spline_order + 1;  
      weight = (double *) malloc( n*m* sizeof(double)); 

      for (i = 0; i < vtx_count + spline_order; i++)
        knot[i] = (i < spline_order) ? 0 : 
        (i > vtx_count) ? knot[i-1] : knot[i-1] + 1;
      total_length = get_total_length(vtx_list);
      r = (spline_resolution==0) ? sqrt(total_length) :
        total_length/spline_resolution;
      number_of_segments = ROUND(r);
      spline_step = ((double)knot[vtx_count+spline_order-1]) / 
        number_of_segments;
      for (knot_index=spline_order-1; knot_index<vtx_count; knot_index++)
        { 
          for (i = 0; i <= vtx_count + spline_order - 2; i++)
            weight[i] = (i==knot_index && knot[i]!=knot[i+1]);
          t = knot[knot_index];
          while (t < knot[knot_index+1] - spline_step / 2.0)
            { 
              spline_pnt_x = 0.0;
              spline_pnt_y = 0.0;
              for (j = 2; j <= spline_order; j++)
                {
                  i = 0;
                  xypnt_first_pnt(vtx_list, &curr_pnt, &end_of_list);
                  while (!end_of_list)
                    {
                      weight[(j-1)*n + i] = 0;
                      if (weight[(j-2)*n+i])
                        weight[(j-1)*n+i] += (t-knot[i]) * weight[(j-2)*n+i] / 
                          (knot[i+j-1] - knot[i]);
                      if (weight[(j-2)*n + i+1])
                        weight[(j-1)*n+i] += (knot[i+j]-t)*weight[(j-2)*n+i+1]/
                          (knot[i+j] - knot[i+1]);
                      if (j == spline_order)
                        {
                          spline_pnt_x += curr_pnt.xp * weight[(j-1)*n+i];
                          spline_pnt_y += curr_pnt.yp * weight[(j-1)*n+i];
                        }
                      i++;
                      xypnt_next_pnt(vtx_list, &curr_pnt, &end_of_list);
                    } 
                  weight[(j-1)*n+i] = 0;
                } 
              spline_pnt.xp = ROUND(spline_pnt_x);
              spline_pnt.yp = ROUND(spline_pnt_y);
              xypnt_add_pnt(*new_vtx_list, spline_pnt);
              t += spline_step;
            } 
        }
      xypnt_last_pnt(vtx_list, &spline_pnt, &end_of_list);
      xypnt_add_pnt(*new_vtx_list, spline_pnt);

      free(weight);
    } 

  return(0);
}


/******************************************************************************
* This function outputs the DXF code which produces the polylines 
*/
static void out_splines (FILE * ps_file, spline_list_array_type shape)
{
  extern bool at_centerline; 
  unsigned this_list;
  double startx, starty;
  xypnt_head_rec *vec, *res;
  xypnt pnt, pnt1, pnt_old;
  char fin, new_layer=0, layerstr[10];
  int i, first_seg = 1, idx;

  strcpy(layerstr, "C1");
  for (this_list = 0; this_list < SPLINE_LIST_ARRAY_LENGTH (shape);
       this_list++)
    {
      unsigned this_spline;
      color_type last_color;

      spline_list_type list = SPLINE_LIST_ARRAY_ELT (shape, this_list);
      spline_type first = SPLINE_LIST_ELT (list, 0);

      if (this_list == 0 || !COLOR_EQUAL(list.color, last_color))
        {
          /* sometimes RGB 0,0,0 is in list.color, assume that this means no color change */
          if (!(list.color.r==0 && list.color.g==0 && list.color.b==0))
            {
             idx = GetIndexByRGBValue(list.color.r, list.color.g, list.color.b);
             sprintf(layerstr, "C%d", idx);
             new_layer = 1;
             last_color = list.color;
            }
    	}
      startx = START_POINT (first).x;
      starty = START_POINT (first).y;
      if (!first_seg)
        {
         if (ROUND(startx*RESOLUTION) != pnt_old.xp || ROUND(starty*RESOLUTION) != pnt_old.yp || new_layer)
           {
            /* must begin new polyline */
             new_layer = 0;
             fprintf(ps_file, "  0\nSEQEND\n  8\n%s\n", layerstr);
             fprintf(ps_file, "  0\nPOLYLINE\n  8\n%s\n  66\n1\n  10\n%f\n  20\n%f\n",
                     layerstr, startx, starty);
             fprintf(ps_file, "  0\nVERTEX\n  8\n%s\n  10\n%f\n  20\n%f\n",
                     layerstr, startx, starty);
             pnt_old.xp = ROUND(startx*RESOLUTION);
             pnt_old.yp = ROUND(starty*RESOLUTION);
           }
        }   
      else
        {
         fprintf(ps_file, "  0\nPOLYLINE\n  8\n%s\n  66\n1\n  10\n%f\n  20\n%f\n",
                 layerstr, startx, starty);
         fprintf(ps_file, "  0\nVERTEX\n  8\n%s\n  10\n%f\n  20\n%f\n",
                 layerstr, startx, starty);
         pnt_old.xp = ROUND(startx*RESOLUTION);
         pnt_old.yp = ROUND(starty*RESOLUTION);
        } 
      for (this_spline = 0; this_spline < SPLINE_LIST_LENGTH (list);
           this_spline++)
        {
          spline_type s = SPLINE_LIST_ELT (list, this_spline);

          if (SPLINE_DEGREE (s) == LINEARTYPE)
            {

             if (ROUND(startx*RESOLUTION) != pnt_old.xp || ROUND(starty*RESOLUTION) != pnt_old.yp || new_layer)
               {
                /* must begin new polyline */
                new_layer = 0;
                fprintf(ps_file, "  0\nSEQEND\n  8\n%s\n", layerstr);
                fprintf(ps_file, "  0\nPOLYLINE\n  8\n%s\n  66\n1\n  10\n%f\n  20\n%f\n",
                        layerstr, startx, starty);
                fprintf(ps_file, "  0\nVERTEX\n  8\n%s\n  10\n%f\n  20\n%f\n",
                        layerstr, startx, starty);
               }
             fprintf(ps_file, "  0\nVERTEX\n  8\n%s\n  10\n%f\n  20\n%f\n",
                     layerstr, END_POINT(s).x, END_POINT (s).y);

             startx = END_POINT(s).x;
             starty = END_POINT(s).y;
             pnt_old.xp = ROUND(startx*RESOLUTION);
             pnt_old.yp = ROUND(starty*RESOLUTION);
            }
          else
            {
             vec = (struct xypnt_head_t *) calloc(1, sizeof (struct xypnt_head_t));

             pnt.xp = ROUND(startx*RESOLUTION);  pnt.yp = ROUND(starty*RESOLUTION);
             xypnt_add_pnt(vec, pnt);
             pnt.xp = ROUND(CONTROL1(s).x*RESOLUTION);  pnt.yp = ROUND(CONTROL1 (s).y*RESOLUTION);
             xypnt_add_pnt(vec, pnt);
             pnt.xp = ROUND(CONTROL2(s).x*RESOLUTION);  pnt.yp = ROUND(CONTROL2 (s).y*RESOLUTION);
             xypnt_add_pnt(vec, pnt);
             pnt.xp = ROUND(END_POINT(s).x*RESOLUTION);  pnt.yp = ROUND(END_POINT (s).y*RESOLUTION);
             xypnt_add_pnt(vec, pnt);

             res = NULL;

             /* Note that spline order can be max. 4 since we have only 4 spline control points */
             bspline_to_lines(vec, &res, 4, 4, 10000);


             xypnt_first_pnt(res, &pnt, &fin);

             if (pnt.xp != pnt_old.xp || pnt.yp != pnt_old.yp || new_layer)
               {
                /* must begin new polyline */
                new_layer = 0;
                fprintf(ps_file, "  0\nSEQEND\n  8\n%s\n", layerstr);
                fprintf(ps_file, "  0\nPOLYLINE\n  8\n%s\n  66\n1\n  10\n%f\n  20\n%f\n",
                        layerstr, (double)pnt.xp/RESOLUTION, (double)pnt.yp/RESOLUTION);
                fprintf(ps_file, "  0\nVERTEX\n  8\n%s\n  10\n%f\n  20\n%f\n",
                        layerstr, (double)pnt.xp/RESOLUTION, (double)pnt.yp/RESOLUTION);
               }
             i = 0;
             while (!fin)
                {
                 if (i)
                   {
                    fprintf(ps_file, "  0\nVERTEX\n  8\n%s\n  10\n%f\n  20\n%f\n",
                           layerstr, (double)pnt.xp/RESOLUTION, (double)pnt.yp/RESOLUTION);
                   }
                 pnt1 = pnt;
                 xypnt_next_pnt(res, &pnt, &fin);
                 i++;
                }

             pnt_old = pnt;

             xypnt_dispose_list(&vec);
             xypnt_dispose_list(&res);

             startx = END_POINT(s).x;
             starty = END_POINT(s).y;

             free(res);
             free(vec);
            }
        }
      first_seg = 0;
      last_color = list.color;
    }

  fprintf(ps_file, "  0\nSEQEND\n  8\n0\n");

}


/******************************************************************************
* This function outputs a complete layer table for all 255 colors. 
*/
void output_layer(FILE *ps_file)
{
  int i;
  char str[20], str1[20];

  OUT_LINE("  0");
  OUT_LINE("SECTION");
  OUT_LINE("  2");
  OUT_LINE("TABLES");
  OUT_LINE("  0");
  OUT_LINE("TABLE");
  OUT_LINE("  2");
  OUT_LINE("LAYER");
  OUT_LINE("  70");
  OUT_LINE("     2048");

  OUT_LINE("  0");
  OUT_LINE("LAYER");
  OUT_LINE("  2");
  OUT_LINE("0");
  OUT_LINE("  70");
  OUT_LINE("    0");
  OUT_LINE("  62");
  OUT_LINE("     7");
  OUT_LINE("  6");
  OUT_LINE("CONTINUOUS");

  for (i=1; i<256; i++)
    {
     sprintf(str, "C%d", i); 
     sprintf(str1, "%d", i); 
     OUT_LINE("  0");
     OUT_LINE("LAYER");
     OUT_LINE("   2");
     OUT_LINE(str);
     OUT_LINE("  70");
     OUT_LINE("     64");
     OUT_LINE("  62");
     OUT_LINE(str1);
     OUT_LINE("  6");
     OUT_LINE("CONTINUOUS");
    } 

  OUT_LINE("  0");
  OUT_LINE("ENDTAB");
  OUT_LINE ("  0");
  OUT_LINE ("ENDSEC");

}


/******************************************************************************
* DXF output function.
*/
int output_dxf_writer(FILE* ps_file, string name,
         		      int llx, int lly, int urx, int ury,
		              spline_list_array_type shape)
{

  output_layer(ps_file);

  OUT_LINE ("  0");
  OUT_LINE ("SECTION");
  OUT_LINE ("  2");
  OUT_LINE ("ENTITIES");

  out_splines(ps_file, shape);

  OUT_LINE ("  0");
  OUT_LINE ("ENDSEC");
  OUT_LINE ("  0");
  OUT_LINE ("EOF");
  return 0;
}

