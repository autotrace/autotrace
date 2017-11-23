/* epsilon-equal.h: define an error resist compare. */

#ifndef EPSILON_EQUAL_H
#define EPSILON_EQUAL_H

#include "types.h"

/* Says whether V1 and V2 are within REAL_EPSILON of each other.
   Fixed-point arithmetic would be better, to guarantee machine
   independence, but it's so much more painful to work with.  The value
   here is smaller than can be represented in either a `fix_word' or a
   `scaled_num', so more precision than this will be lost when we
   output, anyway.  */
extern gboolean epsilon_equal(gfloat v1, gfloat v2);
#define REAL_EPSILON 0.00001

#endif /* not EPSILON_EQUAL_H */
