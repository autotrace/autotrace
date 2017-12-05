/* logreport.h: status reporting routines. */

#ifndef LOGREPORT_H
#define LOGREPORT_H

#include <stdio.h>
#include "types.h"
#include <stdlib.h>

#ifdef _EXPORTING
#define DECLSPEC __declspec(dllexport)
#elif _IMPORTING
#define DECLSPEC __declspec(dllimport)
#else
#define DECLSPEC
#endif

/* Whether to write a log */
extern gboolean logging;

#define LOG(...)								\
  do { if (logging) fprintf (stdout, __VA_ARGS__); } while (0)

/* Define common sorts of messages.  */

#define FATAL(...)							\
  do { fputs ("fatal: ", stderr); LOG("fatal: "); fprintf (stderr, __VA_ARGS__); LOG (__VA_ARGS__); fputs (".\n", stderr); exit (1); } while (0)

#define WARNING(...)							\
  do { fputs ("warning: ", stderr); LOG ("warning: "); fprintf (stderr, __VA_ARGS__); LOG (__VA_ARGS__); fputs (".\n", stderr); } while (0)

#endif /* not LOGREPORT_H */
