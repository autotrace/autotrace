/* epsilon-equal.c: define a error resist compare. */

#include "ptypes.h"
#include "epsilon-equal.h"
#include <math.h>

/* Numerical errors sometimes make a floating point number just slightly
   larger or smaller than its true value.  When it matters, we need to
   compare with some tolerance, REAL_EPSILON, defined in kbase.h.  */

bool
epsilon_equal (real v1, real v2)
{
  return
    v1 == v2		       /* Usually they'll be exactly equal, anyway.  */
    || fabs (v1 - v2) <= REAL_EPSILON;
}

