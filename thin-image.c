#include <stdlib.h>
#include <stdio.h>
#include "thin-image.h"
#include "logreport.h"
#include "types.h" 
#include "bitmap.h" 
#include <string.h>
 
#define BACKGROUND 0xffffff 
 
 
void thin (bitmap_type *image, unsigned int colour); 
 
typedef unsigned char Pixel;          /* Pixel data type              */ 
 
 
/* -------------------------------- ThinImage - Thin binary image. --------------------------- * 
 *                                                            
 *    Description:                                                    
 *        Thins the supplied binary image using Rosenfeld's parallel   
 *        thinning algorithm.                                         
 *                                                                     
 *    On Entry:                                                        
 *        image = Image to thin.                                       
 *                                                                     
 * -------------------------------------------------------------------------------------------- */ 
 
 
/* Direction masks:                  */ 
/*   N     S     W        E            */ 
static        unsigned int     masks[]         = { 0200, 0002, 0040, 0010 }; 
 
/*    True if pixel neighbor map indicates the pixel is 8-simple and  */ 
/*    not an end point and thus can be deleted.  The neighborhood     */ 
/*    map is defined as an integer of bits abcdefghi with a non-zero  */ 
/*    bit representing a non-zero pixel.  The bit assignment for the  */ 
/*    neighborhood is:                                                */ 
/*                                                                    */ 
/*                            a b c                                   */ 
/*                            d e f                                   */ 
/*                            g h i                                   */ 
 
static        unsigned char   delete[512] = { 
              0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
              0, 0, 0, 1, 0, 0, 1, 1, 0, 1, 1, 1, 0, 0, 1, 1, 
              0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
              0, 0, 1, 1, 1, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 
              0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
              0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 1, 1, 
              0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
              0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 1, 1, 
              0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
              0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 1, 1, 
              0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
              1, 0, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 
              0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
              1, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 1, 1, 
              0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
              1, 0, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 
              0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
              0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
              0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
              1, 0, 1, 1, 1, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 
              0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
              0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
              0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
              0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 1, 1, 
              0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
              1, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 1, 1, 
              0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
              1, 0, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 
              0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
              1, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 1, 1, 
              0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
              1, 0, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1}; 
 
unsigned int pixel (unsigned int x,unsigned int y, bitmap_type *image) 
{ 
      Pixel *p; 
      unsigned int v; 
      p = (BITMAP_PIXEL (*image,y,x)); 
      v = *p++; 
      if (BITMAP_PLANES(*image) == 3){ 
              v=(v*256) + *p++; 
              v=(v*256) + *p++; 
      }        
 
      /* return int value of pixel */ 
      return v; 
} 
 
void clr_pixel (unsigned int x, unsigned int y, bitmap_type *image) 
{ 
      Pixel *p = BITMAP_PIXEL(*image,y,x); 
      *p ++ = (char)(BACKGROUND%256); 
      if (BITMAP_PLANES(*image) == 3){ 
              *p++ = (char)(BACKGROUND%256); 
              *p = (char)(BACKGROUND%256); 
      }        
} 
 
void clr_colour ( unsigned int colour, bitmap_type *bm) 
{ 
      unsigned int x; 
      unsigned int y; 
      for (y=0; y < BITMAP_HEIGHT(*bm); y++){ 
              for (x=0; x < BITMAP_WIDTH(*bm); x++) 
                      if (pixel (x,y,bm) == colour) clr_pixel (x,y,bm); 
      } 
} 
 
void thin_image (bitmap_type *image) 
{ 
      /* This is nasty as we need to call thin once for each  
       * colour in the image the way I do this is to keep a second  
       * copy of the bitmap and to use this to keep 
       * track of which colours have not yet been processed, 
       * trades time for pathological case memory.....*/ 
      unsigned int x,y,p; 
      bitmap_type bm; 
      memcpy (&bm,image, sizeof (bitmap_type)); 
 
      bm.bitmap = malloc (3 * BITMAP_HEIGHT(bm) * BITMAP_WIDTH (bm)); 
      memcpy (bm.bitmap,image->bitmap,3 * BITMAP_HEIGHT(bm) * BITMAP_WIDTH (bm)); 
      /* that clones the image */ 
      for (y=0; y < BITMAP_HEIGHT(bm); y++){ 
              for (x=0; x < BITMAP_WIDTH(bm); x++){ 
                      p = pixel (x,y,&bm); 
                      if (p != BACKGROUND) { 
                              /* we have a new colour in the image */ 
                              LOG1 ("Thinning colour %x\n",p); 
                              clr_colour (p,&bm); 
                              thin (image,p); 
                      } 
              } 
      } 
 
      free (bm.bitmap); 
} 

 
void thin (bitmap_type *image, unsigned int colour) 
{ 
      unsigned int    xsize, ysize;   /* Image resolution             */ 
      unsigned int    x, y;           /* Pixel location               */ 
      unsigned int    i;              /* Pass index           */ 
      unsigned int    pc      = 0;    /* Pass count           */ 
      unsigned int    count   = 1;    /* Deleted pixel count          */ 
      unsigned int    p, q;           /* Neighborhood maps of adjacent*/ 
                                      /* cells                        */ 
      Pixel           *qb;            /* Neighborhood maps of previous*/ 
                                      /* scanline                     */ 
      unsigned int    m;              /* Deletion direction mask      */ 
 
      LOG (" Thinning image.....\n "); 
      xsize = BITMAP_WIDTH(*image); 
      ysize = BITMAP_HEIGHT(*image); 
      qb    = malloc (xsize*sizeof(Pixel)); 
      qb[xsize-1] = 0;                /* Used for lower-right pixel   */ 
 
      while ( count ) {               /* Scan image while deletions   */ 
          pc++; 
          count = 0; 
 
          for ( i = 0 ; i < 4 ; i++ ) { 
 
              m = masks[i]; 
 
              /* Build initial previous scan buffer.                  */ 
              p = (pixel (0,0,image) == colour); 
              for ( x = 0 ; x < xsize-1 ; x++ ) 
                  qb[x] = p = ((p<<1)&0006) | (pixel(x+1,0,image) == colour); 
 
              /* Scan image for pixel deletion candidates.            */ 
 
              for ( y = 0 ; y < ysize-1 ; y++ ) { 
 
                  q = qb[0]; 
                  p = ((q<<3)&0110) | (pixel(0,y+1,image) == colour); 
 
                  for ( x = 0 ; x < xsize-1 ; x++ ) { 
                      q = qb[x]; 
                      p = ((p<<1)&0666) | ((q<<3)&0110) | 
                              (pixel(x+1,y+1,image) == colour); 
                      qb[x] = p; 
                      if  ( ((p&m) == 0) && delete[p] ) { 
                          count++; 
                          clr_pixel(x,y,image); 
                          /* delete the pixel */ 
                      } 
                  } 
 
                  /* Process right edge pixel.                        */ 
 
                  p = (p<<1)&0666; 
                  if  ( (p&m) == 0 && delete[p] ) { 
                      count++; 
                      clr_pixel(xsize-1,y,image); 
                  } 
              } 
 
              /* Process bottom scan line.                            */ 
 
              for ( x = 0 ; x < xsize ; x++ ) { 
                  q = qb[x]; 
                  p = ((p<<1)&0666) | ((q<<3)&0110); 
                  if  ( (p&m) == 0 && delete[p] ) { 
                      count++; 
                      clr_pixel(x,ysize-1,image); 
                  } 
              } 
          } 
 
          LOG2 ("ThinImage: pass %d, %d pixels deleted\n", pc, count); 
      } 
 
      free (qb); 
} 
