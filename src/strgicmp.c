#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* Def: HAVE_CONFIG_H */

#include "strgicmp.h"
#include <ctype.h>

gboolean strgicmp(const char *s1, const char *s2)
{
  if (s1 == NULL || s2 == NULL)
    return (FALSE);

  while (*s1 != '\0' && *s2 != '\0') {
    if (tolower(*s1) != tolower(*s2))
      break;
    s1++;
    s2++;
  }
  if (*s1 == '\0' && *s2 == '\0')
    return (TRUE);
  else
    return (FALSE);
}

#if 0
#include <stdio.h>
void result(gboolean val)
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
  return 0;
}
#endif /* 0 */
