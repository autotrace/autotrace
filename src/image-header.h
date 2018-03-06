/* image-header.h: declarations for a generic image header. */

#ifndef IMAGE_HEADER_H
#define IMAGE_HEADER_H

#include "types.h"

/* The important general information about the image data.
   See `get_{img,pbm}_header' for the full details of the headers for
   the particular formats.  */
typedef struct {
  unsigned short hres, vres;    /* In pixels per inch.  */
  unsigned short width, height; /* In bits.  */
  unsigned short depth;         /* Perhaps the depth?  */
  unsigned format;              /* (for pbm) Whether packed or not.  */
} image_header_type;

#endif /* not IMAGE_HEADER_H */
