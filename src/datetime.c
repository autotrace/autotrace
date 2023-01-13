
/* datetime.c: date and time specific operations

   Copyright (C) 2023 Peter Lemenkov

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

#include <glib.h>
#include <glib/gprintf.h>

gchar *at_time_string(void)
{
	GDateTime *date;
	gchar *date_string;
	date = g_date_time_new_now_local ();
	date_string = g_date_time_format (date, "%a %b %e %H:%M:%S %Y");
	g_date_time_unref (date);
	return date_string;
}
