#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* Def: HAVE_CONFIG_H */

#include "exception.h"

at_exception_type at_exception_new(at_msg_func client_func, gpointer client_data)
{
  at_exception_type e;
  e.msg_type = AT_MSG_NOT_SET;
  e.client_func = client_func;
  e.client_data = client_data;
  return e;
}

gboolean at_exception_got_fatal(at_exception_type * exception)
{
  return (exception->msg_type == AT_MSG_FATAL) ? TRUE : FALSE;
}

void at_exception_fatal(at_exception_type * exception, const gchar * message)
{
  if (!exception)
    return;
  exception->msg_type = AT_MSG_FATAL;
  if (exception->client_func) {
    exception->client_func(message, AT_MSG_FATAL, exception->client_data);
  }
}

void at_exception_warning(at_exception_type * exception, const gchar * message)
{
  if (!exception)
    return;
  exception->msg_type = AT_MSG_WARNING;
  if (exception->client_func) {
    exception->client_func(message, AT_MSG_WARNING, exception->client_data);
  }
}

GQuark at_error_quark(void)
{
  static GQuark q = 0;
  if (q == 0)
    q = g_quark_from_static_string("at-error-quark");
  return q;
}
