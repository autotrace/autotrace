/* xmem.h: declarations for shared routines. */

#ifndef XMEM_H
#define XMEM_H

#include "types.h"
#include "message.h"
#include <string.h>

/* These call the corresponding function in the standard library, and
   abort if those routines fail. */

#ifndef __cplusplus
#define XMALLOC(new_mem, size)									\
do												\
  {												\
    new_mem = (address) malloc (size);								\
												\
    if (new_mem == NULL)									\
      FATAL3 ("malloc: request for %u bytes failed in %s line %d",				\
        (size), __FILE__, (unsigned) __LINE__);							\
  } while (0)


#define XCALLOC(new_mem, size)									\
do												\
  {												\
    new_mem = (address) calloc (size, 1);							\
												\
    if (new_mem == NULL)									\
      FATAL3 ("calloc: request for %u bytes failed in %s line %d",				\
        (size), __FILE__, (unsigned) __LINE__);							\
  } while (0)


#define XREALLOC(old_ptr, size)									\
do 												\
  {												\
    address new_mem;										\
												\
    if (old_ptr == NULL)									\
      XMALLOC (new_mem, size);									\
    else											\
      {												\
        new_mem = (address) realloc (old_ptr, size);						\
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
    (address&)(new_mem) = (address) malloc (size);						\
												\
    if ((address&)(new_mem) == NULL)								\
      FATAL3 ("malloc: request for %u bytes failed in %s line %d",				\
              (size), __FILE__, (unsigned) __LINE__);						\
  } while (0) 

 
#define XCALLOC(new_mem, sizex)									\
do												\
  {												\
    (address&)(new_mem) = (void *) calloc (sizex, 1);						\
												\
  } while (0) 
 
 
#define XREALLOC(old_ptr, size)									\
do 												\
  {												\
    address new_mem;										\
												\
    if (old_ptr == NULL)									\
      XMALLOC (new_mem, (size));								\
    else											\
      {												\
        (address&) new_mem = (address) realloc ((old_ptr), (size));				\
        if (new_mem == NULL)									\
          FATAL4 ("realloc: request for %lx to be %u bytes failed in %s line %d",		\
                  (unsigned long) (old_ptr), (size), __FILE__, (unsigned) __LINE__);		\
      }												\
												\
    (address&)old_ptr = new_mem;								\
  } while (0) 
#endif

#endif /* not XMEM_H */
