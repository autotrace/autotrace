/* epsilon-equal.c: define a error resist compare. */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* Def: HAVE_CONFIG_H */

#include "types.h"
#include "epsilon-equal.h"
#include <math.h>

/* Numerical errors sometimes make a floating point number just slightly
   larger or smaller than its TRUE value.  When it matters, we need to
   compare with some tolerance, REAL_EPSILON, defined in kbase.h.  */

gboolean epsilon_equal(gfloat v1, gfloat v2)
{
  if (v1 == v2                  /* Usually they'll be exactly equal, anyway.  */
      || fabs(v1 - v2) <= REAL_EPSILON)
    return TRUE;

  return FALSE;
}
