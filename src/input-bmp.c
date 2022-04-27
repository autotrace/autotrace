/* input-bmp.c:	reads any bitmap I could get for testing

   Copyright (C) 1999, 2000, 2001 Martin Weber.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public License
   as published by the Free Software Foundation; either version 2.1 of
   the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with this library; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
   USA. */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* Def: HAVE_CONFIG_H */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "types.h"
#include "bitmap.h"
#include "logreport.h"
#include "xstd.h"
#include "input-bmp.h"


#define BI_RGB            0
#define BI_RLE8           1
#define BI_RLE4           2
#define BI_BITFIELDS      3
#define BI_ALPHABITFIELDS 4

#define BitSet(byte, bit)  (((byte) & (bit)) == (bit))

#define ReadOK(file,buffer,len)  (fread(buffer, len, 1, file) != 0)

struct Bitmap_File_Head_Struct {
  char zzMagic[2];              /* 00 "BM" */
  unsigned long bfSize;         /* 02 */
  unsigned short zzHotX;        /* 06 */
  unsigned short zzHotY;        /* 08 */
  unsigned long bfOffs;         /* 0A */
  unsigned long biSize;         /* 0E */
} Bitmap_File_Head;

struct Bitmap_Head_Struct {
  unsigned long biWidth;        /* 12 */
  unsigned long biHeight;       /* 16 */
  unsigned short biPlanes;      /* 1A */
  unsigned short biBitCnt;      /* 1C */
  unsigned long biCompr;        /* 1E */
  unsigned long biSizeIm;       /* 22 */
  unsigned long biXPels;        /* 26 */
  unsigned long biYPels;        /* 2A */
  unsigned long biClrUsed;      /* 2E */
  unsigned long biClrImp;       /* 32 */
  unsigned long masks[4];       /* 36 */
  /* 3A */
} Bitmap_Head;
 
typedef struct
{
	unsigned long mask;
	unsigned long shiftin;
	float  max_value;
} Bitmap_Channel;

static void
setMasksDefault (unsigned short        biBitCnt,
                 Bitmap_Channel *masks)
{
  switch (biBitCnt)
    {
    case 32:
      masks[0].mask      = 0x00ff0000;
      masks[0].shiftin   = 16;
      masks[0].max_value = (float)255.0;
      masks[1].mask      = 0x0000ff00;
      masks[1].shiftin   = 8;
      masks[1].max_value = (float)255.0;
      masks[2].mask      = 0x000000ff;
      masks[2].shiftin   = 0;
      masks[2].max_value = (float)255.0;
      masks[3].mask      = 0x00000000;
      masks[3].shiftin   = 0;
      masks[3].max_value = (float)0.0;
      break;

    case 24:
      masks[0].mask      = 0xff0000;
      masks[0].shiftin   = 16;
      masks[0].max_value = (float)255.0;
      masks[1].mask      = 0x00ff00;
      masks[1].shiftin   = 8;
      masks[1].max_value = (float)255.0;
      masks[2].mask      = 0x0000ff;
      masks[2].shiftin   = 0;
      masks[2].max_value = (float)255.0;
      masks[3].mask      = 0x0;
      masks[3].shiftin   = 0;
      masks[3].max_value = (float)0.0;
      break;

    case 16:
      masks[0].mask      = 0x7c00;
      masks[0].shiftin   = 10;
      masks[0].max_value = (float)31.0;
      masks[1].mask      = 0x03e0;
      masks[1].shiftin   = 5;
      masks[1].max_value = (float)31.0;
      masks[2].mask      = 0x001f;
      masks[2].shiftin   = 0;
      masks[2].max_value = (float)31.0;
      masks[3].mask      = 0x0;
      masks[3].shiftin   = 0;
      masks[3].max_value = (float)0.0;
      break;

    default:
      break;
    }
}

static long ToL(unsigned char *);
static short ToS(unsigned char *);
static int ReadColorMap(FILE *, unsigned char[256][3], int, int, gboolean *, at_exception_type *);
static gboolean ReadChannelMasks(unsigned int *, Bitmap_Channel *, unsigned int);
static unsigned char *ReadImage(FILE *, int, int, unsigned char[256][3], int, int, int, int, gboolean, const Bitmap_Channel *, at_exception_type *);

at_bitmap input_bmp_reader(gchar * filename, at_input_opts_type * opts, at_msg_func msg_func, gpointer msg_data, gpointer user_data)
{
  FILE *fd;
  unsigned char buffer[128];
  int ColormapSize, rowbytes, Maps;
  gboolean Grey = FALSE;
  unsigned char ColorMap[256][3];
  at_bitmap image = at_bitmap_init(0, 0, 0, 1);
  unsigned char *image_storage;
  at_exception_type exp = at_exception_new(msg_func, msg_data);
  char magick[2];
  Bitmap_Channel masks[4];

  fd = fopen(filename, "rb");

  if (!fd) {
    LOG("Can't open \"%s\"\n", filename);
    at_exception_fatal(&exp, "bmp: cannot open input file");
    goto cleanup;
  }

  /* It is a File. Now is it a Bitmap? Read the shortest possible header. */

  if (!ReadOK(fd, magick, 2) ||
	  !(!strncmp(magick, "BA", 2) ||
		  !strncmp(magick, "BM", 2) ||
		  !strncmp(magick, "IC", 2) ||
		  !strncmp(magick, "PT", 2) ||
		  !strncmp(magick, "CI", 2) ||
		  !strncmp(magick, "CP", 2)))
  {
	  LOG("%s is not a valid BMP file", filename);
	  at_exception_fatal(&exp, "bmp: invalid input file");
	  goto cleanup;
  }

  while (!strncmp(magick, "BA", 2))
  {
	  if (!ReadOK(fd, buffer, 12))
	  {
		  LOG("%s is not a valid BMP file", filename);
		  at_exception_fatal(&exp, "bmp: invalid input file");
		  goto cleanup;
	  }

	  if (!ReadOK(fd, magick, 2))
	  {
		  LOG("%s is not a valid BMP file", filename);
		  at_exception_fatal(&exp, "bmp: invalid input file");
		  goto cleanup;
	  }
  }

  if (!ReadOK(fd, buffer, 12))////
  {
	  LOG("%s is not a valid BMP file", filename);
	  at_exception_fatal(&exp, "bmp: invalid input file");
	  goto cleanup;
  }

  /* bring them to the right byteorder. Not too nice, but it should work */

  Bitmap_File_Head.bfSize = ToL(&buffer[0x00]);
  Bitmap_File_Head.zzHotX = ToS(&buffer[0x04]);
  Bitmap_File_Head.zzHotY = ToS(&buffer[0x06]);
  Bitmap_File_Head.bfOffs = ToL(&buffer[0x08]);

  if (!ReadOK(fd, buffer, 4))
  {
	  LOG("%s is not a valid BMP file", filename);
	  at_exception_fatal(&exp, "bmp: invalid input file");
	  goto cleanup;
  }

  Bitmap_File_Head.biSize = ToL(&buffer[0x00]);

  /* What kind of bitmap is it? */

  if (Bitmap_File_Head.biSize == 12) {  /* OS/2 1.x ? */
    if (!ReadOK(fd, buffer, 8)) {
      LOG("Error reading BMP file header\n");
      at_exception_fatal(&exp, "Error reading BMP file header");
      goto cleanup;
    }

    Bitmap_Head.biWidth = ToS(&buffer[0x00]); /* 12 */
    Bitmap_Head.biHeight = ToS(&buffer[0x02]);  /* 14 */
    Bitmap_Head.biPlanes = ToS(&buffer[0x04]);  /* 16 */
    Bitmap_Head.biBitCnt = ToS(&buffer[0x06]);  /* 18 */
    Bitmap_Head.biCompr = 0;
    Bitmap_Head.biSizeIm = 0;
    Bitmap_Head.biXPels = Bitmap_Head.biYPels = 0;
    Bitmap_Head.biClrUsed = 0;
    Bitmap_Head.biClrImp = 0;
    Bitmap_Head.masks[0] = 0;
    Bitmap_Head.masks[1] = 0;
    Bitmap_Head.masks[2] = 0;
    Bitmap_Head.masks[3] = 0;

    memset(masks, 0, sizeof(masks));
    Maps = 3;

  } else if (Bitmap_File_Head.biSize == 40) { /* Windows 3.x */
    if (!ReadOK(fd, buffer, 36))
    {
      LOG ("Error reading BMP file header\n");
      at_exception_fatal(&exp, "Error reading BMP file header");
      goto cleanup;
    }
          

    Bitmap_Head.biWidth = ToL(&buffer[0x00]); /* 12 */
    Bitmap_Head.biHeight = ToL(&buffer[0x04]);  /* 16 */
    Bitmap_Head.biPlanes = ToS(&buffer[0x08]);  /* 1A */
    Bitmap_Head.biBitCnt = ToS(&buffer[0x0A]);  /* 1C */
    Bitmap_Head.biCompr = ToL(&buffer[0x0C]); /* 1E */
    Bitmap_Head.biSizeIm = ToL(&buffer[0x10]);  /* 22 */
    Bitmap_Head.biXPels = ToL(&buffer[0x14]); /* 26 */
    Bitmap_Head.biYPels = ToL(&buffer[0x18]); /* 2A */
    Bitmap_Head.biClrUsed = ToL(&buffer[0x1C]); /* 2E */
    Bitmap_Head.biClrImp = ToL(&buffer[0x20]);  /* 32 */
    Bitmap_Head.masks[0] = 0;
    Bitmap_Head.masks[1] = 0;
    Bitmap_Head.masks[2] = 0;
    Bitmap_Head.masks[3] = 0;

    Maps = 4;
    memset(masks, 0, sizeof(masks));

    if (Bitmap_Head.biCompr == BI_BITFIELDS)
      {
	if (!ReadOK(fd, buffer, 3 * sizeof(unsigned long)))
	  {
	    LOG("Error reading BMP file header\n");
	    at_exception_fatal(&exp, "Error reading BMP file header");
	    goto cleanup;
	  }

	Bitmap_Head.masks[0] = ToL(&buffer[0x00]);
	Bitmap_Head.masks[1] = ToL(&buffer[0x04]);
	Bitmap_Head.masks[2] = ToL(&buffer[0x08]);

	ReadChannelMasks(&Bitmap_Head.masks[0], masks, 3);
      }
    else if (Bitmap_Head.biCompr == BI_RGB)
      {
	setMasksDefault(Bitmap_Head.biBitCnt, masks);
      }
    else if ((Bitmap_Head.biCompr != BI_RLE4) &&
	     (Bitmap_Head.biCompr != BI_RLE8))
      {
	/* BI_ALPHABITFIELDS, etc. */
	LOG("Unsupported compression in BMP file\n");
	at_exception_fatal(&exp, "Unsupported compression in BMP file");
	goto cleanup;
      }
  }
  else if (Bitmap_File_Head.biSize >= 56 &&
	   Bitmap_File_Head.biSize <= 64)
  {
    /* enhanced Windows format with bit masks */

    if (!ReadOK (fd, buffer, Bitmap_File_Head.biSize - 4))
    {

      LOG("Error reading BMP file header\n");
      at_exception_fatal(&exp, "Error reading BMP file header");
      goto cleanup;
    }

    Bitmap_Head.biWidth = ToL(&buffer[0x00]); /* 12 */
    Bitmap_Head.biHeight = ToL(&buffer[0x04]);  /* 16 */
    Bitmap_Head.biPlanes = ToS(&buffer[0x08]);  /* 1A */
    Bitmap_Head.biBitCnt = ToS(&buffer[0x0A]);  /* 1C */
    Bitmap_Head.biCompr = ToL(&buffer[0x0C]); /* 1E */
    Bitmap_Head.biSizeIm = ToL(&buffer[0x10]);  /* 22 */
    Bitmap_Head.biXPels = ToL(&buffer[0x14]); /* 26 */
    Bitmap_Head.biYPels = ToL(&buffer[0x18]); /* 2A */
    Bitmap_Head.biClrUsed = ToL(&buffer[0x1C]); /* 2E */
    Bitmap_Head.biClrImp = ToL(&buffer[0x20]);  /* 32 */
    Bitmap_Head.masks[0] = ToL(&buffer[0x24]);       /* 36 */
    Bitmap_Head.masks[1] = ToL(&buffer[0x28]);       /* 3A */
    Bitmap_Head.masks[2] = ToL(&buffer[0x2C]);       /* 3E */
    Bitmap_Head.masks[3] = ToL(&buffer[0x30]);       /* 42 */

    Maps = 4;
    ReadChannelMasks(&Bitmap_Head.masks[0], masks, 4);
  }
  else if (Bitmap_File_Head.biSize == 108 ||
           Bitmap_File_Head.biSize == 124)
  {
    /* BMP Version 4 or 5 */

    if (!ReadOK(fd, buffer, Bitmap_File_Head.biSize - 4))
    {
	    LOG("Error reading BMP file header\n");
	    at_exception_fatal(&exp, "Error reading BMP file header");
	    goto cleanup;
    }

    Bitmap_Head.biWidth = ToL(&buffer[0x00]);
    Bitmap_Head.biHeight = ToL(&buffer[0x04]);
    Bitmap_Head.biPlanes = ToS(&buffer[0x08]);
    Bitmap_Head.biBitCnt = ToS(&buffer[0x0A]);
    Bitmap_Head.biCompr = ToL(&buffer[0x0C]);
    Bitmap_Head.biSizeIm = ToL(&buffer[0x10]);
    Bitmap_Head.biXPels = ToL(&buffer[0x14]);
    Bitmap_Head.biYPels = ToL(&buffer[0x18]);
    Bitmap_Head.biClrUsed = ToL(&buffer[0x1C]);
    Bitmap_Head.biClrImp = ToL(&buffer[0x20]);
    Bitmap_Head.masks[0] = ToL(&buffer[0x24]);
    Bitmap_Head.masks[1] = ToL(&buffer[0x28]);
    Bitmap_Head.masks[2] = ToL(&buffer[0x2C]);
    Bitmap_Head.masks[3] = ToL(&buffer[0x30]);

    Maps = 4;

    if (Bitmap_Head.biCompr == BI_BITFIELDS)
    {
	    ReadChannelMasks(&Bitmap_Head.masks[0], masks, 4);
    }
    else if (Bitmap_Head.biCompr == BI_RGB)
    {
	    setMasksDefault(Bitmap_Head.biBitCnt, masks);
    }
  } else {
    LOG("Error reading BMP file header\n");
    at_exception_fatal(&exp, "Error reading BMP file header");
    goto cleanup;
  }

  /* Valid options 1, 4, 8, 16, 24, 32 */
  /* 16 is awful, we should probably shoot whoever invented it */

  switch (Bitmap_Head.biBitCnt)
  {
  case 1:
  case 2:
  case 4:
  case 8:
  case 16:
  case 24:
  case 32:
	  break;
  default:
	  LOG("%s is not a valid BMP file", filename);
	  at_exception_fatal(&exp, "bmp: invalid input file");
	  goto cleanup;
  }

  /* There should be some colors used! */

  ColormapSize = (Bitmap_File_Head.bfOffs - Bitmap_File_Head.biSize - 14) / Maps;

  if ((Bitmap_Head.biClrUsed == 0) &&
      (Bitmap_Head.biBitCnt <= 8))
  {
	  ColormapSize = Bitmap_Head.biClrUsed = 1 << Bitmap_Head.biBitCnt;
  }

  if (ColormapSize > 256)
    ColormapSize = 256;

  /* Sanity checks */

  if (Bitmap_Head.biHeight == 0 ||
	  Bitmap_Head.biWidth == 0)
  {
	  LOG("%s is not a valid BMP file", filename);
	  at_exception_fatal(&exp, "bmp: invalid input file");
	  goto cleanup;
  }

  /* biHeight may be negative, but -2147483648 is dangerous because:
	 -2147483648 == -(-2147483648) */
  if (Bitmap_Head.biWidth < 0 ||
	  Bitmap_Head.biHeight == -2147483648)
  {
	  LOG("%s is not a valid BMP file", filename);
	  at_exception_fatal(&exp, "bmp: invalid input file");
	  goto cleanup;
  }

  if (Bitmap_Head.biPlanes != 1)
  {
	  LOG("%s is not a valid BMP file", filename);
	  at_exception_fatal(&exp, "bmp: invalid input file");
	  goto cleanup;
  }

  if (Bitmap_Head.biClrUsed > 256 &&
	  Bitmap_Head.biBitCnt <= 8)
  {
	  LOG("%s is not a valid BMP file", filename);
	  at_exception_fatal(&exp, "bmp: invalid input file");
	  goto cleanup;
  }

  /* protect against integer overflows caused by malicious BMPs */
  /* use divisions in comparisons to avoid type overflows */

  if (((unsigned long)Bitmap_Head.biWidth) > (unsigned int)0x7fffffff / Bitmap_Head.biBitCnt ||
	  ((unsigned long)Bitmap_Head.biWidth) > ((unsigned int)0x7fffffff /abs(Bitmap_Head.biHeight)) / 4)
  {
	  LOG("%s is not a valid BMP file", filename);
	  at_exception_fatal(&exp, "bmp: invalid input file");
	  goto cleanup;
  }

  /* Windows and OS/2 declare filler so that rows are a multiple of
   * word length (32 bits == 4 bytes)
   */
   
  unsigned long overflowTest = Bitmap_Head.biWidth * Bitmap_Head.biBitCnt;
  if (overflowTest / Bitmap_Head.biWidth != Bitmap_Head.biBitCnt) {
    LOG("Error reading BMP file header. Width is too large\n");
    at_exception_fatal(&exp, "Error reading BMP file header. Width is too large");
    goto cleanup;
  }

  rowbytes = ((Bitmap_Head.biWidth * Bitmap_Head.biBitCnt - 1) / 32) * 4 + 4;

#ifdef DEBUG
  printf("\nSize: %u, Colors: %u, Bits: %u, Width: %u, Height: %u, Comp: %u, Zeile: %u\n", Bitmap_File_Head.bfSize, Bitmap_Head.biClrUsed, Bitmap_Head.biBitCnt, Bitmap_Head.biWidth, Bitmap_Head.biHeight, Bitmap_Head.biCompr, rowbytes);
#endif


  if (Bitmap_Head.biBitCnt <= 8)
  {
#ifdef DEBUG
    printf("Colormap read\n");
#endif
	  /* Get the Colormap */
	  if (!ReadColorMap(fd, ColorMap, ColormapSize, Maps, &Grey, &exp))
		  goto cleanup;
  }

  fseek(fd, Bitmap_File_Head.bfOffs, SEEK_SET);

  /* Get the Image and return the ID or -1 on error */
  image_storage = ReadImage(fd, 
	Bitmap_Head.biWidth, Bitmap_Head.biHeight,
	ColorMap,
        Bitmap_Head.biClrUsed,
	Bitmap_Head.biBitCnt, Bitmap_Head.biCompr, rowbytes,
        Grey,
	masks,
	&exp);

  image = at_bitmap_init(image_storage, (unsigned short)Bitmap_Head.biWidth, (unsigned short)Bitmap_Head.biHeight, Grey ? 1 : 3);
cleanup:
  fclose(fd);
  return (image);
}

static gboolean ReadColorMap(FILE * fd, unsigned char buffer[256][3], int number, int size, 
	gboolean *Grey,
	at_exception_type *exp)
{
  int i;
  unsigned char rgb[4];

  *Grey = (number > 2);
  for (i = 0; i < number; i++) {
    if (!ReadOK(fd, rgb, size)) {
      LOG ("Bad colormap\n");
      at_exception_fatal (exp, "Bad colormap");
      return FALSE;
    }

    /* Bitmap save the colors in another order! But change only once! */

    buffer[i][0] = rgb[2];
    buffer[i][1] = rgb[1];
    buffer[i][2] = rgb[0];
    *Grey = ((*Grey) && (rgb[0] == rgb[1]) && (rgb[1] == rgb[2]));
  }
cleanup:
  return TRUE;
}

static gboolean
ReadChannelMasks(unsigned int       *tmp,
	Bitmap_Channel *masks,
	unsigned int          channels)
{
	unsigned int i;

	for (i = 0; i < channels; i++)
	{
		unsigned int mask;
		int    nbits, offset, bit;

		mask = tmp[i];
		masks[i].mask = mask;
		nbits = 0;
		offset = -1;

		for (bit = 0; bit < 32; bit++)
		{
			if (mask & 1)
			{
				nbits++;
				if (offset == -1)
					offset = bit;
			}

			mask = mask >> 1;
		}

		masks[i].shiftin = offset;
		masks[i].max_value = (float)((1 << nbits) - 1);

#ifdef _DEBUG
		LOG4("Channel %d mask %08x in %d max_val %d\n",
			i, masks[i].mask, masks[i].shiftin, (int)masks[i].max_value);
#endif
	}

	return TRUE;
}

/*static gint32
ReadImage(FILE                 *fd,
	const gchar          *filename,
	gint                  width,
	gint                  height,
	guchar                cmap[256][3],
	gint                  ncols,
	gint                  bpp,
	gint                  compression,
	gint                  rowbytes,
	gboolean              gray,
	const BitmapChannel  *masks,
	GError              **error)*/


static unsigned char *ReadImage(FILE * fd, int width, int height,
	unsigned char cmap[256][3],
	int ncols,
	int bpp, int compression, int rowbytes,
	gboolean    Grey,
	const Bitmap_Channel * masks,
	at_exception_type * exp)
{
  unsigned char v, n;
  int xpos = 0;
  int ypos = 0;
  unsigned char * image;
  unsigned char *dest, *temp, *row_buf;
  long rowstride, channels;
  unsigned short rgb;
  int i, i_max, j;
  int total_bytes_read;
  unsigned int px32;

  if (!(compression == BI_RGB ||
	  (bpp == 8 && compression == BI_RLE8) ||
	  (bpp == 4 && compression == BI_RLE4) ||
	  (bpp == 16 && compression == BI_BITFIELDS) ||
	  (bpp == 32 && compression == BI_BITFIELDS)))
  {
	  LOG("Unrecognized or invalid BMP compression format.\n");
	  at_exception_fatal(exp, "Unrecognized or invalid BMP compression format.");
	  return NULL;
  }

  if (bpp >= 16) {              /* color image */
    XMALLOC(image, width * height * 3 * sizeof(unsigned char));
    if (masks[3].mask != 0)
    {
      channels = 4;
    }
    else
    {
      channels = 3;
    }
  }
  else if (Grey) /* Grey image */
  {
    XMALLOC(image, width * height * 1 * sizeof(unsigned char));
    channels = 1;
  } else {                      /* indexed image */

    XMALLOC(image, width * height * 1 * sizeof(unsigned char));
    channels = 1;
  }

  /* use XCALLOC to initialize the dest row_buf so that unspecified
	 pixels in RLE bitmaps show up as the zeroth element in the palette.
  */
  XCALLOC(dest, width * height * channels);
  XMALLOC (row_buf, rowbytes); 
  rowstride = width * channels;

  ypos = height - 1;            /* Bitmaps begin in the lower left corner */

  switch (bpp) {

  case 32:
    {
      while (ReadOK (fd, row_buf, rowbytes))
      {
        temp = image + (ypos * rowstride);
        for (xpos = 0; xpos < width; ++xpos) {
			  px32 = ToL(&row_buf[xpos * 4]);
			  unsigned char red = *(temp++) = ((px32 & masks[0].mask) >> masks[0].shiftin) * 255.0 / masks[0].max_value + 0.5;
			  unsigned char green = *(temp++) = ((px32 & masks[1].mask) >> masks[1].shiftin) * 255.0 / masks[1].max_value + 0.5;
			  unsigned char blue = *(temp++) = ((px32 & masks[2].mask) >> masks[2].shiftin) * 255.0 / masks[2].max_value + 0.5;
			  /* currently alpha channels are not supported by AutoTrace, thus simply ignored */
			  /*if (channels > 3)
				  *(temp++) = ((px32 & masks[3].mask) >> masks[3].shiftin) * 255.0 / masks[3].max_value + 0.5;*/
		  }

		  if (ypos == 0)
			  break;

		  --ypos; /* next line */
      }
    }
    break;

  case 24:
    {
      while (ReadOK (fd, row_buf, rowbytes))
      {
        temp = image + (ypos * rowstride);
        for (xpos = 0; xpos < width; ++xpos) {
          *(temp++) = row_buf[xpos * 3 + 2];
          *(temp++) = row_buf[xpos * 3 + 1];
          *(temp++) = row_buf[xpos * 3];
        }
        --ypos;                 /* next line */
      }
    }
    break;

  case 16:
    {
      while (ReadOK (fd, row_buf, rowbytes))
      {
        temp = image + (ypos * rowstride);
        for (xpos = 0; xpos < width; ++xpos)
        {
			  rgb = ToS(&row_buf[xpos * 2]);
			  *(temp++) = ((rgb & masks[0].mask) >> masks[0].shiftin) * 255.0 / masks[0].max_value + 0.5;
			  *(temp++) = ((rgb & masks[1].mask) >> masks[1].shiftin) * 255.0 / masks[1].max_value + 0.5;
			  *(temp++) = ((rgb & masks[2].mask) >> masks[2].shiftin) * 255.0 / masks[2].max_value + 0.5;
			  /* currently alpha channels are not supported by AutoTrace, thus simply ignored */
			  /*if (channels > 3)
				  *(temp++) = ((rgb & masks[3].mask) >> masks[3].shiftin) * 255.0 / masks[3].max_value + 0.5;*/
		  }

		  if (ypos == 0)
			  break;

		  --ypos; /* next line */
      }
    }
    break;

  case 8:
  case 4:
  case 1:
    {
      if (compression == 0) {
        while (ReadOK(fd, &v, 1)) {
          for (i = 1; (i <= (8 / bpp)) && (xpos < width); i++, xpos++) {
			temp = image + (ypos * rowstride) + (xpos * channels);
			*temp = (v & (((1 << bpp) - 1) << (8 - (i*bpp)))) >> (8 - (i*bpp));
			if (Grey)
				*temp = cmap[*temp][0];
	  }

          if (xpos == width) {
	    ReadOK (fd, row_buf, rowbytes - 1 - (width * bpp - 1) / 8);
            ypos--;
            xpos = 0;

          }
          if (ypos < 0)
            break;
        }
        break;
      } else {
	/* compressed image (either RLE8 or RLE4) */
        while (ypos >= 0 && xpos <= width) {
			if (!ReadOK(fd, row_buf, 2))
			{
				LOG("The bitmap ends unexpectedly.");
				break;
			}

			if ((unsigned char) row_buf[0] != 0)
		      /* Count + Color - record */
		      {
				/* encoded mode run -
				 *   row_buf[0] == run_length
				 *   row_buf[1] == pixel data
				 */
		        for (j = 0; ((unsigned char) j < (unsigned char) row_buf[0]) && (xpos < width);)
		          {
#ifdef DEBUG2
              printf("%u %u | ", xpos, width);
#endif
			        for (i = 1;
			             ((i <= (8 / bpp)) &&
			             (xpos < width) &&
			             ((unsigned char) j < (unsigned char) row_buf[0]));
			             i++, xpos++, j++)
			          {
			            temp = dest + (ypos * rowstride) + (xpos * channels);
			            *temp = (unsigned char) ((row_buf[1] & (((1<<bpp)-1) << (8 - (i * bpp)))) >> (8 - (i * bpp)));
				        if (Grey)
					      *temp = cmap[*temp][0];
			          }
            }
          }
          if ((row_buf[0] == 0) && (row_buf[1] > 2))
            /* uncompressed record */
		{
			n = row_buf[1];
			total_bytes_read = 0;

			for (j = 0; j < n; j += (8 / bpp))
			{
				/* read the next byte in the record */
				if (!ReadOK(fd, &v, 1))
				{
					LOG("The bitmap ends unexpectedly.");
					break;
				} 

				total_bytes_read++;

				/* read all pixels from that byte */
				i_max = 8 / bpp;
				if (n - j < i_max)
				{
					i_max = n - j;
				}

				i = 1;
				while ((i <= i_max) && (xpos < width))
				{
					temp =
						dest + (ypos * rowstride) + (xpos * channels);
					*temp = (v >> (8 - (i*bpp))) & ((1 << bpp) - 1);
					if (Grey)
						*temp = cmap[*temp][0];
					i++;
					xpos++;
				}
			}

			/* absolute mode runs are padded to 16-bit alignment */
			if (total_bytes_read % 2)
				fread(&v, 1, 1, fd); //ReadOk
		}
	  if (((unsigned char) row_buf[0] == 0) && ((unsigned char) row_buf[1]==0))
            /* Line end */
          {
            ypos--;
            xpos = 0;
          }
          if (((unsigned char)row_buf[0] == 0) && ((unsigned char)row_buf[1] == 1))
            /* Bitmap end */
          {
            break;
          }
          if (((unsigned char)row_buf[0] == 0) && ((unsigned char)row_buf[1] == 2))
            /* Deltarecord */
          {
			if (!ReadOK(fd, row_buf, 2))
			  {
				LOG("The bitmap ends unexpectedly.");
				break;
			  }
			xpos += (unsigned char) row_buf[0];
		    ypos -= (unsigned char) row_buf[1];
          }
        }
        break;
      }
    }
    break;
  default:
    /* This is very bad, we should not be here */
    break;
  }

  if (bpp <= 8) {
    unsigned char *temp2, *temp3;
    unsigned char index;
    temp2 = temp = image;
    XMALLOC (image, width * height * 3 * sizeof (unsigned char)); //???
    temp3 = image;
    for (ypos = 0; ypos < height; ypos++) {
      for (xpos = 0; xpos < width; xpos++) {
        index = *temp2++;
        *temp3++ = cmap[index][0];
        if (!Grey) {
          *temp3++ = cmap[index][1];
          *temp3++ = cmap[index][2];
        }
      }
    }
    free(temp);
  }

  free (row_buf);
  free(dest);
  return image;
}

static long ToL(unsigned char *puffer)
{
  return (puffer[0] | puffer[1] << 8 | puffer[2] << 16 | puffer[3] << 24);
}

static short ToS(unsigned char *puffer)
{
  return ((short)(puffer[0] | puffer[1] << 8));
}
