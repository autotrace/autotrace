/* make-suffix.c: unconditionally change the suffix on a string. */

#include "types.h"
#include "find-suffix.h"
#include "concat3.h"
#include "xmem.h"
#include <string.h>


string
make_suffix (string s, string new_suffix)
{
  string new_s;
  string old_suffix = find_suffix (s);

  if (old_suffix == NULL)
    new_s = concat3 (s, ".", new_suffix);
  else
    {
      unsigned length_through_dot = old_suffix - s;

      XMALLOC (new_s, length_through_dot + strlen (new_suffix) + 1);
      strncpy (new_s, s, length_through_dot);
      strcpy (new_s + length_through_dot, new_suffix);
    }

  return new_s;
}

/* version 0.17 */
