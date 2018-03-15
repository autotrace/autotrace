#include <stdlib.h>
#include <time.h>

char *at_time_string(void)
{
  char *time_string = calloc(sizeof(char), 25);
  struct tm newtime;
  time_t t = time(NULL);

  localtime_r(&t, &newtime);
  strftime(time_string, 25, "%c", &newtime);

  return time_string;
}
