/* exception.h: facility to handle error in autotrace */

#ifndef AT_EXCEPTION_H
#define AT_EXCEPTION_H 

#include "autotrace.h"
#include "types.h"

typedef struct at_exception_type at_exception;
struct at_exception_type
{
  at_bool got_fatal;
  at_msg_func client_func;
  at_address * client_data;
};

at_exception at_exception_new(at_msg_func client_func,
			      at_address * client_data);
at_bool at_exception_got_fatal(at_exception * exception);
void at_exception_fatal(at_exception * exception,
			at_string message);
void at_exception_warning(at_exception * exception,
			  at_string message);

#endif /* Not def: AT_EXCEPTION_H */
