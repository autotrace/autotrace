/* vector.c: vector/point operations. */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* Def: HAVE_CONFIG_H */

#include "vector.h"
#include "logreport.h"
#include "epsilon-equal.h"
#include <math.h>
#include <errno.h>
#include <assert.h>
#include <string.h>

static gfloat acos_d(gfloat, at_exception_type * excep);

#define _USE_MATH_DEFINES
#include <math.h>

/* Given the point COORD, return the corresponding vector.  */

vector_type make_vector(const at_real_coord c)
{
  vector_type v;

  v.dx = c.x;
  v.dy = c.y;
  v.dz = c.z;

  return v;
}

/* And the converse: given a vector, return the corresponding point.  */

at_real_coord vector_to_point(const vector_type v)
{
  at_real_coord coord;

  coord.x = v.dx;
  coord.y = v.dy;

  return coord;
}

gfloat magnitude(const vector_type v)
{
  return (gfloat) sqrt(v.dx * v.dx + v.dy * v.dy + v.dz * v.dz);
}

vector_type normalize(const vector_type v)
{
  vector_type new_v;
  gfloat m = magnitude(v);

  /* assert (m > 0.0); */

  if (m > 0.0) {
    new_v.dx = v.dx / m;
    new_v.dy = v.dy / m;
    new_v.dz = v.dz / m;
  } else {
    new_v.dx = v.dx;
    new_v.dy = v.dy;
    new_v.dz = v.dz;
  }

  return new_v;
}

vector_type Vadd(const vector_type v1, const vector_type v2)
{
  vector_type new_v;

  new_v.dx = v1.dx + v2.dx;
  new_v.dy = v1.dy + v2.dy;
  new_v.dz = v1.dz + v2.dz;

  return new_v;
}

gfloat Vdot(const vector_type v1, const vector_type v2)
{
  return v1.dx * v2.dx + v1.dy * v2.dy + v1.dz * v2.dz;
}

vector_type Vmult_scalar(const vector_type v, const gfloat r)
{
  vector_type new_v;

  new_v.dx = v.dx * r;
  new_v.dy = v.dy * r;
  new_v.dz = v.dz * r;

  return new_v;
}

/* Given the IN_VECTOR and OUT_VECTOR, return the angle between them in
   degrees, in the range zero to 180.  */

gfloat Vangle(const vector_type in_vector, const vector_type out_vector, at_exception_type * exp)
{
  vector_type v1 = normalize(in_vector);
  vector_type v2 = normalize(out_vector);

  return acos_d(Vdot(v2, v1), exp);
}

at_real_coord Vadd_point(const at_real_coord c, const vector_type v)
{
  at_real_coord new_c;

  new_c.x = c.x + v.dx;
  new_c.y = c.y + v.dy;
  new_c.z = c.z + v.dz;
  return new_c;
}

at_real_coord Vsubtract_point(const at_real_coord c, const vector_type v)
{
  at_real_coord new_c;

  new_c.x = c.x - v.dx;
  new_c.y = c.y - v.dy;
  new_c.z = c.z - v.dz;
  return new_c;
}

at_coord Vadd_int_point(const at_coord c, const vector_type v)
{
  at_coord a;

  a.x = (unsigned short)lround((gfloat) c.x + v.dx);
  a.y = (unsigned short)lround((gfloat) c.y + v.dy);
  return a;
}

vector_type Vabs(const vector_type v)
{
  vector_type new_v;

  new_v.dx = (gfloat) fabs(v.dx);
  new_v.dy = (gfloat) fabs(v.dy);
  new_v.dz = (gfloat) fabs(v.dz);
  return new_v;
}

/* Operations on points.  */

at_real_coord Padd(const at_real_coord coord1, const at_real_coord coord2)
{
  at_real_coord sum;

  sum.x = coord1.x + coord2.x;
  sum.y = coord1.y + coord2.y;
  sum.z = coord1.z + coord2.z;

  return sum;
}

at_real_coord Pmult_scalar(const at_real_coord coord, const gfloat r)
{
  at_real_coord answer;

  answer.x = coord.x * r;
  answer.y = coord.y * r;
  answer.z = coord.z * r;

  return answer;
}

vector_type Psubtract(const at_real_coord c1, const at_real_coord c2)
{
  vector_type v;

  v.dx = c1.x - c2.x;
  v.dy = c1.y - c2.y;
  v.dz = c1.z - c2.z;

  return v;
}

/* Operations on integer points.  */

vector_type IPsubtract(const at_coord coord1, const at_coord coord2)
{
  vector_type v;

  v.dx = (gfloat) (coord1.x - coord2.x);
  v.dy = (gfloat) (coord1.y - coord2.y);
  v.dz = 0.0;

  return v;
}

at_coord IPsubtractP(const at_coord c1, const at_coord c2)
{
  at_coord c;

  c.x = c1.x - c2.x;
  c.y = c1.y - c2.y;

  return c;
}

at_coord IPadd(const at_coord c1, const at_coord c2)
{
  at_coord c;

  c.x = c1.x + c2.x;
  c.y = c1.y + c2.y;

  return c;
}

at_coord IPmult_scalar(const at_coord c, const int i)
{
  at_coord a;

  a.x = (unsigned short)(c.x * i);
  a.y = (unsigned short)(c.y * i);

  return a;
}

at_real_coord IPmult_real(const at_coord c, const gfloat r)
{
  at_real_coord a;

  a.x = c.x * r;
  a.y = c.y * r;

  return a;
}

gboolean IPequal(const at_coord c1, const at_coord c2)
{
  if ((c1.x == c2.x) && (c1.y == c2.y))
    return TRUE;
  else
    return FALSE;
}

static gfloat acos_d(gfloat v, at_exception_type * excep)
{
  gfloat a;

  if (epsilon_equal(v, 1.0))
    v = 1.0;
  else if (epsilon_equal(v, -1.0))
    v = -1.0;

  errno = 0;
  a = (gfloat) acos(v);
  if (errno == ERANGE || errno == EDOM) {
    at_exception_fatal(excep, strerror(errno));
    return 0.0;
  }

  return a * (gfloat) 180.0 / (gfloat) M_PI;
}
