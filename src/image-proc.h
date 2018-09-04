/* image-proc.h: image processing routines */

#ifndef IMAGE_PROC_H
#define IMAGE_PROC_H

#include "bitmap.h"
#include "color.h"

typedef struct {
  unsigned height, width;
  float **weight;
  float **d;
} at_distance_map;

/* Allocate and compute a new distance map. */
extern at_distance_map new_distance_map(at_bitmap *, unsigned char target_value, gboolean padded, at_exception_type * exp);

/* Free the dynamically-allocated storage associated with a distance map. */
extern void free_distance_map(at_distance_map *);

#endif /* not IMAGE_PROC_H */
