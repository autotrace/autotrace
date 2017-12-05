/* atou.c: like atoi, but if the number is negative, abort. */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* Def: HAVE_CONFIG_H */

#include "logreport.h"
#include "atou.h"

unsigned atou(gchar * s)
{
  int i = atoi(s);

  if (i < 0)
    FATAL("I expected a positive number, not %d", i);

  return (unsigned)i;
}
