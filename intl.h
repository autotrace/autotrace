#ifndef __INTL_H__
#define __INTL_H__

#include "config.h"

#ifdef ENABLE_NLS
#include <libintl.h>
#define _(String) dgettext(PACKAGE,String)
#define N_(String) (String)
#else /* NLS is disabled */
#define _(String) (String)
#define N_(String) (String)
#define textdomain(String) (String)
#define gettext(String) (String)
#define dgettext(Domain,String) (String)
#define dcgettext(Domain,String,Type) (String)
#define bindtextdomain(Domain,Directory) (Domain) 
#endif

#endif
