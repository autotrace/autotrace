/*
 * SPDX-FileCopyrightText: © 2002 Martin Weber
 * SPDX-FileCopyrightText: © 2002-2003 Masatake YAMATO
 * SPDX-FileCopyrightText: © 2017-2025 Peter Lemenkov
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "exception.h"
#include "logreport.h"

/* The code of the fatal message that at_exception_fatal_code() or
   at_msg_fatal_code() is passing to a callback in this thread, plus 1, so
   that NULL means none.  The callback of a function with a GError reads
   it (issue #345): readers report errors only as strings.  */
static GPrivate current_code;

AtError at_exception_current_code(void)
{
  gpointer code = g_private_get(&current_code);
  return code ? (AtError)(GPOINTER_TO_INT(code) - 1) : AT_ERROR_FAILED;
}

static void fatal_with_code(at_msg_func msg_func, gpointer msg_data, AtError code,
                            const gchar *message)
{
  gpointer previous = g_private_get(&current_code);

  g_private_set(&current_code, GINT_TO_POINTER((gint)code + 1));
  msg_func(message, AT_MSG_FATAL, msg_data);
  g_private_set(&current_code, previous);
}

at_exception_type at_exception_new(at_msg_func client_func, gpointer client_data)
{
  at_exception_type e;
  e.msg_type = AT_MSG_NOT_SET;
  e.client_func = client_func;
  e.client_data = client_data;
  return e;
}

gboolean at_exception_got_fatal(at_exception_type *exception)
{
  return exception->msg_type == AT_MSG_FATAL;
}

void at_exception_fatal(at_exception_type *exception, const gchar *message)
{
  if (!exception)
    return;
  exception->msg_type = AT_MSG_FATAL;
  if (exception->client_func)
    /* A fatal message without a code is AT_ERROR_FAILED, even when the
       callback runs inside one with a code (a callback of the client's
       that calls libautotrace again).  */
    fatal_with_code(exception->client_func, exception->client_data, AT_ERROR_FAILED, message);
}

void at_exception_fatal_code(at_exception_type *exception, AtError code, const gchar *format, ...)
{
  va_list args;
  gchar *message;

  if (!exception)
    return;
  exception->msg_type = AT_MSG_FATAL;
  if (!exception->client_func)
    return;
  va_start(args, format);
  message = g_strdup_vprintf(format, args);
  va_end(args);
  fatal_with_code(exception->client_func, exception->client_data, code, message);
  g_free(message);
}

void at_msg_fatal_code(at_msg_func msg_func, gpointer msg_data, AtError code, const gchar *format,
                       ...)
{
  va_list args;
  gchar *message;

  if (!msg_func)
    return;
  va_start(args, format);
  message = g_strdup_vprintf(format, args);
  va_end(args);
  fatal_with_code(msg_func, msg_data, code, message);
  g_free(message);
}

void at_exception_warning(at_exception_type *exception, const gchar *message)
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
