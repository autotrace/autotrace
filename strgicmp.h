#ifndef STRGICMP_H
#define STRGICMP_H

#include "types.h"
#include "ptypes.h"

extern bool strgicmp (const unsigned char *s1, const unsigned char *s2);
extern bool strgnicmp (const unsigned char *s1, const unsigned char *s2, long len);

#endif /* STRGICMP_H */
