/* Qunatize a high color bitmap */

#ifndef QUANTIZE_H
#define QUANTIZE_H

#define PRECISION_R 	6
#define PRECISION_G 	6
#define PRECISION_B 	6

#define HIST_R_ELEMS 	(1<<PRECISION_R)
#define HIST_G_ELEMS 	(1<<PRECISION_G)
#define HIST_B_ELEMS 	(1<<PRECISION_B)

#define MR 		HIST_G_ELEMS*HIST_B_ELEMS
#define MG 		HIST_B_ELEMS

void quantize(unsigned char *src,unsigned char *dest,int width, int height,long ncolors);

#endif /* NOT QUANTIZE_H */

/* version 0.24 */