/* image-proc.h: image processing routines */

#ifndef IMAGE_PROC_H
#define IMAGE_PROC_H

#include "bitmap.h"
#include "color.h"


/* Threshold for binarizing a monochrome image */
#define GRAY_THRESHOLD 225

/* RGB to grayscale */
#define LUMINANCE(r, g, b) ((r) * 0.30 + (g) * 0.59 + (b) * 0.11 + 0.5)


typedef struct
{
  unsigned height, width;
  float **weight;
  float **d;
} distance_map_type;


/* Allocate and compute a new distance map. */
extern distance_map_type new_distance_map(bitmap_type,
    unsigned char target_value, at_bool padded,
					  at_exception_type * exp);

/* Free the dynamically-allocated storage associated with a distance map. */
extern void free_distance_map(distance_map_type*);


extern void medial_axis(bitmap_type *bitmap, distance_map_type *dist,
    const color_type *bg_color);


/* Binarize a grayscale or color image. */
extern void binarize(bitmap_type*);


/* Thin a binary image, replacing the original image with the thinned one. */
extern bitmap_type ip_thin(bitmap_type);

#endif /* not IMAGE_PROC_H */

