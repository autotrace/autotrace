#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* Def: HAVE_CONFIG_H */

#include "exception.h"

at_exception_type
at_exception_new(at_msg_func client_func,
		 at_address client_data)
{
  at_exception_type e = {0, client_func, client_data};
  return e;
}

at_bool
at_exception_got_fatal(at_exception_type * exception)
{
  return (exception->msg_type == AT_MSG_FATAL);
}

void
at_exception_fatal(at_exception_type * exception,
		   const at_string message)
{
  if (!exception)
    return;
  exception->msg_type = AT_MSG_FATAL;
  if (exception->client_func)
    {
      exception->client_func(message, 
			     AT_MSG_FATAL,
			     exception->client_data);
    }
}

void 
at_exception_warning(at_exception_type * exception,
		     const at_string message)
{
  if (!exception)
    return;
  exception->msg_type = AT_MSG_WARNING;
  if (exception->client_func)
    {
      exception->client_func(message, 
			     AT_MSG_WARNING,
			     exception->client_data);
    }
}
