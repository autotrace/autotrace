#include "xstd.h"
#include <errno.h>

/* xfopen.c: fopen and fclose with error checking. */
FILE *
xfopen (string filename, string mode)
{
  FILE *f = fopen (filename, mode);

  if (f == NULL)
    FATAL_PERROR (filename);

  return f;
}


void
xfclose (FILE *f, string filename)
{
  if (fclose (f) == EOF)
    FATAL_PERROR (filename);
}

void
xfseek (FILE *f, long offset, int wherefrom, string filename)
{
  if (fseek (f, offset, wherefrom) < 0)
    FATAL_PERROR (filename);
}

