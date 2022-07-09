/*
 * Copyright (C) 1992 Free Software Foundation, Inc.
 * SPDX-FileCopyrightText: © 2000-2002 Martin Weber
 * SPDX-FileCopyrightText: © 2000-2003 Masatake YAMATO
 * SPDX-FileCopyrightText: © 2017-2025 Peter Lemenkov
 * SPDX-FileCopyrightText: © 2022 Biswapriyo Nath
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "logreport.h"
#include "curve.h"
#include <glib.h>

static at_real_coord int_to_real_coord(at_coord);

/* Return an entirely empty curve.  */

curve_type new_curve(void)
{
  curve_type curve = g_malloc(sizeof(struct curve));
  curve->point_list = NULL;
  CURVE_LENGTH(curve) = 0;
  CURVE_CYCLIC(curve) = FALSE;
  CURVE_START_TANGENT(curve) = CURVE_END_TANGENT(curve) = NULL;
  PREVIOUS_CURVE(curve) = NEXT_CURVE(curve) = NULL;

  return curve;
}

/* Don't copy the points or tangents, but copy everything else.  */

curve_type copy_most_of_curve(curve_type old_curve)
{
  curve_type curve = new_curve();

  CURVE_CYCLIC(curve) = CURVE_CYCLIC(old_curve);
  PREVIOUS_CURVE(curve) = PREVIOUS_CURVE(old_curve);
  NEXT_CURVE(curve) = NEXT_CURVE(old_curve);

  return curve;
}

/* The length of CURVE will be zero if we ended up not being able to fit
   it (which in turn implies a problem elsewhere in the program, but at
   any rate, we shouldn't try here to free the nonexistent curve).  */

void free_curve(curve_type curve)
{
  if (CURVE_LENGTH(curve) > 0)
    g_free(curve->point_list);
  if (CURVE_START_TANGENT(curve))
    g_free(CURVE_START_TANGENT(curve));
  if (CURVE_END_TANGENT(curve))
    g_free(CURVE_END_TANGENT(curve));
}

void append_pixel(curve_type curve, at_coord coord)
{
  append_point(curve, int_to_real_coord(coord));
}

void append_point(curve_type curve, at_real_coord coord)
{
  CURVE_LENGTH(curve)++;
  curve->point_list = g_realloc(curve->point_list, CURVE_LENGTH(curve) * sizeof(point_type));
  LAST_CURVE_POINT(curve) = coord;
  /* The t value does not need to be set.  */
}

/* Print a curve in human-readable form.  It turns out we never care
   about most of the points on the curve, and so it is pointless to
   print them all out umpteen times.  What matters is that we have some
   from the end and some from the beginning.  */

#define NUM_TO_PRINT 3

#define DEBUG_CURVE_POINT(c, p, print_t)                                                           \
  do {                                                                                             \
    DEBUG("(%.3f,%.3f)", CURVE_POINT(c, p).x, CURVE_POINT(c, p).y);                                \
    if (print_t)                                                                                   \
      DEBUG("/%.2f", CURVE_T(c, p));                                                               \
  } while (0)

void log_curve(curve_type curve, gboolean print_t)
{
  unsigned this_point;

  DEBUG("curve id = %lx:\n", (unsigned long)(uintptr_t)curve);
  DEBUG("  length = %u.\n", CURVE_LENGTH(curve));
  if (CURVE_CYCLIC(curve))
    DEBUG("  cyclic.\n");

  /* It should suffice to check just one of the tangents for being null
     -- either they both should be, or neither should be.  */
  if (CURVE_START_TANGENT(curve) != NULL)
    DEBUG("  tangents = (%.3f,%.3f) & (%.3f,%.3f).\n", CURVE_START_TANGENT(curve)->dx,
          CURVE_START_TANGENT(curve)->dy, CURVE_END_TANGENT(curve)->dx,
          CURVE_END_TANGENT(curve)->dy);

  DEBUG("  ");

  /* If the curve is short enough, don't use ellipses.  */
  if (CURVE_LENGTH(curve) <= NUM_TO_PRINT * 2) {
    for (this_point = 0; this_point < CURVE_LENGTH(curve); this_point++) {
      DEBUG_CURVE_POINT(curve, this_point, print_t);
      DEBUG(" ");

      if (this_point != CURVE_LENGTH(curve) - 1 && (this_point + 1) % NUM_TO_PRINT == 0)
        DEBUG("\n  ");
    }
  } else {
    for (this_point = 0; this_point < NUM_TO_PRINT && this_point < CURVE_LENGTH(curve);
         this_point++) {
      DEBUG_CURVE_POINT(curve, this_point, print_t);
      DEBUG(" ");
    }

    DEBUG("...\n   ...");

    for (this_point = CURVE_LENGTH(curve) - NUM_TO_PRINT; this_point < CURVE_LENGTH(curve);
         this_point++) {
      DEBUG(" ");
      DEBUG_CURVE_POINT(curve, this_point, print_t);
    }
  }

  DEBUG(".\n");
}

/* Like `log_curve', but write the whole thing.  */

void log_entire_curve(curve_type curve)
{
  unsigned this_point;

  DEBUG("curve id = %lx:\n", (unsigned long)(uintptr_t)curve);
  DEBUG("  length = %u.\n", CURVE_LENGTH(curve));
  if (CURVE_CYCLIC(curve))
    DEBUG("  cyclic.\n");

  /* It should suffice to check just one of the tangents for being null
     -- either they both should be, or neither should be.  */
  if (CURVE_START_TANGENT(curve) != NULL)
    DEBUG("  tangents = (%.3f,%.3f) & (%.3f,%.3f).\n", CURVE_START_TANGENT(curve)->dx,
          CURVE_START_TANGENT(curve)->dy, CURVE_END_TANGENT(curve)->dx,
          CURVE_END_TANGENT(curve)->dy);

  DEBUG(" ");

  for (this_point = 0; this_point < CURVE_LENGTH(curve); this_point++) {
    DEBUG(" ");
    DEBUG_CURVE_POINT(curve, this_point, TRUE);
    /* Compiler warning `Condition is always true' can be ignored */
  }

  DEBUG(".\n");
}

/* Return an initialized but empty curve list.  */

curve_list_type new_curve_list(void)
{
  curve_list_type curve_list;

  curve_list.length = 0;
  curve_list.data = NULL;

  return curve_list;
}

/* Free a curve list and all the curves it contains.  */

void free_curve_list(curve_list_type *curve_list)
{
  unsigned this_curve;

  for (this_curve = 0; this_curve < curve_list->length; this_curve++) {
    free_curve(curve_list->data[this_curve]);
    g_free(curve_list->data[this_curve]);
  }

  /* If the character was empty, it won't have any curves.  */
  g_free(curve_list->data);
}

/* Add an element to a curve list.  */

void append_curve(curve_list_type *curve_list, curve_type curve)
{
  curve_list->length++;
  curve_list->data = g_realloc(curve_list->data, curve_list->length * sizeof(curve_type));
  curve_list->data[curve_list->length - 1] = curve;
}

/* Return an initialized but empty curve list array.  */

curve_list_array_type new_curve_list_array(void)
{
  curve_list_array_type curve_list_array;

  CURVE_LIST_ARRAY_LENGTH(curve_list_array) = 0;
  curve_list_array.data = NULL;

  return curve_list_array;
}

/* Free a curve list array and all the curve lists it contains.  */

void free_curve_list_array(curve_list_array_type *curve_list_array,
                           at_progress_func notify_progress, gpointer client_data)
{
  unsigned this_list;

  for (this_list = 0; this_list < CURVE_LIST_ARRAY_LENGTH(*curve_list_array); this_list++) {
    if (notify_progress)
      notify_progress(((gfloat)this_list) /
                              (CURVE_LIST_ARRAY_LENGTH(*curve_list_array) * (gfloat)3.0) +
                          (gfloat)0.666,
                      client_data);
    free_curve_list(&CURVE_LIST_ARRAY_ELT(*curve_list_array, this_list));
  }

  /* If the character was empty, it won't have any curves.  */
  g_free(curve_list_array->data);
}

/* Add an element to a curve list array.  */

void append_curve_list(curve_list_array_type *curve_list_array, curve_list_type curve_list)
{
  CURVE_LIST_ARRAY_LENGTH(*curve_list_array)++;
  curve_list_array->data = g_realloc(
      curve_list_array->data, CURVE_LIST_ARRAY_LENGTH(*curve_list_array) * sizeof(curve_list_type));
  LAST_CURVE_LIST_ARRAY_ELT(*curve_list_array) = curve_list;
}

/* Turn an integer point into a real one.  */

static at_real_coord int_to_real_coord(at_coord int_coord)
{
  at_real_coord real_coord;

  real_coord.x = int_coord.x;
  real_coord.y = int_coord.y;
  real_coord.z = 0.0;

  return real_coord;
}
