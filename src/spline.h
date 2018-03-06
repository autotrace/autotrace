/* spline.h: manipulate the spline representation.
   Some of macrs are only renamed macros in output.h. */

#ifndef SPLINE_H
#define SPLINE_H

#include <stdio.h>
#include "autotrace.h"
#include "output.h"

typedef at_polynomial_degree polynomial_degree;
typedef at_spline_type spline_type;

#define LINEARTYPE          AT_LINEARTYPE
#define QUADRATICTYPE       AT_QUADRATICTYPE
#define CUBICTYPE           AT_CUBICTYPE
#define PARALLELELLIPSETYPE AT_PARALLELELLIPSETYPE
#define ELLIPSETYPE         AT_ELLIPSETYPE
#define CIRCLETYPE          AT_CIRCLETYPE

#define START_POINT	    AT_SPLINE_START_POINT_VALUE
#define CONTROL1            AT_SPLINE_CONTROL1_VALUE
#define CONTROL2            AT_SPLINE_CONTROL2_VALUE
#define END_POINT           AT_SPLINE_END_POINT_VALUE
#define SPLINE_DEGREE	    AT_SPLINE_DEGREE_VALUE
#define SPLINE_LINEARITY(spl)	((spl).linearity)

#ifndef _IMPORTING
/* Print a spline on the given file.  */
extern void print_spline(spline_type);

/* Evaluate SPLINE at the given T value.  */
extern at_real_coord evaluate_spline(spline_type spline, gfloat t);
#endif

/* Each outline in a character is typically represented by many
   splines.  So, here is a list structure for that:  */
typedef at_spline_list_type spline_list_type;

/* An empty list will have length zero (and null data).  */
#define SPLINE_LIST_LENGTH  AT_SPLINE_LIST_LENGTH_VALUE

/* The address of the beginning of the array of data.  */
#define SPLINE_LIST_DATA    AT_SPLINE_LIST_DATA_VALUE

/* The element INDEX in S_L.  */
#define SPLINE_LIST_ELT     AT_SPLINE_LIST_ELT_VALUE

/* The last element in S_L.  */
#define LAST_SPLINE_LIST_ELT(s_l) \
  (SPLINE_LIST_DATA (s_l)[SPLINE_LIST_LENGTH (s_l) - 1])

/* The previous and next elements to INDEX in S_L.  */
#define NEXT_SPLINE_LIST_ELT(s_l, index)				\
  SPLINE_LIST_ELT (s_l, ((index) + 1) % SPLINE_LIST_LENGTH (s_l))
#define PREV_SPLINE_LIST_ELT(s_l, index)				\
  SPLINE_LIST_ELT (s_l, index == 0					\
                        ? SPLINE_LIST_LENGTH (s_l) - 1			\
                        : index - 1)

#ifndef _IMPORTING
/* Construct and destroy new `spline_list_type' objects.  */
extern spline_list_type *new_spline_list(void); /* Allocate new memory */
extern spline_list_type empty_spline_list(void);  /* No allocation */
extern spline_list_type *new_spline_list_with_spline(spline_type);
extern void free_spline_list(spline_list_type);

/* Append the spline S to the list S_LIST.  */
extern void append_spline(spline_list_type * s_list, spline_type s);

/* Append the elements in list S2 to S1, changing S1.  */
extern void concat_spline_lists(spline_list_type * s1, spline_list_type s2);
#endif

typedef at_spline_list_array_type spline_list_array_type;

/* Turns out we can use the same definitions for lists of lists as for
   just lists.  But we define the usual names, just in case.  */
#define SPLINE_LIST_ARRAY_LENGTH   AT_SPLINE_LIST_ARRAY_LENGTH_VALUE
#define SPLINE_LIST_ARRAY_DATA     SPLINE_LIST_DATA
#define SPLINE_LIST_ARRAY_ELT      AT_SPLINE_LIST_ARRAY_ELT_VALUE
#define LAST_SPLINE_LIST_ARRAY_ELT LAST_SPLINE_LIST_ELT

extern spline_list_array_type new_spline_list_array(void);
extern void append_spline_list(spline_list_array_type *, spline_list_type);
extern void free_spline_list_array(spline_list_array_type *);

#endif /* not SPLINE_H */
