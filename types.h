/* types.h: general types. */

#ifndef TYPES_H
#define TYPES_H

#ifndef __cplusplus
/* Booleans.  */
#ifndef bool
typedef enum { false = 0, true = 1 } bool;
#endif
#endif

/* The X11 library defines `FALSE' and `TRUE', and so we only want to
   define them if necessary.  */
#ifndef FALSE
#define FALSE false
#define TRUE true
#endif /* FALSE */

/* The usual null-terminated string.  */
typedef char *string;

/* A generic pointer in ANSI C.  */
typedef void *address;

/* We use `real' for our floating-point variables.  */
typedef float real;

/* Dimensions of a rectangle.  */
typedef struct
{
  unsigned height, width;
} dimensions_type;

#define DIMENSIONS_HEIGHT(d) ((d).height)
#define DIMENSIONS_WIDTH(d) ((d).width)

/* Cartesian points.  */
typedef struct
{
  short x, y;
} coordinate_type;

typedef struct
{
  real x, y;
} real_coordinate_type;

/* Curvetype */
#define LINE 1
#define QUAD_BEZIER 2
#define CUB_BEZIER 4
#define CIRCLE 8
#define PARALLEL_ELLIPSE 16
#define ELLIPSE 32

#endif /* not TYPES_H */
