/* logreport.h: status reporting routines. */

#ifndef LOGREPORT_H
#define LOGREPORT_H

#include <stdio.h>
#include "types.h"

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

#endif /* not LOGREPORT_H */

