/* cmdline.h: macros to help process command-line arguments. */

#ifndef CMDLINE_H
#define CMDLINE_H

#include "getopt.h"
#include "types.h"
#include <string.h>

/* Test whether getopt found an option ``A''.
   Assumes the option index is in the variable `option_index', and the
   option table in a variable `long_options'.  */

#define ARGUMENT_IS(a) (long_options[option_index].name != NULL && a != NULL && 0 == strcasecmp(long_options[option_index].name, a))

/* Perform common actions at the end of parsing the arguments.  Assumes
   lots of variables: `printed_version', a boolean for whether the
   version number has been printed; `optind', the current option index;
   `argc'; `argv'.  */

#define FINISH_COMMAND_LINE()						\
  do									\
    {									\
      /* Just wanted to know the version number?  */			\
      if (printed_version && optind == argc) exit (0);			\
                                                                        \
      /* Exactly one (non-empty) argument left?  */			\
      if (optind + 1 == argc && *argv[optind] != 0)			\
        {								\
          return (argv[optind]);					\
        }								\
      else								\
        {								\
          fprintf (stderr, "Usage: %s [options] <image_name>.\n", argv[0]);\
          fprintf (stderr, "(%s.)\n", optind == argc ? "Missing <image_name>"\
                                      : "Too many <image_name>s");	\
          fputs ("For more information, use ``-help''.\n", stderr);	\
          exit (1);							\
        }								\
      return NULL; /* stop warnings */					\
    }									\
  while (0)

#define GETOPT_USAGE \
"  You can use `--' or `-' to start an option.\n\
  You can use any unambiguous abbreviation for an option name.\n\
  You can separate option names and values with `=' or ` '.\n\
"

/* What to pass to `strtok' to separate different arguments to an
   option, as in `-option=arg1,arg2,arg3'.  It is useful to allow
   whitespace as well so that the option value can come from a file, via
   the shell construct "`cat file`" (including the quotes).  */
#define ARG_SEP ", \t\n"

#endif /* not CMDLINE_H */
