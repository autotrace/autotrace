/* make-suffix.h: declarations for shared routines. */

#ifndef MAKE_SUFFIX_H
#define MAKE_SUFFIX_H

/* Return S with the suffix SUFFIX, removing any suffix already present.
   For example, `make_suffix ("/foo/bar.baz", "karl")' returns
   `/foo/bar.karl'.  Returns a string allocated with malloc.  */
extern string make_suffix (string s, string suffix);

#endif /* not MAKE_SUFFIX_H */

/* version 0.17 */
