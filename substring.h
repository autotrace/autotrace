/* substring.h: copy a chunk from a string. */

#ifndef SUBSTRING_H
#define SUBSTRING_H

#include "types.h"

/* Return a fresh copy of SOURCE[START..LIMIT], or NULL if LIMIT<START.
   If LIMIT>strlen(START), it is reassigned. */

string substring (string source, const unsigned start, const unsigned limit);

#endif /* not SUBSTRING_H */

/* version 0.17 */
