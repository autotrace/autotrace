/* input-pnm.h: import pnm and pbm files. */

#ifndef INPUT_PNM_H
#define INPUT_PNM_H

#include "input.h"

at_bitmap_type pnm_load_image (at_string filename,
			       at_msg_func msg_func, 
			       at_address msg_data);

#endif /* not INPUT_PNM_H */

