#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* Def: HAVE_CONFIG_H */

#include "exception.h"

at_exception
at_exception_new(at_msg_func client_func,
		 at_address client_data)
{
  at_exception e = {false, client_func, client_data};
  return e;
}

at_bool
at_exception_got_fatal(at_exception * exception)
{
  return exception->got_fatal;
}

void
at_exception_fatal(at_exception * exception,
		   const at_string message)
{
  if (!exception)
    return;
  exception->got_fatal = true;
  if (exception->client_func)
    {
      exception->client_func(message, 
			     AT_MSG_FATAL,
			     exception->client_data);
    }
}

void 
at_exception_warning(at_exception * exception,
		     const at_string message)
{
  if (!exception)
    return;
  if (exception->client_func)
    {
      exception->client_func(message, 
			     AT_MSG_WARNING,
			     exception->client_data);
    }
}
