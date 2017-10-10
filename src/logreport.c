/* logreport.c: showing information to the user. */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* Def: HAVE_CONFIG_H */

#include "logreport.h"
#include "message.h"

#ifdef _EXPORTING
/* Says whether to output detailed progress reports, i.e., all the data
   on the fitting, as we run.  (-log)  */
FILE __declspec(dllexport) *log_file = NULL;

#else
/* Says whether to output detailed progress reports, i.e., all the data
   on the fitting, as we run.  (-log)  */
FILE *log_file = NULL;

#endif

void
flush_log_output (void)
{
  if (log_file)
    fflush (log_file);
}

