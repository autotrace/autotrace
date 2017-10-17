/* message.h: extend the standard programming environment a little.  This
   is included from config.h, which everyone includes. */

#ifndef MESSAGE_H
#define MESSAGE_H

#include "logreport.h"
#include "types.h"
#include <stdio.h>
#include <stdlib.h>

/* Define common sorts of messages.  */

/* This should be called only after a system call fails.  */
#define FATAL_PERROR(s) do { perror (s); exit (errno); } while (0)

#define FATAL(...)							\
  do { fputs ("fatal: ", stderr); LOG("fatal: "); fprintf (stderr, __VA_ARGS__); LOG (__VA_ARGS__); fputs (".\n", stderr); exit (1); } while (0)

#define WARNING(...)							\
  do { fputs ("warning: ", stderr); LOG ("warning: "); fprintf (stderr, __VA_ARGS__); LOG (__VA_ARGS__); fputs (".\n", stderr); } while (0)

#endif /* not MESSAGE_H */
