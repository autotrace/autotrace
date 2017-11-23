/* output.h: interface for output handlers

   Copyright (C) 1999, 2000, 2001, 2002 Bernhard Herzog.

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

#ifndef OUTPUT_H
#define OUTPUT_H
#include <stdio.h>
#include "autotrace.h"
#include "types.h"
#include "exception.h"
#include <glib.h>

#ifdef __cplusplus
extern "C" {
#endif                          /* __cplusplus */

  typedef
  int (*at_output_func) (FILE *, gchar * name, int llx, int lly, int urx, int ury, at_output_opts_type * opts, at_splines_type shape, at_msg_func msg_func, gpointer msg_data, gpointer user_data);

  extern int at_output_add_handler(const gchar * suffix, const gchar * description, at_output_func writer);

  extern int at_output_add_handler_full(const gchar * suffix, const gchar * description, at_output_func writer, gboolean override, gpointer user_data, GDestroyNotify user_data_destroy_func);

/* Data struct hierarchy:
   spline_list_array (splines)
   -> spline_list...
   --> spline */

/* Accessors to the Data member  */
#define AT_SPLINE_START_POINT_VALUE(spl)      ((spl).v[0])
#define AT_SPLINE_CONTROL1_VALUE(spl)	      ((spl).v[1])
#define AT_SPLINE_CONTROL2_VALUE(spl)         ((spl).v[2])
#define AT_SPLINE_END_POINT_VALUE(spl)        ((spl).v[3])
#define AT_SPLINE_DEGREE_VALUE(spl)           ((spl).degree)

#define AT_SPLINE_START_POINT(spl)            (&AT_SPLINE_START_POINT_VALUE(*(spl)))
#define AT_SPLINE_CONTROL1(spl)	              (&AT_SPLINE_CONTROL1_VALUE(*(spl)))
#define AT_SPLINE_CONTROL2(spl)               (&AT_SPLINE_CONTROL2_VALUE(*(spl)))
#define AT_SPLINE_END_POINT(spl)              (&AT_SPLINE_END_POINT_VALUE(*(spl)))
#define AT_SPLINE_DEGREE(spl)	              AT_SPLINE_DEGREE_VALUE(*(spl))

#define AT_SPLINE_LIST_LENGTH_VALUE(spll)     ((spll).length)
#define AT_SPLINE_LIST_LENGTH(spll)           AT_SPLINE_LIST_LENGTH_VALUE(*(spll))
#define AT_SPLINE_LIST_DATA_VALUE(spll)       ((spll).data)
#define AT_SPLINE_LIST_DATA(spll)             AT_SPLINE_LIST_DATA_VALUE((*spll))
#define AT_SPLINE_LIST_ELT_VALUE(spll,index)  AT_SPLINE_LIST_DATA_VALUE(spll)[(index)]
#define AT_SPLINE_LIST_ELT(spll,index)        (&(AT_SPLINE_LIST_ELT_VALUE((*spll), (index))))
#define AT_SPLINE_LIST_COLOR_VALUE(spll)      ((spll).color)
#define AT_SPLINE_LIST_COLOR(spll)            (&(AT_SPLINE_LIST_COLOR_VALUE(*spll)))
#define AT_SPLINE_LIST_IS_OPENED_VALUE(spll)  ((spll).open)
#define AT_SPLINE_LIST_IS_OPENED(spll)        AT_SPLINE_LIST_IS_OPENED_VALUE(*(spll))

#define AT_SPLINE_LIST_ARRAY_LENGTH_VALUE     AT_SPLINE_LIST_LENGTH_VALUE
#define AT_SPLINE_LIST_ARRAY_LENGTH           AT_SPLINE_LIST_LENGTH
#define AT_SPLINE_LIST_ARRAY_ELT_VALUE        AT_SPLINE_LIST_ELT_VALUE
#define AT_SPLINE_LIST_ARRAY_ELT              AT_SPLINE_LIST_ELT

#define AT_SPLINE_LIST_ARRAY_IS_CENTERLINE_VALUE(splla) ((splla).centerline)
#define AT_SPLINE_LIST_ARRAY_IS_CENTERLINE(splla)       AT_SPLINE_LIST_ARRAY_IS_CENTERLINE_VALUE(*(splla))

/*
 * Glib style traversing
 */

  typedef void (*AtSplineListForeachFunc) (at_spline_list_type * spline_list, at_spline_type * spline, int index, gpointer user_data);
  typedef void (*AtSplineListArrayForeachFunc) (at_spline_list_array_type * spline_list_array, at_spline_list_type * spline_list, int index, gpointer user_data);

  void at_spline_list_foreach(at_spline_list_type *, AtSplineListForeachFunc func, gpointer user_data);
  void at_spline_list_array_foreach(at_spline_list_array_type *, AtSplineListArrayForeachFunc func, gpointer user_data);

#ifdef __cplusplus
}
#endif                          /* __cplusplus */
#endif                          /* not OUTPUT_H */
