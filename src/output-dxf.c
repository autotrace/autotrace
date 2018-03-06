/* output-dxf.c: utility routines for DXF output.

   Copyright (C) 1999, 2000, 2001 Dietmar Kovar

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

/* mail comments and suggestions to kovar@t-online.de */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* Def: HAVE_CONFIG_H */

#include "types.h"
#include "spline.h"
#include "color.h"
#include "output-dxf.h"
#include "xstd.h"
#include "autotrace.h"
#include <math.h>
#include <time.h>
#include <string.h>

/* Output macros.  */

/* This should be used for outputting a string S on a line by itself.  */
#define OUT_LINE(s)							\
  fprintf (dxf_file, "%s\n", s)

/* These output their arguments, preceded by the indentation.  */
#define OUT(s, ...)							\
  fprintf (dxf_file, s, __VA_ARGS__)

#define color_check FALSE

/**************************************************************************************
Definitions for spline to line transformation
**************************************************************************************/

typedef enum { NATURAL, TANGENT, PERIODIC, CYCLIC, ANTICYCLIC
} SPLINE_END_TYPE;

#define MAX_VERTICES 10000
#define RESOLUTION   10000      /* asume no pixels bigger than 1000000.0 */
#define RADIAN                  57.295779513082

typedef struct xypnt_t {
  int xp, yp;
} xypnt;

typedef struct xypnt_point_t {
  xypnt point;
  struct xypnt_point_t *next_point;
} xypnt_point_rec;

typedef struct xypnt_head_t {
  xypnt_point_rec *first_point, *last_point, *current_point;
  struct xypnt_head_t *next_head;
} xypnt_head_rec;

typedef struct Colors_t {
  int red, green, blue;
} Colors;

/* RGB to layer transformation table follows */

#define MAX_COLORS 255

struct Colors_t dxftable[MAX_COLORS] = {
/*   1 */ {255, 0, 0},
/*   2 */ {255, 255, 0},
/*   3 */ {0, 255, 0},
/*   4 */ {0, 255, 255},
/*   5 */ {0, 0, 255},
/*   6 */ {255, 0, 255},
/*   7 */ {255, 255, 255},
/*   8 */ {255, 255, 255},
/*   9 */ {255, 255, 255},
/*  10 */ {255, 9, 0},
/*  11 */ {255, 128, 128},
/*  12 */ {166, 0, 0},
/*  13 */ {166, 83, 83},
/*  14 */ {128, 0, 0},
/*  15 */ {128, 64, 64},
/*  16 */ {77, 0, 0},
/*  17 */ {77, 38, 38},
/*  18 */ {38, 0, 0},
/*  19 */ {38, 19, 19},
/*  20 */ {255, 64, 0},
/*  21 */ {255, 160, 128},
/*  22 */ {166, 42, 0},
/*  23 */ {166, 104, 83},
/*  24 */ {128, 32, 0},
/*  25 */ {128, 80, 64},
/*  26 */ {77, 19, 0},
/*  27 */ {77, 48, 38},
/*  28 */ {38, 10, 0},
/*  29 */ {38, 24, 19},
/*  30 */ {256, 128, 0},
/*  31 */ {256, 192, 0},
/*  32 */ {166, 83, 0},
/*  33 */ {166, 125, 83},
/*  34 */ {128, 64, 0},
/*  35 */ {128, 96, 64},
/*  36 */ {77, 38, 0},
/*  37 */ {77, 58, 38},
/*  38 */ {38, 19, 0},
/*  39 */ {38, 29, 19},
/*  40 */ {255, 192, 0},
/*  41 */ {255, 224, 128},
/*  42 */ {166, 125, 0},
/*  43 */ {166, 146, 83},
/*  44 */ {128, 96, 0},
/*  45 */ {128, 112, 64},
/*  46 */ {77, 58, 0},
/*  47 */ {77, 67, 38},
/*  48 */ {38, 29, 0},
/*  49 */ {38, 34, 19},
/*  50 */ {255, 255, 0},
/*  51 */ {255, 255, 128},
/*  52 */ {166, 166, 0},
/*  53 */ {166, 166, 83},
/*  54 */ {128, 128, 0},
/*  55 */ {128, 128, 64},
/*  56 */ {77, 77, 0},
/*  57 */ {77, 77, 38},
/*  58 */ {38, 38, 0},
/*  59 */ {38, 38, 19},
/*  60 */ {192, 255, 0},
/*  61 */ {224, 255, 128},
/*  62 */ {125, 166, 0},
/*  63 */ {146, 166, 83},
/*  64 */ {96, 128, 0},
/*  65 */ {112, 128, 64},
/*  66 */ {58, 77, 0},
/*  67 */ {67, 77, 38},
/*  68 */ {29, 38, 0},
/*  69 */ {34, 38, 19},
/*  70 */ {128, 255, 0},
/*  71 */ {192, 255, 128},
/*  72 */ {83, 166, 0},
/*  73 */ {125, 166, 83},
/*  74 */ {64, 128, 0},
/*  75 */ {96, 128, 64},
/*  76 */ {38, 77, 0},
/*  77 */ {58, 77, 38},
/*  78 */ {19, 38, 0},
/*  79 */ {29, 38, 19},
/*  80 */ {64, 255, 0},
/*  81 */ {160, 255, 128},
/*  82 */ {42, 160, 0},
/*  83 */ {104, 160, 80},
/*  84 */ {32, 128, 0},
/*  85 */ {80, 128, 64},
/*  86 */ {19, 77, 0},
/*  87 */ {48, 77, 38},
/*  88 */ {10, 38, 0},
/*  89 */ {24, 38, 19},
/*  90 */ {0, 255, 0},
/*  91 */ {128, 255, 128},
/*  92 */ {0, 166, 0},
/*  93 */ {83, 166, 83},
/*  94 */ {0, 128, 0},
/*  95 */ {64, 128, 64},
/*  96 */ {0, 77, 0},
/*  97 */ {38, 77, 38},
/*  98 */ {0, 38, 0},
/*  99 */ {19, 38, 19},
/* 100 */ {0, 255, 64},
/* 101 */ {128, 255, 160},
/* 102 */ {0, 166, 42},
/* 103 */ {83, 166, 118},
/* 104 */ {0, 128, 32},
/* 105 */ {64, 128, 80},
/* 106 */ {0, 77, 19},
/* 107 */ {38, 77, 48},
/* 108 */ {0, 38, 10},
/* 109 */ {19, 38, 24},
/* 110 */ {0, 255, 128},
/* 111 */ {128, 255, 192},
/* 112 */ {0, 166, 83},
/* 113 */ {83, 166, 125},
/* 114 */ {0, 128, 64},
/* 115 */ {64, 128, 96},
/* 116 */ {0, 77, 38},
/* 117 */ {38, 77, 58},
/* 118 */ {0, 38, 19},
/* 119 */ {19, 38, 29},
/* 120 */ {0, 255, 192},
/* 121 */ {128, 255, 224},
/* 122 */ {0, 166, 125},
/* 123 */ {83, 166, 146},
/* 124 */ {0, 128, 96},
/* 125 */ {64, 128, 112},
/* 125 */ {0, 77, 58},
/* 127 */ {38, 77, 67},
/* 128 */ {0, 38, 29},
/* 129 */ {19, 38, 34},
/* 130 */ {0, 255, 255},
/* 131 */ {128, 255, 255},
/* 132 */ {0, 166, 166},
/* 133 */ {83, 166, 166},
/* 134 */ {0, 128, 128},
/* 135 */ {64, 128, 128},
/* 136 */ {0, 77, 77},
/* 137 */ {38, 77, 77},
/* 138 */ {0, 38, 38},
/* 139 */ {19, 38, 38},
/* 140 */ {0, 192, 255},
/* 141 */ {128, 224, 255},
/* 142 */ {0, 125, 166},
/* 143 */ {83, 146, 166},
/* 144 */ {0, 96, 128},
/* 145 */ {64, 112, 128},
/* 146 */ {0, 58, 77},
/* 147 */ {38, 67, 77},
/* 148 */ {0, 29, 38},
/* 149 */ {19, 34, 38},
/* 150 */ {0, 128, 255},
/* 151 */ {128, 192, 255},
/* 152 */ {0, 83, 166},
/* 153 */ {83, 125, 166},
/* 154 */ {0, 64, 128},
/* 155 */ {64, 96, 128},
/* 156 */ {0, 38, 77},
/* 157 */ {38, 58, 77},
/* 158 */ {0, 19, 38},
/* 159 */ {19, 29, 38},
/* 160 */ {0, 64, 255},
/* 161 */ {128, 160, 255},
/* 162 */ {0, 42, 166},
/* 163 */ {83, 104, 166},
/* 164 */ {0, 32, 128},
/* 165 */ {64, 80, 128},
/* 166 */ {0, 19, 77},
/* 167 */ {38, 48, 77},
/* 168 */ {0, 10, 38},
/* 169 */ {19, 24, 38},
/* 170 */ {0, 0, 255},
/* 171 */ {128, 128, 255},
/* 172 */ {0, 0, 166},
/* 173 */ {83, 83, 166},
/* 174 */ {0, 0, 128},
/* 175 */ {64, 64, 128},
/* 176 */ {0, 0, 77},
/* 177 */ {38, 38, 77},
/* 178 */ {0, 0, 38},
/* 179 */ {19, 19, 38},
/* 180 */ {64, 0, 255},
/* 181 */ {160, 128, 255},
/* 182 */ {42, 0, 166},
/* 183 */ {104, 83, 166},
/* 184 */ {32, 0, 128},
/* 185 */ {80, 64, 128},
/* 186 */ {19, 0, 77},
/* 187 */ {48, 38, 77},
/* 188 */ {10, 0, 38},
/* 189 */ {24, 19, 38},
/* 190 */ {128, 0, 255},
/* 191 */ {192, 128, 255},
/* 192 */ {83, 0, 166},
/* 193 */ {125, 83, 166},
/* 194 */ {64, 0, 128},
/* 195 */ {96, 64, 128},
/* 196 */ {38, 0, 77},
/* 197 */ {58, 38, 77},
/* 198 */ {19, 0, 38},
/* 199 */ {29, 19, 38},
/* 200 */ {192, 0, 255},
/* 201 */ {224, 128, 255},
/* 202 */ {125, 0, 166},
/* 203 */ {146, 83, 166},
/* 204 */ {96, 0, 128},
/* 205 */ {112, 64, 128},
/* 206 */ {58, 0, 77},
/* 207 */ {67, 38, 77},
/* 208 */ {29, 0, 38},
/* 209 */ {34, 19, 38},
/* 210 */ {255, 0, 255},
/* 211 */ {255, 128, 255},
/* 212 */ {166, 0, 166},
/* 213 */ {166, 83, 166},
/* 214 */ {128, 0, 128},
/* 215 */ {128, 64, 128},
/* 216 */ {77, 0, 77},
/* 217 */ {77, 38, 77},
/* 218 */ {38, 0, 38},
/* 219 */ {38, 19, 38},
/* 220 */ {255, 0, 192},
/* 221 */ {255, 128, 224},
/* 222 */ {166, 0, 125},
/* 223 */ {166, 83, 146},
/* 224 */ {128, 0, 96},
/* 225 */ {128, 64, 112},
/* 226 */ {77, 0, 58},
/* 227 */ {77, 38, 67},
/* 228 */ {38, 0, 29},
/* 229 */ {38, 19, 34},
/* 230 */ {255, 0, 128},
/* 231 */ {255, 128, 192},
/* 232 */ {166, 0, 83},
/* 233 */ {166, 83, 125},
/* 234 */ {128, 0, 64},
/* 235 */ {128, 64, 96},
/* 236 */ {77, 0, 38},
/* 237 */ {77, 38, 58},
/* 238 */ {38, 0, 19},
/* 239 */ {38, 19, 29},
/* 240 */ {255, 0, 64},
/* 241 */ {255, 128, 160},
/* 242 */ {166, 0, 42},
/* 243 */ {166, 83, 104},
/* 244 */ {128, 0, 32},
/* 245 */ {128, 64, 80},
/* 246 */ {77, 0, 19},
/* 247 */ {77, 38, 48},
/* 248 */ {38, 0, 10},
/* 249 */ {38, 19, 24},
/* 250 */ {84, 84, 84},
/* 251 */ {119, 119, 119},
/* 252 */ {153, 153, 153},
/* 253 */ {187, 187, 187},
/* 254 */ {222, 222, 222},
/* 255 */ {255, 255, 255}
};

/******************************************************************************
* Moves the current_point pointer to the next point in the list.
* If current_point is at last point then it becomes NULL.
* finished is 1 if coord_point has not been set, that is current_point is NULL.
*/
void xypnt_next_pnt(xypnt_head_rec * head_xypnt /*  */ ,
                    xypnt * coord_point /*  */ ,
                    char *finished /*  */ )
{
  if (head_xypnt && head_xypnt->current_point) {
    head_xypnt->current_point = head_xypnt->current_point->next_point;
    if (head_xypnt->current_point == NULL)
      *finished = 1;
    else {
      *coord_point = head_xypnt->current_point->point;
      *finished = 0;
    }
  } else
    *finished = 1;
}

/******************************************************************************
* Moves the current_point pointer to the begining of the list
*/
void xypnt_first_pnt(xypnt_head_rec * head_xypnt /*  */ ,
                     xypnt * coord_point /*  */ ,
                     char *finished /*  */ )
{
  if (head_xypnt) {
    head_xypnt->current_point = head_xypnt->first_point;
    if (head_xypnt->current_point == NULL)
      *finished = 1;
    else {
      *coord_point = head_xypnt->current_point->point;
      *finished = 0;
    }
  } else
    *finished = 1;
}

/******************************************************************************
* This routine will add the "coord_point" to the end of the xypnt list
* which is specified by the "head_xypnt". Does not change current_point.
*/
void xypnt_add_pnt(xypnt_head_rec * head_xypnt /*  */ ,
                   xypnt coord_point /*  */ )
{
  xypnt_point_rec *temp_point;

  if (!head_xypnt)
    return;
  temp_point = (struct xypnt_point_t *)calloc(1, sizeof(struct xypnt_point_t));
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
void xypnt_dispose_list(xypnt_head_rec ** head_xypnt /*  */ )
{
  xypnt_point_rec *p, *old;
  if (head_xypnt && *head_xypnt) {
    if ((*head_xypnt)->last_point && (*head_xypnt)->first_point) {
      p = (*head_xypnt)->first_point;
      while (p) {
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
int GetIndexByRGBValue(int red /*  */ ,
                       int green /*  */ ,
                       int blue /*  */ )
{
  int savdis = 1, i;
  double psav = 10000000, pnew, px, py, pz;
  int nred, ngreen, nblue;

  for (i = 0; i < MAX_COLORS; i++) {
    nred = dxftable[i].red;
    ngreen = dxftable[i].green;
    nblue = dxftable[i].blue;
    /* compute shortest distance between rgb colors */
    px = red * red - 2 * nred * red + nred * nred;
    py = green * green - 2 * ngreen * green + ngreen * ngreen;
    pz = blue * blue - 2 * nblue * blue + nblue * nblue;
    pnew = sqrt(px + py + pz);
    if (pnew < psav) {
      psav = pnew;
      savdis = i;
    }
  }
  return (savdis + 1);
}

/******************************************************************************
* Moves the current_point pointer to the end of the list
*/
void xypnt_last_pnt(xypnt_head_rec * head_xypnt /*  */ ,
                    xypnt * coord_point /*  */ ,
                    char *finished /*  */ )
{
  if (head_xypnt) {
    head_xypnt->current_point = head_xypnt->last_point;
    if (head_xypnt->current_point == NULL)
      *finished = 1;
    else {
      *coord_point = head_xypnt->current_point->point;
      *finished = 0;
    }
  } else
    *finished = 1;
}

/******************************************************************************
* computes the distance between p1 and p2
*
* returns:
*/
double distpt2pt(xypnt p1 /*  */ ,
                 xypnt p2 /*  */ )
{
  double dx, dy;

  dx = p2.xp - p1.xp;
  dy = p2.yp - p1.yp;
  if (p1.xp == p2.xp)
    return (fabs(dy));
  else if (p1.yp == p2.yp)
    return (fabs(dx));
  else
    return (sqrt(dx * dx + dy * dy));
}

/******************************************************************************
* returns: length of all vectors in vertex list
*/
static double get_total_length(xypnt_head_rec * vtx_list /*  */ )
{
  double total_length;
  xypnt curr_pnt, next_pnt;
  char end_of_list;

  total_length = 0.0;
  xypnt_first_pnt(vtx_list, &curr_pnt, &end_of_list);
  while (!end_of_list) {
    xypnt_next_pnt(vtx_list, &next_pnt, &end_of_list);
    total_length += distpt2pt(curr_pnt, next_pnt);
    curr_pnt = next_pnt;
  }
  return (total_length);
}

/******************************************************************************
* Convert B-Spline to list of lines.
*/
int bspline_to_lines(xypnt_head_rec * vtx_list /*  */ ,
                     xypnt_head_rec ** new_vtx_list /*  */ ,
                     int vtx_count /*  */ ,
                     int spline_order /*  */ ,
                     int spline_resolution /*  */ )
{
  int i, j, knot_index, number_of_segments, knot[MAX_VERTICES + 1], n, m;
  double spline_step, total_length, t, spline_pnt_x, spline_pnt_y, r, *weight;
  xypnt curr_pnt, spline_pnt;
  char end_of_list;

  *new_vtx_list = (struct xypnt_head_t *)calloc(1, sizeof(struct xypnt_head_t));
  if (vtx_list) {
    n = vtx_count + spline_order + 1;
    m = spline_order + 1;
    XMALLOC(weight, n * m * sizeof(double));

    for (i = 0; i < vtx_count + spline_order; i++)
      knot[i] = (i < spline_order) ? 0 : (i > vtx_count) ? knot[i - 1] : knot[i - 1] + 1;
    total_length = get_total_length(vtx_list);
    r = (spline_resolution == 0) ? sqrt(total_length) : total_length / spline_resolution;
    number_of_segments = lround(r);
    spline_step = ((double)knot[vtx_count + spline_order - 1]) / number_of_segments;
    for (knot_index = spline_order - 1; knot_index < vtx_count; knot_index++) {
      for (i = 0; i <= vtx_count + spline_order - 2; i++)
        weight[i] = (i == knot_index && knot[i] != knot[i + 1]);
      t = knot[knot_index];
      while (t < knot[knot_index + 1] - spline_step / 2.0) {
        spline_pnt_x = 0.0;
        spline_pnt_y = 0.0;
        for (j = 2; j <= spline_order; j++) {
          i = 0;
          xypnt_first_pnt(vtx_list, &curr_pnt, &end_of_list);
          while (!end_of_list) {
            weight[(j - 1) * n + i] = 0;
            if (weight[(j - 2) * n + i])
              weight[(j - 1) * n + i] += (t - knot[i]) * weight[(j - 2) * n + i] / (knot[i + j - 1] - knot[i]);
            if (weight[(j - 2) * n + i + 1])
              weight[(j - 1) * n + i] += (knot[i + j] - t) * weight[(j - 2) * n + i + 1] / (knot[i + j] - knot[i + 1]);
            if (j == spline_order) {
              spline_pnt_x += curr_pnt.xp * weight[(j - 1) * n + i];
              spline_pnt_y += curr_pnt.yp * weight[(j - 1) * n + i];
            }
            i++;
            xypnt_next_pnt(vtx_list, &curr_pnt, &end_of_list);
          }
          weight[(j - 1) * n + i] = 0;
        }
        spline_pnt.xp = lround(spline_pnt_x);
        spline_pnt.yp = lround(spline_pnt_y);
        xypnt_add_pnt(*new_vtx_list, spline_pnt);
        t += spline_step;
      }
    }
    xypnt_last_pnt(vtx_list, &spline_pnt, &end_of_list);
    xypnt_add_pnt(*new_vtx_list, spline_pnt);

    free(weight);
  }

  return (0);
}

/******************************************************************************
* This function outputs the DXF code which produces the polylines
*/
static void out_splines(FILE * dxf_file, spline_list_array_type shape)
{
  unsigned this_list;
  double startx, starty;
  xypnt_head_rec *vec, *res;
  xypnt pnt, pnt_old = { 0, 0 };
  char fin, new_layer = 0, layerstr[10];
  int i, first_seg = 1, idx;

  strcpy(layerstr, "C1");
  for (this_list = 0; this_list < SPLINE_LIST_ARRAY_LENGTH(shape); this_list++) {
    unsigned this_spline;
    at_color last_color = { 0, 0, 0 };

    spline_list_type list = SPLINE_LIST_ARRAY_ELT(shape, this_list);
    spline_type first = SPLINE_LIST_ELT(list, 0);
    at_color curr_color = (list.clockwise && shape.background_color != NULL) ? *(shape.background_color) : list.color;

    if (this_list == 0 || !at_color_equal(&curr_color, &last_color)) {
      if (!(curr_color.r == 0 && curr_color.g == 0 && curr_color.b == 0) || !color_check) {
        idx = GetIndexByRGBValue(curr_color.r, curr_color.g, curr_color.b);
        sprintf(layerstr, "C%d", idx);
        new_layer = 1;
        last_color = curr_color;
      }
    }
    startx = START_POINT(first).x;
    starty = START_POINT(first).y;
    if (!first_seg) {
      if (lround(startx * RESOLUTION) != pnt_old.xp || lround(starty * RESOLUTION) != pnt_old.yp || new_layer) {
        /* must begin new polyline */
        new_layer = 0;
        fprintf(dxf_file, "  0\nSEQEND\n  8\n%s\n", layerstr);
        fprintf(dxf_file, "  0\nPOLYLINE\n  8\n%s\n  66\n1\n  10\n%f\n  20\n%f\n", layerstr, startx, starty);
        fprintf(dxf_file, "  0\nVERTEX\n  8\n%s\n  10\n%f\n  20\n%f\n", layerstr, startx, starty);
        pnt_old.xp = lround(startx * RESOLUTION);
        pnt_old.yp = lround(starty * RESOLUTION);
      }
    } else {
      fprintf(dxf_file, "  0\nPOLYLINE\n  8\n%s\n  66\n1\n  10\n%f\n  20\n%f\n", layerstr, startx, starty);
      fprintf(dxf_file, "  0\nVERTEX\n  8\n%s\n  10\n%f\n  20\n%f\n", layerstr, startx, starty);
      pnt_old.xp = lround(startx * RESOLUTION);
      pnt_old.yp = lround(starty * RESOLUTION);
    }
    for (this_spline = 0; this_spline < SPLINE_LIST_LENGTH(list); this_spline++) {
      spline_type s = SPLINE_LIST_ELT(list, this_spline);

      if (SPLINE_DEGREE(s) == LINEARTYPE) {

        if (lround(startx * RESOLUTION) != pnt_old.xp || lround(starty * RESOLUTION) != pnt_old.yp || new_layer) {
          /* must begin new polyline */
          new_layer = 0;
          fprintf(dxf_file, "  0\nSEQEND\n  8\n%s\n", layerstr);
          fprintf(dxf_file, "  0\nPOLYLINE\n  8\n%s\n  66\n1\n  10\n%f\n  20\n%f\n", layerstr, startx, starty);
          fprintf(dxf_file, "  0\nVERTEX\n  8\n%s\n  10\n%f\n  20\n%f\n", layerstr, startx, starty);
        }
        fprintf(dxf_file, "  0\nVERTEX\n  8\n%s\n  10\n%f\n  20\n%f\n", layerstr, END_POINT(s).x, END_POINT(s).y);

        startx = END_POINT(s).x;
        starty = END_POINT(s).y;
        pnt_old.xp = lround(startx * RESOLUTION);
        pnt_old.yp = lround(starty * RESOLUTION);
      } else {
        vec = (struct xypnt_head_t *)calloc(1, sizeof(struct xypnt_head_t));

        pnt.xp = lround(startx * RESOLUTION);
        pnt.yp = lround(starty * RESOLUTION);
        xypnt_add_pnt(vec, pnt);
        pnt.xp = lround(CONTROL1(s).x * RESOLUTION);
        pnt.yp = lround(CONTROL1(s).y * RESOLUTION);
        xypnt_add_pnt(vec, pnt);
        pnt.xp = lround(CONTROL2(s).x * RESOLUTION);
        pnt.yp = lround(CONTROL2(s).y * RESOLUTION);
        xypnt_add_pnt(vec, pnt);
        pnt.xp = lround(END_POINT(s).x * RESOLUTION);
        pnt.yp = lround(END_POINT(s).y * RESOLUTION);
        xypnt_add_pnt(vec, pnt);

        res = NULL;

        /* Note that spline order can be max. 4 since we have only 4 spline control points */
        bspline_to_lines(vec, &res, 4, 4, 10000);

        xypnt_first_pnt(res, &pnt, &fin);

        if (pnt.xp != pnt_old.xp || pnt.yp != pnt_old.yp || new_layer) {
          /* must begin new polyline */
          new_layer = 0;
          fprintf(dxf_file, "  0\nSEQEND\n  8\n%s\n", layerstr);
          fprintf(dxf_file, "  0\nPOLYLINE\n  8\n%s\n  66\n1\n  10\n%f\n  20\n%f\n", layerstr, (double)pnt.xp / RESOLUTION, (double)pnt.yp / RESOLUTION);
          fprintf(dxf_file, "  0\nVERTEX\n  8\n%s\n  10\n%f\n  20\n%f\n", layerstr, (double)pnt.xp / RESOLUTION, (double)pnt.yp / RESOLUTION);
        }
        i = 0;
        while (!fin) {
          if (i) {
            fprintf(dxf_file, "  0\nVERTEX\n  8\n%s\n  10\n%f\n  20\n%f\n", layerstr, (double)pnt.xp / RESOLUTION, (double)pnt.yp / RESOLUTION);
          }
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
    last_color = curr_color;
  }

  fprintf(dxf_file, "  0\nSEQEND\n  8\n0\n");

}

/******************************************************************************
* This function outputs a complete layer table for all 255 colors.
*/
void output_layer(FILE * dxf_file, spline_list_array_type shape)
{
  int i, idx;
  char layerlist[256];
  unsigned this_list;
  at_color last_color = { 0, 0, 0 };

  memset(layerlist, 0, sizeof(layerlist));
  for (this_list = 0; this_list < SPLINE_LIST_ARRAY_LENGTH(shape); this_list++) {
    spline_list_type list = SPLINE_LIST_ARRAY_ELT(shape, this_list);
    at_color curr_color = (list.clockwise && shape.background_color != NULL) ? *(shape.background_color) : list.color;

    if (this_list == 0 || !at_color_equal(&curr_color, &last_color)) {
      if (!(curr_color.r == 0 && curr_color.g == 0 && curr_color.b == 0) || !color_check) {
        idx = GetIndexByRGBValue(curr_color.r, curr_color.g, curr_color.b);
        layerlist[idx - 1] = 1;
        last_color = curr_color;
      }
    }
  }

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

  for (i = 1; i < 256; i++) {
    if (layerlist[i - 1]) {
      OUT_LINE("  0");
      OUT_LINE("LAYER");
      OUT_LINE("   2");
      OUT("C%d\n", i);
      OUT_LINE("  70");
      OUT_LINE("     64");
      OUT_LINE("  62");
      OUT("%d\n", i);
      OUT_LINE("  6");
      OUT_LINE("CONTINUOUS");
    }
  }

  OUT_LINE("  0");
  OUT_LINE("ENDTAB");
  OUT_LINE("  0");
  OUT_LINE("ENDSEC");

}

/******************************************************************************
* DXF output function.
*/
int output_dxf12_writer(FILE * dxf_file, gchar * name, int llx, int lly, int urx, int ury, at_output_opts_type * opts, spline_list_array_type shape, at_msg_func msg_func, gpointer msg_data, gpointer user_data)
{
  OUT_LINE("  0");
  OUT_LINE("SECTION");
  OUT_LINE("  2");
  OUT_LINE("HEADER");
  OUT_LINE("  9");
  OUT_LINE("$ACADVER");
  OUT_LINE("  1");
  OUT_LINE("AC1009");
  OUT_LINE("  9");
  OUT_LINE("$EXTMIN");
  OUT_LINE("  10");
  OUT(" %f\n", (double)llx);
  OUT_LINE("  20");
  OUT(" %f\n", (double)lly);
  OUT_LINE("  30");
  OUT_LINE(" 0.000000");
  OUT_LINE("  9");
  OUT_LINE("$EXTMAX");
  OUT_LINE("  10");
  OUT(" %f\n", (double)urx);
  OUT_LINE("  20");
  OUT(" %f\n", (double)ury);
  OUT_LINE("  30");
  OUT_LINE(" 0.000000");
  OUT_LINE("  0");
  OUT_LINE("ENDSEC");

  output_layer(dxf_file, shape);

  OUT_LINE("  0");
  OUT_LINE("SECTION");
  OUT_LINE("  2");
  OUT_LINE("ENTITIES");

  out_splines(dxf_file, shape);

  OUT_LINE("  0");
  OUT_LINE("ENDSEC");
  OUT_LINE("  0");
  OUT_LINE("EOF");
  return 0;
}
