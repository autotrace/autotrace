/* logreport.h: status reporting routines. */

#ifndef LOGREPORT_H
#define LOGREPORT_H

#include <stdio.h>
#include "types.h"

#ifdef _EXPORTING
/* The file we write information to.  */
extern FILE __declspec(dllexport) *log_file;

#elif _IMPORTING
/* The file we write information to.  */
extern FILE __declspec(dllimport) *log_file;

#else
/* The file we write information to.  */
extern FILE *at_log_file;
#define log_file at_log_file
#endif

extern void flush_log_output (void);

#define LOG(...)								\
  do { if (log_file) fprintf (log_file, __VA_ARGS__); } while (0)

#endif /* not LOGREPORT_H */

