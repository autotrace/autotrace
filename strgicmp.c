#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* Def: HAVE_CONFIG_H */

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

at_bool strgnicmp (const char *s1, const char *s2, size_t len)
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
  if ((*s1 == '\0' && *s2 == '\0') || len == i)
	return (true);
  else
    return (false);
}

#if 0
#include <stdio.h>
void
result (at_bool val)
{
  if (val)
    printf("successful\n");
  else
    printf("failed\n");
}
int main()
{
  result(strgicmp("abc", "abc"));
  result(!strgicmp("abc", "abcd"));
  result(strgicmp("abc", "ABC"));
  result(strgicmp("abc", "abC"));
  result(!strgicmp("abc", "abCd"));

  result(strgnicmp("abc", "abc", 3));
  result(strgnicmp("abc", "abcd", 3));
  result(!strgnicmp("abc", "abcd", 4));
  result(strgnicmp("abc", "ABC", 3));
  result(strgnicmp("abc", "ABZ", 2));
  result(strgnicmp("abc", "abC", 3));
  result(strgnicmp("abc", "abCdddd", 3));
  return 0;
}
#endif /* 0 */
