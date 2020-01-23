#include <stdlib.h>
#include <time.h>
#include <config.h>

char *at_time_string(void)
{
  char *time_string = calloc(sizeof(char), 50);
  time_t t = time(NULL);
#if HAVE_LOCALTIME_R
  struct tm newtime;

  localtime_r(&t, &newtime);
  strftime(time_string, 50, "%c", &newtime);
#else
  // the mingw-w64 toolchain does not have localtime_r()
  strftime(time_string, 50, "%c", localtime(&t));
#endif

  return time_string;
}
