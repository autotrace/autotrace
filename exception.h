/* exception.h: facility to handle error in autotrace */

#ifndef AT_EXCEPTION_H
#define AT_EXCEPTION_H 

#include "autotrace.h"
#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Protocol:
   If a function raises a FATAL(including propagation), 
   the function must release resources allocated by the 
   function itself. */
typedef struct _at_exception_type at_exception_type;
struct _at_exception_type
{
  at_msg_type msg_type;
  at_msg_func client_func;
  at_address * client_data;
};

at_exception_type at_exception_new(at_msg_func client_func,
				   at_address client_data);
at_bool at_exception_got_fatal(at_exception_type * exception);
void at_exception_fatal(at_exception_type * exception,
			const at_string message);
void at_exception_warning(at_exception_type * exception,
			  const at_string message);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* Not def: AT_EXCEPTION_H */
