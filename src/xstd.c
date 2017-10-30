/* xfopen.c: fopen with error checking. */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* Def: HAVE_CONFIG_H */

#include "xstd.h"
#include <errno.h>

FILE *
xfopen (gchar* filename, gchar* mode)
{
  FILE *f;

  if (strcmp(filename, "-") == 0)
    f = stdin;
  else
    {
      f = fopen (filename, mode);
      if (f == NULL) {
	perror (filename);
	exit (errno);
      }
    }

  return f;
}

