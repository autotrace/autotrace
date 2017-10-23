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

/* The file we write information to.  */
extern FILE DECLSPEC *log_file;

#define LOG(...)								\
  do { if (log_file) fprintf (log_file, __VA_ARGS__); } while (0)

#endif /* not LOGREPORT_H */

