#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <magick/api.h>
#include "input-magick.h"
#include "bitmap.h"

bitmap_type magick_load_image(string filename)
{
  Image *image;
  ImageInfo *image_info;
  ImageType image_type;
  unsigned int i,j,point,np,runcount;
  bitmap_type bitmap;
#if MagickLibVersion < 0x500
  RunlengthPacket* pixel;
#else
  PixelPacket p;
  PixelPacket *pixel=&p;
  ExceptionInfo exception;
#endif
#if MagickLibVersion >= 0x521
  MagickIncarnate("");
#endif
#if MagickLibVersion > 0x500
  GetExceptionInfo(&exception);
#endif
  image_info=CloneImageInfo((ImageInfo *) NULL);
  (void) strcpy(image_info->filename,filename);
#if MagickLibVersion < 0x500
  image=ReadImage(image_info);
#else
  image=ReadImage(image_info,&exception);
  if (image == (Image *) NULL)
    MagickError(exception.severity,exception.message,exception.qualifier);
#endif
  
  image_type=GetImageType(image);
  if(image_type == BilevelType || image_type == GrayscaleType)
    np=1;
  else
    np=3;

  bitmap.np=np;
  bitmap.dimensions.width=image->columns;
  bitmap.dimensions.height=image->rows;
  bitmap.bitmap=(unsigned char*)malloc(np*image->columns*image->rows);

#if MagickLibVersion < 0x500
  pixel=image->pixels;
#endif

  for(j=0,runcount=0,point=0;j<image->rows;j++)
    for(i=0;i<image->columns;i++) {
#if MagickLibVersion < 0x500
      if(runcount)
        runcount--;
      else {
        runcount=pixel->length;
        pixel++;
      }
#else
      p=GetOnePixel(image,i,j);
#endif

      bitmap.bitmap[point++]=pixel->red; /* if gray: red=green=blue */
      if(np==3) {
        bitmap.bitmap[point++]=pixel->green;
        bitmap.bitmap[point++]=pixel->blue;
      }
    }
  return(bitmap);
}
