/* input-tga.h: import tga files. */

#ifndef INPUT_TGA_H
#define INPUT_TGA_H

#include "input.h"

at_bitmap_type tga_load_image (at_string filename,
			       at_msg_func msg_func, 
			       at_address msg_data);

#endif /* not INPUT_TGA_H */

