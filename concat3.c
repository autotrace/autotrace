/* concat3.c: concatenate three strings. */

#include "types.h"
#include "concat3.h"
#include "xmem.h"
#include <string.h>

string
concat3 (string s1, string s2, string s3)
{
  string answer;
  XMALLOC (answer, strlen (s1) + strlen (s2) + strlen (s3) + 1);
  strcpy (answer, s1);
  strcat (answer, s2);
  strcat (answer, s3);

  return answer;
}

/* version 0.17 */
