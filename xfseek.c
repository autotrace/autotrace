/* xfseek.c: fseek with error checking. */

#include "message.h"
#include "types.h"
#include "xfile.h"
#include <errno.h>

void
xfseek (FILE *f, long offset, int wherefrom, string filename)
{
  if (fseek (f, offset, wherefrom) < 0)
    FATAL_PERROR (filename);
}

/* version 0.17 */
