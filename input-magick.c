/* input-magick.c: import files via image magick */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <magick/magick.h>
#include "input-magick.h"
#include "bitmap.h"

bitmap_type magick_load_image(string filename)
{
  Image *image;
  ImageInfo *image_info;
  ImageType image_type;
  unsigned int i,j,point,np,runcount;
  bitmap_type bitmap;
  PixelPacket p;
  PixelPacket *pixel=&p;
  ExceptionInfo exception;
  MagickIncarnate("");
  GetExceptionInfo(&exception);
  image_info=CloneImageInfo((ImageInfo *) NULL);
  (void) strcpy(image_info->filename,filename);

  image=ReadImage(image_info,&exception);
  if (image == (Image *) NULL)
#if (MagickLibVersion <= 0x525)
    MagickError(exception.severity,exception.message,exception.qualifier);
#else
    MagickError(exception.severity,exception.reason,exception.description);
#endif
  image_type=GetImageType(image);
  if(image_type == BilevelType || image_type == GrayscaleType)
    np=1;
  else
    np=3;

  bitmap.np=np;
  BITMAP_WIDTH (bitmap)=image->columns;
  BITMAP_HEIGHT (bitmap)=image->rows;
  BITMAP_BITS (bitmap)=(unsigned char*)malloc(np*image->columns*image->rows);

  for(j=0,runcount=0,point=0;j<image->rows;j++)
    for(i=0;i<image->columns;i++) {
      p=GetOnePixel(image,i,j);
      bitmap.bitmap[point++]=pixel->red; /* if gray: red=green=blue */
      if(np==3) {
        bitmap.bitmap[point++]=pixel->green;
        bitmap.bitmap[point++]=pixel->blue;
      }
    }
  return(bitmap);
}
