/* xfopen.c: fopen and fclose with error checking. */

#include "xstd.h"
#include <errno.h>

FILE *
xfopen (string filename, string mode)
{
  FILE *f;

  if (strcmp(filename, "-") == 0)
    f = stdin;
  else
    {
      f = fopen (filename, mode);
      if (f == NULL)
	FATAL_PERROR (filename);
    }

  return f;
}


void
xfclose (FILE *f, string filename)
{
  if (f != stdin)
    {
      if (fclose (f) == EOF)
	FATAL_PERROR (filename);
    }
}

void
xfseek (FILE *f, long offset, int wherefrom, string filename)
{
  if (fseek (f, offset, wherefrom) < 0)
    FATAL_PERROR (filename);
}

