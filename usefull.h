/* useful.h: extend the standard programming environment a little.  This
   is included from config.h, which everyone includes. */

#ifndef USEFULL_H
#define USEFULL_H

/* Define useful abbreviations.  */

/* Some simple numeric operations.  It is possible to define these much
   more cleanly in GNU C, but we haven't done that (yet).  */
#define SQUARE(x) ((x) * (x))
#define CUBE(x) ((x) * (x) * (x))
#define	SAME_SIGN(u,v) ((u) >= 0 && (v) >= 0 || (u) < 0 && (v) < 0)
#define ROUND(x) ((int) ((int) (x) + .5 * SIGN (x)))
#define SIGN(x) ((x) > 0 ? 1 : (x) < 0 ? -1 : 0)

#ifndef MAX
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif
#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif

/* Too bad C doesn't define operators for these.  */
#define MAX_EQUALS(var, expr) if ((expr) > (var)) (var) = (expr)
#define MIN_EQUALS(var, expr) if ((expr) < (var)) (var) = (expr)

#endif /* not USEFULL_H */

/* version 0.25 */
