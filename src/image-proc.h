/* image-proc.h: image processing routines */

#ifndef IMAGE_PROC_H
#define IMAGE_PROC_H

#include "bitmap.h"
#include "color.h"

/* Threshold for binarizing a monochrome image */
#define GRAY_THRESHOLD 225

/* RGB to grayscale */
#define LUMINANCE(r, g, b) ((r) * 0.30 + (g) * 0.59 + (b) * 0.11 + 0.5)

typedef struct {
  unsigned height, width;
  float **weight;
  float **d;
} at_distance_map;

/* Allocate and compute a new distance map. */
extern at_distance_map new_distance_map(at_bitmap *, unsigned char target_value, gboolean padded, at_exception_type * exp);

/* Free the dynamically-allocated storage associated with a distance map. */
extern void free_distance_map(at_distance_map *);

extern void medial_axis(at_bitmap * bitmap, at_distance_map * dist, const at_color * bg_color);

/* Binarize a grayscale or color image. */
extern void binarize(at_bitmap *);

/* Thin a binary image, replacing the original image with the thinned one. */
#if 0
extern at_bitmap *ip_thin(at_bitmap *);
#endif /* 0 */

#endif /* not IMAGE_PROC_H */
