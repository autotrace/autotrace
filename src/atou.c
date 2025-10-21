/* atou.c: like atoi, but if the number is negative, abort. */

#include "atou.h"
#include "logreport.h"
#include <glib.h>

unsigned atou(const gchar *s)
{
	guint64 val;

	if (!g_ascii_string_to_unsigned(s, 10, 0, UINT_MAX, &val, NULL)) {
		FATAL("Invalid unsigned integer: '%s'", s);
	}

	return (unsigned)val;
}
