/* exception.h: facility to handle error in autotrace */

#ifndef AT_EXCEPTION_H
#define AT_EXCEPTION_H

#include "autotrace.h"
#include "types.h"
#include <glib.h>

#ifdef __cplusplus
extern "C" {
#endif                          /* __cplusplus */

/* Protocol:
   If a function raises a FATAL(including propagation),
   the function must release resources allocated by the
   function itself. */
  typedef struct _at_exception_type at_exception_type;
  struct _at_exception_type {
    at_msg_type msg_type;
    at_msg_func client_func;
    gpointer client_data;
  };

  at_exception_type at_exception_new(at_msg_func client_func, gpointer client_data);
  gboolean at_exception_got_fatal(at_exception_type * exception);
  void at_exception_fatal(at_exception_type * exception, const gchar * message);
  void at_exception_warning(at_exception_type * exception, const gchar * message);

#define AT_ERROR at_error_quark()
  GQuark at_error_quark(void);
  typedef enum {
    AT_ERROR_WRONG_COLOR_STRING,
  } AtError;

#ifdef __cplusplus
}
#endif                          /* __cplusplus */
#endif                          /* Not def: AT_EXCEPTION_H */
