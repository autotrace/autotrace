/* $Id: input-png.h,v 1.4 2002/01/14 17:27:33 masata-y Exp $ */
/* input-png.h: import png files. */
#ifndef INPUT_PNG_H
#define INPUT_PNG_H

#include "input.h"

at_bitmap_type png_load_image(at_string filename, 
			      at_msg_func msg_func, 
			      at_address msg_data);

#endif /* not INPUT_PNG_H */
