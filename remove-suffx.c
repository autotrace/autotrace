/* remove-suffx.c: remove any suffix.

Copyright (C) 1999 Free Software Foundation, Inc.

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

#include "types.h"
#include "find-suffix.h"
#include "substring.h"
#include <stdio.h>

string
remove_suffix (string s)
{
  string suffix = find_suffix (s);

  return suffix == NULL ? s : substring (s, 0, suffix - 2 - s);
}

/* version 0.17 */
