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

#define LOG(s)								\
  do { if (log_file) fputs (s, log_file); } while (0)
#define LOG1(s, e)							\
  do { if (log_file) fprintf (log_file, s, e); } while (0)
#define LOG2(s, e1, e2)							\
  do { if (log_file) fprintf (log_file, s, e1, e2); } while (0)
#define LOG3(s, e1, e2, e3)						\
  do { if (log_file) fprintf (log_file, s, e1, e2, e3); } while (0)
#define LOG4(s, e1, e2, e3, e4)						\
  do { if (log_file) fprintf (log_file, s, e1, e2, e3, e4); } while (0)
#define LOG5(s, e1, e2, e3, e4, e5)					\
  do { if (log_file) fprintf (log_file, s, e1, e2, e3, e4, e5); } while (0)

#endif /* not LOGREPORT_H */

