/* xstd.h: Wrappers for functions in C standard library 
 Was: xmem, xfile */

/* These call the corresponding function in the standard library, and
   abort if those routines fail. */

#ifndef XSTD_H
#define XSTD_H 

#include "types.h"
#include "message.h"
#include <stdio.h>
#include <string.h>

/*
 * XMEM
 */
#ifndef __cplusplus
#define XMALLOC(new_mem, size)									\
do												\
  {												\
    new_mem = (at_address) malloc (size);								\
												\
    if (new_mem == NULL)									\
      FATAL3 ("malloc: request for %u bytes failed in %s line %d",				\
        (size), __FILE__, (unsigned) __LINE__);							\
  } while (0)


#define XCALLOC(new_mem, size)									\
do												\
  {												\
    new_mem = (at_address) calloc (size, 1);							\
												\
    if (new_mem == NULL)									\
      FATAL3 ("calloc: request for %u bytes failed in %s line %d",				\
        (size), __FILE__, (unsigned) __LINE__);							\
  } while (0)


#define XREALLOC(old_ptr, size)									\
do 												\
  {												\
    at_address new_mem;										\
												\
    if (old_ptr == NULL)									\
      XMALLOC (new_mem, size);									\
    else											\
      {												\
        new_mem = (at_address) realloc (old_ptr, size);						\
	if (new_mem == NULL)									\
          FATAL4 ("realloc: request for %lx to be %u bytes failed in %s line %d",		\
            (unsigned long) (old_ptr), (size), __FILE__, (unsigned) __LINE__);			\
      }												\
       												\
    old_ptr = new_mem;										\
} while (0)


#else
/* Use templates if Cplusplus... */
#define XMALLOC(new_mem, size)									\
do												\
  {												\
    (at_address&)(new_mem) = (at_address) malloc (size);						\
												\
    if ((at_address&)(new_mem) == NULL)								\
      FATAL3 ("malloc: request for %u bytes failed in %s line %d",				\
              (size), __FILE__, (unsigned) __LINE__);						\
  } while (0) 

 
#define XCALLOC(new_mem, sizex)									\
do												\
  {												\
    (at_address&)(new_mem) = (void *) calloc (sizex, 1);						\
												\
  } while (0) 
 
 
#define XREALLOC(old_ptr, size)									\
do 												\
  {												\
    at_address new_mem;										\
												\
    if (old_ptr == NULL)									\
      XMALLOC (new_mem, (size));								\
    else											\
      {												\
        (at_address&) new_mem = (at_address) realloc ((old_ptr), (size));				\
        if (new_mem == NULL)									\
          FATAL4 ("realloc: request for %lx to be %u bytes failed in %s line %d",		\
                  (unsigned long) (old_ptr), (size), __FILE__, (unsigned) __LINE__);		\
      }												\
												\
    (at_address&)old_ptr = new_mem;								\
  } while (0) 
#endif

/*
 * XFILE
 */
/* Like their stdio counterparts, but abort on error, after calling
   perror(3) with FILENAME as its argument.  */
extern FILE *xfopen (at_string filename, at_string mode);
extern void xfclose (FILE *, at_string filename);
extern void xfseek (FILE *, long, int, at_string filename);

#endif /* Not XSTD_H */
