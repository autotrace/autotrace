/* atou.c: like atoi, but if the number is negative, abort. */

#include "message.h"
#include "atou.h"

const unsigned
atou (string s)
{
  int i = atoi (s);

  if (i < 0)
    FATAL1 ("I expected a positive number, not %d", i);

  return (unsigned) i;
}

/* version 0.24 */
