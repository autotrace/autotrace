/* input-bmp.h: import bmp files. */

#ifndef INPUT_BMP_H
#define INPUT_BMP_H

#include "input.h"

at_bitmap_type bmp_load_image (at_string filename,
			       at_msg_func msg_func, 
			       at_address msg_data); 

#endif /* not INPUT_BMP_H */

