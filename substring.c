/* substring.c: copy a chunk from a string. */

#include "types.h"
#include "xmem.h"
#include <string.h>


/* Return a fresh copy of SOURCE[START..LIMIT], or NULL if LIMIT<START.
   If LIMIT>strlen(START), it is reassigned. */

string
substring (string source, const unsigned start, const unsigned limit)
{
  string result;
  unsigned this_char;
  unsigned length = strlen (source);
  unsigned lim = limit;

  /* Upper bound out of range? */
  if (lim >= length)
    lim = length - 1;

  /* Null substring? */
  if (start > lim)
    return "";

  /* The `2' here is one for the null and one for limit - start inclusive. */
  XMALLOC (result, lim - start + 2);

  for (this_char = start; this_char <= lim; this_char++)
    result[this_char - start] = source[this_char];

  result[this_char - start] = 0;

  return result;
}

/* version 0.17 */
