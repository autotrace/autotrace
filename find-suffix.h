/* find-suffix.h: declarations for shared routines. */

#ifndef FIND_SUFFIX_H
#define FIND_SUFFIX_H

#include "types.h"

/* If NAME has a suffix, return a pointer to its first character (i.e.,
   the one after the `.'); otherwise, return NULL.  */
extern string find_suffix (string name);

#endif /* not FIND_SUFFIX_H */

/* version 0.17 */
