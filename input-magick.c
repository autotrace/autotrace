/* input-magick.c: import files via image magick */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <magick/magick.h>
#include "input-magick.h"
#include "bitmap.h"
#if MagickLibVersion < 0x500
#include "xstd.h"
#endif

bitmap_type magick_load_image(string filename)
{
  Image *image;
  ImageInfo *image_info;
#if MagickLibVersion >= 0x521
  ImageType image_type;
#endif
  unsigned int i,j,point,np,runcount;
  bitmap_type bitmap;
#if MagickLibVersion < 0x500
  RunlengthPacket* pixel;
#else
  PixelPacket p;
  PixelPacket *pixel=&p;
#endif
#if MagickLibVersion >= 0x521
  ExceptionInfo exception;
  MagickIncarnate("");
  GetExceptionInfo(&exception);
  image_info=CloneImageInfo((ImageInfo *) NULL);
#else
  XMALLOC(image_info, sizeof(&image_info));
#endif
  (void) strcpy(image_info->filename,filename);

#if MagickLibVersion < 0x521
  image=ReadImage(image_info);
#else
  image=ReadImage(image_info,&exception);
  if (image == (Image *) NULL)
#endif
#if (MagickLibVersion >= 0x521) && (MagickLibVersion <= 0x525)
    MagickError(exception.severity,exception.message,exception.qualifier);
#endif
#if MagickLibVersion >= 0x526
    MagickError(exception.severity,exception.reason,exception.description);
#endif
#if MagickLibVersion < 0x521
  if(IsGrayImage (image) || IsMonochromeImage (image))
#else
  image_type=GetImageType(image);
  if(image_type == BilevelType || image_type == GrayscaleType)
#endif
    np=1;
  else
    np=3;

  bitmap.np=np;
  BITMAP_WIDTH (bitmap)=image->columns;
  BITMAP_HEIGHT (bitmap)=image->rows;
  BITMAP_BITS (bitmap)=(unsigned char*)malloc(np*image->columns*image->rows);

#if MagickLibVersion < 0x521
  pixel=image->pixels;
#endif

  for(j=0,runcount=0,point=0;j<image->rows;j++)
    for(i=0;i<image->columns;i++) {
#if MagickLibVersion < 0x521
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
