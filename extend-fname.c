/* extend-fname.c: give a filename a suffix, if necessary. */

#include "types.h"
#include "find-suffix.h"
#include "concat3.h"
#include "extend-fname.h"
#include <stdio.h>

string
extend_filename (string name, string default_suffix)
{
  string new_s;
  string suffix = find_suffix (name);

  new_s = suffix == NULL
          ? concat3 (name, ".", default_suffix) : name;
  return new_s;
}

/* version 0.17 */
