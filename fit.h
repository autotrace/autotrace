/* fit.h: convert the pixel representation to splines. */

#ifndef FIT_H
#define FIT_H

#include "autotrace.h"
#include "image-proc.h"
#include "pxl-outline.h"
#include "spline.h"
#include "exception.h"

/* See fit.c for descriptions of these variables, all of which can be
   set using options.  */
typedef at_fitting_opts_type fitting_opts_type;

#ifdef _EXPORTING

/* Fit splines and lines to LIST.  */
extern spline_list_array_type __declspec(dllexport) __stdcall
  fitted_splines (pixel_outline_list_type, fitting_opts_type *,
          distance_map_type *,
          unsigned short width, unsigned short height,
		  at_exception * exception,
		  at_progress_func, at_address,
		  at_testcancel_func, at_address);

/* Get a new set of fitting options */
extern fitting_opts_type __declspec(dllexport)
__stdcall new_fitting_opts (void);

#elif _IMPORTING

/* Fit splines and lines to LIST.  */
extern spline_list_array_type __declspec(dllimport) __stdcall
  fitted_splines (pixel_outline_list_type, fitting_opts_type *,
          distance_map_type *,
		  unsigned short width, unsigned short height,
		  at_exception * exception,
		  at_progress_func, at_address,
		  at_testcancel_func, at_address);

/* Get a new set of fitting options */
extern fitting_opts_type __declspec(dllimport)
__stdcall new_fitting_opts (void);

#else

/* Fit splines and lines to LIST.  */
extern spline_list_array_type fitted_splines
  (pixel_outline_list_type, fitting_opts_type *, 
   distance_map_type *,
   unsigned short width, unsigned short height,
   at_exception * exception,
   at_progress_func, at_address,
   at_testcancel_func, at_address);   

/* Get a new set of fitting options */
extern fitting_opts_type new_fitting_opts (void);

#endif

#endif /* not FIT_H */
