/* xfopen.c: fopen and fclose with error checking. */

#include "message.h"
#include "types.h"
#include "xfile.h"
#include <errno.h>

/* These routines just check the return status from standard library
   routines and abort if an error happens.  */

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

/* version 0.17 */
