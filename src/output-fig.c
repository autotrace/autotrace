/*
 * Copyright (C) 1999, 2000, 2001 Ian MacPhedran
 * SPDX-FileCopyrightText: © 2000-2002 Martin Weber
 * SPDX-FileCopyrightText: © 2000-2003 Masatake YAMATO
 * SPDX-FileCopyrightText: © 2001 Per Grahn
 * SPDX-FileCopyrightText: © 2001-2002 Ian MacPhedran
 * SPDX-FileCopyrightText: © 2017-2025 Peter Lemenkov
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "output-fig.h"
#include <glib.h>
#include "logreport.h"
#include "color.h"
#include "spline.h"

/* use FIG_X and FIG_Y to convert from local units (pixels) to FIG ones */
/* assume 1 pixel is equal to 1/80 inches (old FIG unit) */
/* Offset by 300 units (1/4 inch) */

#define FIG_X(x) (int)((x * 15.0) + 300.0)
#define FIG_Y(y) (int)(((ury - y) * 15.0) + 300.0)

/* the basic colours */
#define FIG_BLACK 0
#define FIG_BLUE 1
#define FIG_GREEN 2
#define FIG_CYAN 3
#define FIG_RED 4
#define FIG_MAGENTA 5
#define FIG_YELLOW 6
#define FIG_WHITE 7

static gfloat bezpnt(gfloat, gfloat, gfloat, gfloat, gfloat);
static void out_fig_splines(FILE *, spline_list_array_type, int, int, int, int,
                            at_exception_type *);
/* Colour information: the eight predefined FIG colours above plus the
   user colours, which FIG numbers from 32 up to 543.  */
#define FIG_USER_COLOUR 32
#define MAX_FIG_COLOUR 543

typedef struct {
  GHashTable *index; /* packed RGB (guint) -> FIG colour number */
  GArray *user;      /* at_color; user colour n is element n - FIG_USER_COLOUR */
} fig_colour_table;

static fig_colour_table *fig_col_init(void);
static void fig_col_free(fig_colour_table *);
static int get_fig_colour(fig_colour_table *, at_color, at_exception_type *);

/* Bounding Box data and routines */
static float glob_min_x, glob_max_x, glob_min_y, glob_max_y;
static float loc_min_x, loc_max_x, loc_min_y, loc_max_y;
static int glo_bbox_flag = 0, loc_bbox_flag = 0, fig_depth;

static void fig_new_depth()
{
  if (glo_bbox_flag == 0) {
    glob_max_y = loc_max_y;
    glob_min_y = loc_min_y;
    glob_max_x = loc_max_x;
    glob_min_x = loc_min_x;
    glo_bbox_flag = 1;
  } else {
    if ((loc_max_y <= glob_min_y) || (loc_min_y >= glob_max_y) || (loc_max_x <= glob_min_x) ||
        (loc_min_x >= glob_max_x)) {
      /* outside global bounds, increase global box */
      if (loc_max_y > glob_max_y)
        glob_max_y = loc_max_y;
      if (loc_min_y < glob_min_y)
        glob_min_y = loc_min_y;
      if (loc_max_x > glob_max_x)
        glob_max_x = loc_max_x;
      if (loc_min_x < glob_min_x)
        glob_min_x = loc_min_x;
    } else {
      /* inside global bounds, decrease depth and create new bounds */
      glob_max_y = loc_max_y;
      glob_min_y = loc_min_y;
      glob_max_x = loc_max_x;
      glob_min_x = loc_min_x;
      if (fig_depth)
        fig_depth--; /* don't let it get < 0 */
    }
  }
  loc_bbox_flag = 0;
}

static void fig_addtobbox(float x, float y)
{
  if (loc_bbox_flag == 0) {
    loc_max_y = y;
    loc_min_y = y;
    loc_max_x = x;
    loc_min_x = x;
    loc_bbox_flag = 1;
  } else {
    if (loc_max_y < y)
      loc_max_y = y;
    if (loc_min_y > y)
      loc_min_y = y;
    if (loc_max_x < x)
      loc_max_x = x;
    if (loc_min_x > x)
      loc_min_x = x;
  }
}

/* Convert Bezier Spline */

static gfloat bezpnt(gfloat t, gfloat z1, gfloat z2, gfloat z3, gfloat z4)
{
  gfloat temp, t1;
  /* Determine ordinate on Bezier curve at length "t" on curve */
  if (t < (gfloat)0.0) {
    t = (gfloat)0.0;
  }
  if (t > (gfloat)1.0) {
    t = (gfloat)1.0;
  }
  t1 = ((gfloat)1.0 - t);
  temp = t1 * t1 * t1 * z1 + (gfloat)3.0 * t * t1 * t1 * z2 + (gfloat)3.0 * t * t * t1 * z3 +
         t * t * t * z4;
  return (temp);
}

static void out_fig_splines(FILE *file, spline_list_array_type shape, int llx, int lly, int urx,
                            int ury, at_exception_type *exp)
{
  unsigned this_list;
  /*    int fig_colour, fig_depth, i; */
  int fig_colour, fig_fill, fig_width, fig_subt, fig_spline_close, i;

  /*
          add an array of colours for splines (one for each group)
          create palette hash
  */

  /*  Need to create hash table for colours */
  g_autofree int *spline_colours = g_new(int, SPLINE_LIST_ARRAY_LENGTH(shape));

  /* Preload the big 8 */
  fig_colour_table *colours = fig_col_init();

  /*  Load the colours from the splines */
  for (this_list = 0; this_list < SPLINE_LIST_ARRAY_LENGTH(shape); this_list++) {
    spline_list_type list = SPLINE_LIST_ARRAY_ELT(shape, this_list);
    at_color curr_color =
        (list.clockwise && shape.background_color != NULL) ? *(shape.background_color) : list.color;
    spline_colours[this_list] = get_fig_colour(colours, curr_color, exp);
  }
  /* Output colours */
  for (i = 0; i < (int)colours->user->len; i++) {
    at_color c = g_array_index(colours->user, at_color, i);
    fprintf(file, "0 %d #%.2x%.2x%.2x\n", FIG_USER_COLOUR + i, c.r, c.g, c.b);
  }
  fig_col_free(colours);
  /*	Each "spline list" in the array appears to be a group of splines */
  fig_depth = SPLINE_LIST_ARRAY_LENGTH(shape) + 20;
  if (fig_depth > 999) {
    fig_depth = 999;
  }

  for (this_list = 0; this_list < SPLINE_LIST_ARRAY_LENGTH(shape); this_list++) {
    unsigned this_spline;
    spline_list_type list = SPLINE_LIST_ARRAY_ELT(shape, this_list);

    int pointcount = 0, is_spline = 0, j;
    int maxlength = SPLINE_LIST_LENGTH(list) * 5 + 1;

    /*	store the spline points in two arrays, control weights in another */
    g_autofree int *pointx = g_new(int, maxlength);
    g_autofree int *pointy = g_new(int, maxlength);
    g_autofree gfloat *contrl = g_new(gfloat, maxlength);

    if (list.clockwise) {
      fig_colour = FIG_WHITE;
    } else {
      fig_colour = spline_colours[this_list];
    }

    fig_spline_close = 5;

    for (this_spline = 0; this_spline < SPLINE_LIST_LENGTH(list); this_spline++) {
      spline_type s = SPLINE_LIST_ELT(list, this_spline);

      if (pointcount == 0) {
        pointx[pointcount] = FIG_X(START_POINT(s).x);
        pointy[pointcount] = FIG_Y(START_POINT(s).y);
        contrl[pointcount] = (gfloat)0.0;
        fig_addtobbox(START_POINT(s).x, START_POINT(s).y);
        pointcount++;
      }
      /* Apparently START_POINT for one spline section is same as END_POINT
         for previous section - should gfloatly test for this */
      if (SPLINE_DEGREE(s) == LINEARTYPE) {
        pointx[pointcount] = FIG_X(END_POINT(s).x);
        pointy[pointcount] = FIG_Y(END_POINT(s).y);
        contrl[pointcount] = (gfloat)0.0;
        fig_addtobbox(START_POINT(s).x, START_POINT(s).y);
        pointcount++;
      } else { /* Assume Bezier like spline */

        /* Convert approximated bezier to interpolated X Spline */
        gfloat temp;
        for (temp = (gfloat)0.2; temp < (gfloat)0.9; temp += (gfloat)0.2) {
          pointx[pointcount] =
              FIG_X(bezpnt(temp, START_POINT(s).x, CONTROL1(s).x, CONTROL2(s).x, END_POINT(s).x));
          pointy[pointcount] =
              FIG_Y(bezpnt(temp, START_POINT(s).y, CONTROL1(s).y, CONTROL2(s).y, END_POINT(s).y));
          contrl[pointcount] = (gfloat)-1.0;
          pointcount++;
        }
        pointx[pointcount] = FIG_X(END_POINT(s).x);
        pointy[pointcount] = FIG_Y(END_POINT(s).y);
        contrl[pointcount] = (gfloat)0.0;
        fig_addtobbox(START_POINT(s).x, START_POINT(s).y);
        fig_addtobbox(CONTROL1(s).x, CONTROL1(s).y);
        fig_addtobbox(CONTROL2(s).x, CONTROL2(s).y);
        fig_addtobbox(END_POINT(s).x, END_POINT(s).y);
        pointcount++;
        is_spline = 1;
      }
    }
    if (shape.centerline) {
      fig_fill = -1;
      fig_width = 1;
      fig_spline_close = 4;
    } else {
      /* Use zero width lines - unit width is too thick */
      fig_fill = 20;
      fig_width = 0;
      fig_spline_close = 5;
    }
    if (is_spline != 0) {
      fig_new_depth();
      fprintf(file, "3 %d 0 %d %d %d %d 0 %d 0.00 0 0 0 %d\n", fig_spline_close, fig_width,
              fig_colour, fig_colour, fig_depth, fig_fill, pointcount);
      /* Print out points */
      j = 0;
      for (i = 0; i < pointcount; i++) {
        j++;
        if (j == 1) {
          fprintf(file, "\t");
        }
        fprintf(file, "%d %d ", pointx[i], pointy[i]);
        if (j == 8) {
          fprintf(file, "\n");
          j = 0;
        }
      }
      if (j != 0) {
        fprintf(file, "\n");
      }
      j = 0;
      /* Print out control weights */
      for (i = 0; i < pointcount; i++) {
        j++;
        if (j == 1) {
          fprintf(file, "\t");
        }
        fprintf(file, "%f ", contrl[i]);
        if (j == 8) {
          fprintf(file, "\n");
          j = 0;
        }
      }
      if (j != 0) {
        fprintf(file, "\n");
      }
    } else {
      /* Polygons can be handled better as polygons */
      fig_subt = 3;
      if (pointcount == 2) {
        if ((pointx[0] == pointx[1]) && (pointy[0] == pointy[1])) {
          /* Point */
          fig_new_depth();
          fprintf(file, "2 1 0 1 %d %d %d 0 -1 0.000 0 0 -1 0 0 1\n", fig_colour, fig_colour,
                  fig_depth);
          fprintf(file, "\t%d %d\n", pointx[0], pointy[0]);
        } else {
          /* Line segment? */
          fig_new_depth();
          fprintf(file, "2 1 0 1 %d %d %d 0 -1 0.000 0 0 -1 0 0 2\n", fig_colour, fig_colour,
                  fig_depth);
          fprintf(file, "\t%d %d %d %d\n", pointx[0], pointy[0], pointx[1], pointy[1]);
        }
      } else {
        if ((pointcount == 3) && (pointx[0] == pointx[2]) && (pointy[0] == pointy[2])) {
          /* Line segment? */
          fig_new_depth();
          fprintf(file, "2 1 0 1 %d %d %d 0 -1 0.000 0 0 -1 0 0 2\n", fig_colour, fig_colour,
                  fig_depth);
          fprintf(file, "\t%d %d %d %d\n", pointx[0], pointy[0], pointx[1], pointy[1]);
        } else {
          if ((pointx[0] != pointx[pointcount - 1]) || (pointy[0] != pointy[pointcount - 1])) {
            if (shape.centerline) {
              fig_subt = 1;
            } else {
              /* Need to have last point same as first for polygon */
              pointx[pointcount] = pointx[0];
              pointy[pointcount] = pointy[0];
              pointcount++;
            }
          }
          fig_new_depth();
          fprintf(file, "2 %d 0 %d %d %d %d 0 %d 0.00 0 0 0 0 0 %d\n", fig_subt, fig_width,
                  fig_colour, fig_colour, fig_depth, fig_fill, pointcount);
          /* Print out points */
          j = 0;
          for (i = 0; i < pointcount; i++) {
            j++;
            if (j == 1) {
              fprintf(file, "\t");
            }
            fprintf(file, "%d %d ", pointx[i], pointy[i]);
            if (j == 8) {
              fprintf(file, "\n");
              j = 0;
            }
          }
          if (j != 0) {
            fprintf(file, "\n");
          }
        }
      }
    }
    /*	fig_depth--; */
    if (fig_depth < 0) {
      fig_depth = 0;
    }
  }
}

int output_fig_writer(FILE *file, gchar *name, int llx, int lly, int urx, int ury,
                      at_output_opts_type *opts, spline_list_array_type shape, at_msg_func msg_func,
                      gpointer msg_data, gpointer user_data)
{
  at_exception_type exp = at_exception_new(msg_func, msg_data);
  /*	Output header	*/
  fprintf(file, "#FIG 3.2\nLandscape\nCenter\nInches\nLetter\n100.00\nSingle\n-2\n1200 2\n");

  /*	Output data	*/
  out_fig_splines(file, shape, llx, lly, urx, ury, &exp);
  return 0;
}

static guint fig_colour_key(at_color c)
{
  return ((guint)c.r << 16) | ((guint)c.g << 8) | c.b;
}

/* Create the colour table with the eight predefined FIG colours in it.  */
static fig_colour_table *fig_col_init(void)
{
  static const struct {
    at_color c;
    int number;
  } predefined[] = {
      {{0, 0, 0}, FIG_BLACK},      {{0, 0, 255}, FIG_BLUE},      {{0, 255, 0}, FIG_GREEN},
      {{0, 255, 255}, FIG_CYAN},   {{255, 0, 0}, FIG_RED},       {{255, 0, 255}, FIG_MAGENTA},
      {{255, 255, 0}, FIG_YELLOW}, {{255, 255, 255}, FIG_WHITE},
  };
  fig_colour_table *table = g_new(fig_colour_table, 1);

  table->index = g_hash_table_new(g_direct_hash, g_direct_equal);
  table->user = g_array_new(FALSE, FALSE, sizeof(at_color));
  for (gsize i = 0; i < G_N_ELEMENTS(predefined); i++)
    g_hash_table_insert(table->index, GUINT_TO_POINTER(fig_colour_key(predefined[i].c)),
                        GINT_TO_POINTER(predefined[i].number));
  return table;
}

static void fig_col_free(fig_colour_table *table)
{
  g_hash_table_unref(table->index);
  g_array_unref(table->user);
  g_free(table);
}

/*
 * Return the FIG colour number of the RGB triplet.
 * If unknown, allocate the next user colour number and return that.
 */

static int get_fig_colour(fig_colour_table *table, at_color this_colour, at_exception_type *exp)
{
  gpointer key = GUINT_TO_POINTER(fig_colour_key(this_colour));
  gpointer number;

  if (g_hash_table_lookup_extended(table->index, key, NULL, &number))
    return GPOINTER_TO_INT(number);

  int next = FIG_USER_COLOUR + table->user->len;
  if (next > MAX_FIG_COLOUR) {
    LOG("Output-Fig: too many colours: %d", next);
    at_exception_fatal(exp, "Output-Fig: too many colours");
    return 0;
  }
  g_array_append_val(table->user, this_colour);
  g_hash_table_insert(table->index, key, GINT_TO_POINTER(next));
  return next;
}
