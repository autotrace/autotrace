/* vector.c: vector/point operations. */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* Def: HAVE_CONFIG_H */

#include "vector.h"
#include "message.h"
#include "epsilon-equal.h"
#include <math.h>
#include <errno.h>
#include <assert.h>
#include <string.h>

static at_real acos_d (at_real, at_exception_type * excep);

#ifndef M_PI
#define M_PI 3.14159265
#endif

#define SIGN(x) ((x) > 0 ? 1 : (x) < 0 ? -1 : 0)
#define ROUND(x) ((int) ((int) (x) + .5 * SIGN (x)))

/* Given the point COORD, return the corresponding vector.  */

vector_type
make_vector (const at_real_coord c)
{
  vector_type v;

  v.dx = c.x;
  v.dy = c.y;
  v.dz = c.z;

  return v;
}


/* And the converse: given a vector, return the corresponding point.  */

at_real_coord
vector_to_point (const vector_type v)
{
  at_real_coord coord;

  coord.x = v.dx;
  coord.y = v.dy;

  return coord;
}


at_real
magnitude (const vector_type v)
{
  return (at_real) sqrt (v.dx * v.dx + v.dy * v.dy + v.dz * v.dz);
}


vector_type
normalize (const vector_type v)
{
  vector_type new_v;
  at_real m = magnitude (v);

  /* assert (m > 0.0); */

  if (m > 0.0)
  {
    new_v.dx = v.dx / m;
    new_v.dy = v.dy / m;
    new_v.dz = v.dz / m;
  }
  else
  {
	new_v.dx = v.dx;
    new_v.dy = v.dy;
    new_v.dz = v.dz;
  }

  return new_v;
}


vector_type
Vadd (const vector_type v1, const vector_type v2)
{
  vector_type new_v;

  new_v.dx = v1.dx + v2.dx;
  new_v.dy = v1.dy + v2.dy;
  new_v.dz = v1.dz + v2.dz;

  return new_v;
}


at_real
Vdot (const vector_type v1, const vector_type v2)
{
  return v1.dx * v2.dx + v1.dy * v2.dy + v1.dz * v2.dz;
}


vector_type
Vmult_scalar (const vector_type v, const at_real r)
{
  vector_type new_v;

  new_v.dx = v.dx * r;
  new_v.dy = v.dy * r;
  new_v.dz = v.dz * r;

  return new_v;
}


/* Given the IN_VECTOR and OUT_VECTOR, return the angle between them in
   degrees, in the range zero to 180.  */

at_real
Vangle (const vector_type in_vector, 
	const vector_type out_vector,
	at_exception_type * exp)
{
  vector_type v1 = normalize (in_vector);
  vector_type v2 = normalize (out_vector);

  return acos_d (Vdot (v2, v1), exp);
}


at_real_coord
Vadd_point (const at_real_coord c, const vector_type v)
{
  at_real_coord new_c;

  new_c.x = c.x + v.dx;
  new_c.y = c.y + v.dy;
  new_c.z = c.z + v.dz;
  return new_c;
}


at_real_coord
Vsubtract_point (const at_real_coord c, const vector_type v)
{
  at_real_coord new_c;

  new_c.x = c.x - v.dx;
  new_c.y = c.y - v.dy;
  new_c.z = c.z - v.dz;
  return new_c;
}


at_coord
Vadd_int_point (const at_coord c, const vector_type v)
{
  at_coord a;

  a.x = (unsigned short) ROUND ((at_real) c.x + v.dx);
  a.y = (unsigned short) ROUND ((at_real) c.y + v.dy);
  return a;
}


vector_type
Vabs (const vector_type v)
{
  vector_type new_v;

  new_v.dx = (at_real) fabs (v.dx);
  new_v.dy = (at_real) fabs (v.dy);
  new_v.dz = (at_real) fabs (v.dz);
  return new_v;
}


/* Operations on points.  */

at_real_coord
Padd (const at_real_coord coord1, const at_real_coord coord2)
{
  at_real_coord sum;

  sum.x = coord1.x + coord2.x;
  sum.y = coord1.y + coord2.y;
  sum.z = coord1.z + coord2.z;

  return sum;
}


at_real_coord
Pmult_scalar (const at_real_coord coord, const at_real r)
{
  at_real_coord answer;

  answer.x = coord.x * r;
  answer.y = coord.y * r;
  answer.z = coord.z * r;

  return answer;
}


vector_type
Psubtract (const at_real_coord c1, const at_real_coord c2)
{
  vector_type v;

  v.dx = c1.x - c2.x;
  v.dy = c1.y - c2.y;
  v.dz = c1.z - c2.z;

  return v;
}



/* Operations on integer points.  */

vector_type
IPsubtract (const at_coord coord1, const at_coord coord2)
{
  vector_type v;

  v.dx = (at_real) (coord1.x - coord2.x);
  v.dy = (at_real) (coord1.y - coord2.y);
  v.dz = 0.0;

  return v;
}


at_coord
IPsubtractP (const at_coord c1, const at_coord c2)
{
  at_coord c;

  c.x = c1.x - c2.x;
  c.y = c1.y - c2.y;

  return c;
}


at_coord
IPadd (const at_coord c1, const at_coord c2)
{
  at_coord c;

  c.x = c1.x + c2.x;
  c.y = c1.y + c2.y;

  return c;
}


at_coord
IPmult_scalar (const at_coord c, const int i)
{
  at_coord a;

  a.x = (unsigned short) (c.x * i);
  a.y = (unsigned short) (c.y * i);

  return a;
}


at_real_coord
IPmult_real (const at_coord c, const at_real r)
{
  at_real_coord a;

  a.x = c.x * r;
  a.y = c.y * r;

  return a;
}


at_bool
IPequal (const at_coord c1, const at_coord c2)
{
  if ((c1.x == c2.x) && (c1.y == c2.y))
    return true;
  else
    return false;
}

static at_real
acos_d (at_real v, at_exception_type * excep)
{
  at_real a;

  if (epsilon_equal (v, 1.0))
    v = 1.0;
  else if (epsilon_equal (v, -1.0))
    v = -1.0;

  errno = 0;
  a = (at_real) acos (v);
  if (errno == ERANGE || errno == EDOM)
    {
      at_exception_fatal(excep, strerror(errno));
      return 0.0;
    }
  
  
  return a * (at_real) 180.0 / (at_real) M_PI;
}
