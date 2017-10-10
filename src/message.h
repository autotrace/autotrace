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


#define START_FATAL() do { fputs ("fatal: ", stderr); LOG("fatal: ")
#define END_FATAL() fputs (".\n", stderr); exit (1); } while (0)

#define FATAL(s)							\
  START_FATAL (); fprintf (stderr, "%s", s); LOG (s); END_FATAL ()
#define FATAL1(s, e1)							\
  START_FATAL (); fprintf (stderr, s, e1); LOG1 (s, e1); END_FATAL ()
#define FATAL2(s, e1, e2)						\
  START_FATAL (); fprintf (stderr, s, e1, e2); LOG2 (s, e1, e2); END_FATAL ()
#define FATAL3(s, e1, e2, e3)						\
  START_FATAL (); fprintf (stderr, s, e1, e2, e3); LOG3 (s, e1, e2, e3); END_FATAL ()
#define FATAL4(s, e1, e2, e3, e4)					\
  START_FATAL (); fprintf (stderr, s, e1, e2, e3, e4); LOG4 (s, e1, e2, e3, e4); END_FATAL ()


#define START_WARNING() do { fputs ("warning: ", stderr); LOG ("warning: ")
#define END_WARNING() fputs (".\n", stderr); } while (0)

#define WARNING(s)							\
  START_WARNING (); fprintf (stderr, "%s", s); LOG (s); END_WARNING ()
#define WARNING1(s, e1)							\
  START_WARNING (); fprintf (stderr, s, e1); LOG1 (s, e1); END_WARNING ()
#define WARNING2(s, e1, e2)						\
  START_WARNING (); fprintf (stderr, s, e1, e2); LOG2 (s, e1, e2); END_WARNING ()
#define WARNING3(s, e1, e2, e3)						\
  START_WARNING (); fprintf (stderr, s, e1, e2, e3); LOG3 (s, e1, e2, e3); END_WARNING ()
#define WARNING4(s, e1, e2, e3, e4)					\
  START_WARNING (); fprintf (stderr, s, e1, e2, e3, e4); LOG4 (s, e1, e2, e3, e4); END_WARNING ()

#endif /* not MESSAGE_H */

