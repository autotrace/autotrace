/* atou.c: like atoi, but if the number is negative, abort. */

#include "logreport.h"
#include "atou.h"
#include <stdlib.h>

unsigned atou(gchar * s)
{
  int i = atoi(s);

  if (i < 0)
    FATAL("I expected a positive number, not %d", i);

  return (unsigned)i;
}
