/* input-pnm.h: import pnm and pbm files. */

#ifndef INPUT_PNM_H
#define INPUT_PNM_H

#include "ptypes.h"
#include "bitmap.h"

bitmap_type pnm_load_image (at_string filename);

#endif /* not INPUT_PNM_H */

