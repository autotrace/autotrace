#include "strgicmp.h"
#include <ctype.h>

at_bool strgicmp (const char *s1, const char *s2)
{
  if (s1 == NULL || s2 == NULL)
    return (false);

  while (*s1 != '\0' && *s2 != '\0')
    {
      if (tolower (*s1) != tolower (*s2))
        break;
      s1++;
      s2++;
    }
  if (*s1 == '\0' && *s2 == '\0')
	return (true);
  else
    return (false);
}

at_bool strgnicmp (const char *s1, const char *s2, long len)
{
  long i = 0;

  if (s1 == NULL || s2 == NULL)
    return (false);

  while (*s1 != '\0' && *s2 != '\0')
    {
      if (tolower (*s1) != tolower (*s2) || i == len)
        break;
      s1++;
      s2++;
	  i++;
    }
  if (*s1 == '\0' && *s2 == '\0' || len == i)
	return (true);
  else
    return (false);
}
