/* xfile.h: Like their stdio counterparts, but abort on error, after calling
   perror(3) with FILENAME as its argument. */

#ifndef XFILE_H
#define XFILE_H

#include "types.h"
#include <stdio.h>

/* Like their stdio counterparts, but abort on error, after calling
   perror(3) with FILENAME as its argument.  */
extern FILE *xfopen (string filename, string mode);
extern void xfclose (FILE *, string filename);
extern void xfseek (FILE *, long, int, string filename);

#endif /* not XFILE_H */

/* version 0.17 */
