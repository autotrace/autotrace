/* extend-fname.h: declarations for shared routines. */

#ifndef EXTEND_FNAME_H
#define EXTEND_FNAME_H

#include "types.h"

/* If NAME has a suffix, simply return it; otherwise, return
   `NAME.SUFFIX'.  */
extern string extend_filename (string name, string suffix);

#endif /* not EXTEND_FNAME_H */

/* version 0.17 */
