/* input-bmp.c	reads any bitmap I could get for testing */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bitmap.h"
#include "message.h"
#include "xmem.h"
#include "input-bmp.h"


#define MAXCOLORS       256
#define Image		long

#define BitSet(byte, bit)  (((byte) & (bit)) == (bit))

#define ReadOK(file,buffer,len)  (fread(buffer, len, 1, file) != 0)
#define Write(file,buffer,len)   fwrite(buffer, len, 1, file)
#define WriteOK(file,buffer,len) (Write(buffer, len, file) != 0)

struct Bitmap_File_Head_Struct
{
  unsigned long   bfSize;      /* 02 */
  unsigned long   reserverd;   /* 06 */
  unsigned long   bfOffs;      /* 0A */
  unsigned long   biSize;      /* 0E */
} Bitmap_File_Head;

struct Bitmap_Head_Struct
{
  unsigned long   biWidth;     /* 12 */
  unsigned long   biHeight;    /* 16 */
  unsigned short  biPlanes;    /* 1A */
  unsigned short  biBitCnt;    /* 1C */
  unsigned long   biCompr;     /* 1E */
  unsigned long   biSizeIm;    /* 22 */
  unsigned long   biXPels;     /* 26 */
  unsigned long   biYPels;     /* 2A */
  unsigned long   biClrUsed;   /* 2E */
  unsigned long   biClrImp;    /* 32 */
                        /* 36 */
} Bitmap_Head;

struct Bitmap_OS2_Head_Struct
{
  unsigned short  bcWidth;     /* 12 */
  unsigned short  bcHeight;    /* 14 */
  unsigned short  bcPlanes;    /* 16 */
  unsigned short  bcBitCnt;    /* 18 */
} Bitmap_OS2_Head;

static long         ToL           (unsigned char *);
static void         FromL         (long,
				   unsigned char *);
static short        ToS           (unsigned char *);
static void         FromS         (short,
				   unsigned char *);
static int          ReadColorMap  (FILE *,
				   unsigned char[256][3],
				   int,
				   int,
				   int *);
static unsigned char        *ReadImage     (FILE *,
				   int,
				   int,
				   unsigned char[256][3],
				   int,
				   int,
				   int,
				   int,
				   int);
static void         WriteColorMap (FILE *,
				   int *,
				   int *,
				   int *,
				   int);
static void         WriteImage    (FILE *,
				   unsigned char *,
				   int,
				   int,
				   int,
				   int,
				   int,
				   int,
				   int);
static int read_os2_head1         (FILE *,
				   int headsz,
				   struct Bitmap_Head_Struct *p);

bitmap_type
ReadBMP (string filename)
{
  FILE *fd;
  unsigned char buf[5];
  int ColormapSize, LineBuffer, Maps, Grey;
  unsigned char ColorMap[256][3];
  unsigned char puffer[50];
  bitmap_type image;

  fd = fopen (filename, "rb");

  if (!fd)
      FATAL1 ("Can't open \"%s\"\n", filename);

  /* It is a File. Now is it a Bitmap? */

  if (!ReadOK(fd,buf,2) || (strncmp(buf,"BM",2)))
      FATAL1 ("Not a valid BMP file %s\n", buf);

  /* How long is the Header? */

  if (!ReadOK (fd, puffer, 0x10))
      FATAL ("Error reading BMP file header\n");

  /* bring them to the right byteorder. Not too nice, but it should work */

  Bitmap_File_Head.bfSize    = ToL (&puffer[0]);
  Bitmap_File_Head.reserverd = ToL (&puffer[4]);
  Bitmap_File_Head.bfOffs    = ToL (&puffer[8]);
  Bitmap_File_Head.biSize    = ToL (&puffer[12]);

  /* Is it a Windows (R) Bitmap or not */

  if (Bitmap_File_Head.biSize == 12) /* old OS/2 */
    {
      if (!read_os2_head1 (fd, Bitmap_File_Head.biSize - 4, &Bitmap_Head))
          FATAL ("%s: error reading BMP file header\n");
      Maps = 3;
      /*if (!ReadOK (fd, puffer, Bitmap_File_Head.biSize))
          FATAL ("Error reading BMP file header\n");

      Bitmap_OS2_Head.bcWidth  = ToS (&puffer[0]);
      Bitmap_OS2_Head.bcHeight = ToS (&puffer[2]);
      Bitmap_OS2_Head.bcPlanes = ToS (&puffer[4]);
      Bitmap_OS2_Head.bcBitCnt = ToS (&puffer[6]);

      Bitmap_Head.biPlanes    = Bitmap_OS2_Head.bcPlanes;
      Bitmap_Head.biBitCnt    = Bitmap_OS2_Head.bcBitCnt;
      Bitmap_File_Head.bfSize = ((Bitmap_File_Head.bfSize * 4) -
				 (Bitmap_File_Head.bfOffs * 3));
      Bitmap_Head.biHeight    = Bitmap_OS2_Head.bcHeight;
      Bitmap_Head.biWidth     = Bitmap_OS2_Head.bcWidth;
      Bitmap_Head.biClrUsed   = 0;
      Bitmap_Head.biCompr     = 0;
      Maps = 3;*/
    }
  else if (Bitmap_File_Head.biSize != 40) /* new OS/2 or other */
      FATAL ("Unsupported format\n");
  else
    {
      if (!ReadOK (fd, puffer, 36))
          FATAL ("Error reading BMP file header\n");
      Bitmap_Head.biWidth   =ToL (&puffer[0x00]);	/* 12 */
      Bitmap_Head.biHeight  =ToL (&puffer[0x04]);	/* 16 */
      Bitmap_Head.biPlanes  =ToS (&puffer[0x08]);       /* 1A */
      Bitmap_Head.biBitCnt  =ToS (&puffer[0x0A]);	/* 1C */
      Bitmap_Head.biCompr   =ToL (&puffer[0x0C]);	/* 1E */
      Bitmap_Head.biSizeIm  =ToL (&puffer[0x10]);	/* 22 */
      Bitmap_Head.biXPels   =ToL (&puffer[0x14]);	/* 26 */
      Bitmap_Head.biYPels   =ToL (&puffer[0x18]);	/* 2A */
      Bitmap_Head.biClrUsed =ToL (&puffer[0x1C]);	/* 2E */
      Bitmap_Head.biClrImp  =ToL (&puffer[0x20]);	/* 32 */
    					                /* 36 */
      Maps = 4;
    }

  if (Bitmap_Head.biBitCnt == 24 && Bitmap_Head.biPlanes != 3)
      /* Bitcount does not match to number of planes */
      Bitmap_Head.biPlanes = 3;

  /* This means wrong file Format. I test this because it could crash the
   * whole.
   */

  if (Bitmap_Head.biBitCnt > 24)
      FATAL1 ("Too many colors: %u\n",
		 (unsigned int) Bitmap_Head.biBitCnt);

  /* There should be some colors used! */

  ColormapSize = (Bitmap_File_Head.bfOffs - Bitmap_File_Head.biSize - 14) / Maps;

  if ((Bitmap_Head.biClrUsed == 0) &&
      (Bitmap_Head.biBitCnt < 24))
    Bitmap_Head.biClrUsed = ColormapSize;

  if (Bitmap_Head.biBitCnt == 24)
    LineBuffer = ((Bitmap_File_Head.bfSize - Bitmap_File_Head.bfOffs) /
		     Bitmap_Head.biHeight);
  else
    LineBuffer = ((Bitmap_File_Head.bfSize - Bitmap_File_Head.bfOffs) /
		     Bitmap_Head.biHeight) * (8 / Bitmap_Head.biBitCnt);

#ifdef DEBUG
  printf("\nSize: %u, Colors: %u, Bits: %u, Width: %u, Height: %u, Comp: %u, Zeile: %u\n",
          Bitmap_File_Head.bfSize,Bitmap_Head.biClrUsed,Bitmap_Head.biBitCnt,Bitmap_Head.biWidth,
          Bitmap_Head.biHeight, Bitmap_Head.biCompr, LineBuffer);
#endif

  /* Get the Colormap */

  if (ReadColorMap (fd, ColorMap, ColormapSize, Maps, &Grey) == -1)
      FATAL ("Cannot read the colormap");

#ifdef DEBUG
  printf("Colormap read\n");
#endif

  /* Get the Image and return the ID or -1 on error*/
  image.bitmap = ReadImage (fd,
			Bitmap_Head.biWidth,
			Bitmap_Head.biHeight,
			ColorMap,
			Bitmap_Head.biClrUsed,
			Bitmap_Head.biBitCnt,
			Bitmap_Head.biCompr,
			LineBuffer,
			Grey);
  BITMAP_WIDTH (image) = Bitmap_Head.biWidth;
  BITMAP_HEIGHT (image) = Bitmap_Head.biHeight;
  BITMAP_PLANES (image) = Grey ? 1 : 3;

  return (image);
}

static int
ReadColorMap (FILE   *fd,
	      unsigned char  buffer[256][3],
	      int    number,
	      int    size,
	      int   *grey)
{
  int i;
  unsigned char rgb[4];

  *grey=(number>2);
  for (i = 0; i < number ; i++)
    {
      if (!ReadOK (fd, rgb, size))
          FATAL ("Bad colormap\n");

      /* Bitmap save the colors in another order! But change only once! */

      if (size == 4)
	{
	  buffer[i][0] = rgb[2];
	  buffer[i][1] = rgb[1];
	  buffer[i][2] = rgb[0];
	}
      else
	{
	  /* this one is for old os2 Bitmaps, but it dosn't work well */
	  buffer[i][0] = rgb[1];
	  buffer[i][1] = rgb[0];
	  buffer[i][2] = rgb[2];
	}
      *grey = ((*grey) && (rgb[0]==rgb[1]) && (rgb[1]==rgb[2]));
    }
  return 0;
}

static unsigned char*
ReadImage (FILE   *fd,
	   int    width,
	   int    height,
	   unsigned char  cmap[256][3],
	   int    ncols,
	   int    bpp,
	   int    compression,
	   int    line,
	   int    grey)
{
  unsigned char v,howmuch;
  unsigned char buf[16];
  int xpos = 0, ypos = 0;
  unsigned char *image;
  unsigned char *temp;
  long rowstride, channels;
  int i, j, notused;

  if (grey)
    {
      XMALLOC (image, width * height * sizeof (unsigned char));
      channels = 1;
    }
  else
    {
      if (bpp<24)
	{
    XMALLOC (image, width * height * 1 * sizeof (unsigned char));
	  channels = 1;
	}
      else
	{
    XMALLOC (image, width * height * 3 * sizeof (unsigned char));
	  channels = 3;
	}
    }

  rowstride = width * channels;

  ypos = height - 1;  /* Bitmaps begin in the lower left corner */

  if (bpp == 24)
    {
      while (ReadOK (fd, buf, 3))
        {
          temp = image + (ypos * rowstride) + (xpos * channels);
          *temp=buf[2];
          temp++;
          *temp=buf[1];
          temp++;
          *temp=buf[0];
          xpos++;
          if (xpos == width)
            {
              notused=ReadOK (fd, buf, line - (width * 3));
              ypos--;
              xpos = 0;
	    }
	  if (ypos < 0)
	    break;
        }
    }
  else
    {
      switch(compression)
	{
	case 0:  			/* uncompressed */
	  {
	    while (ReadOK (fd, &v, 1))
	      {
		for (i = 1; (i <= (8 / bpp)) && (xpos < width); i++, xpos++)
		  {
		    temp = image + (ypos * rowstride) + (xpos * channels);
		    *temp=( v & ( ((1<<bpp)-1) << (8-(i*bpp)) ) ) >> (8-(i*bpp));
		    if (grey)
		      *temp = cmap[*temp][0];
		  }
		if (xpos == width)
		  {
		    notused = ReadOK (fd, buf, (line - width) / (8 / bpp));
		    ypos--;
		    xpos = 0;
		  }
		if (ypos < 0)
		  break;
	      }
	    break;
	  }
	default:			/* Compressed images */
	  {
	    while (TRUE)
	      {
		notused = ReadOK (fd, buf, 2);
		if ((unsigned char) buf[0] != 0)
		  /* Count + Color - record */
		  {
		    for (j = 0; ((unsigned char) j < (unsigned char) buf[0]) && (xpos < width);)
		      {
			for (i = 1;
			     ((i <= (8 / bpp)) &&
			      (xpos < width) &&
			      ((unsigned char) j < (unsigned char) buf[0]));
			     i++, xpos++, j++)
			  {
			    temp = image + (ypos * rowstride) + (xpos * channels);
			    *temp = (buf[1] & (((1<<bpp)-1) << (8 - (i * bpp)))) >> (8 - (i * bpp));
			    if (grey)
			      *temp = cmap[*temp][0];
			  }
		      }
		  }
		if (((unsigned char) buf[0] == 0) && ((unsigned char) buf[1] > 2))
		  /* uncompressed record */
		  {
		    howmuch = buf[1];
		    for (j = 0; j < howmuch; j += (8 / bpp))
		      {
			notused = ReadOK (fd, &v, 1);
			i = 1;
			while ((i <= (8 / bpp)) && (xpos < width))
			  {
			    temp = image + (ypos * rowstride) + (xpos * channels);
			    *temp = (v & (((1<<bpp)-1) << (8-(i*bpp)))) >> (8-(i*bpp));
			    if (grey)
			      *temp = cmap[*temp][0];
			    i++;
			    xpos++;
			  }
		      }

		    if ((howmuch % 2) && (bpp==4))
		      howmuch++;

		    if ((howmuch / (8 / bpp)) % 2)
		      notused = ReadOK (fd, &v, 1);
		    /*if odd(x div (8 div bpp )) then blockread(f,z^,1);*/
		  }
		if (((unsigned char) buf[0] == 0) && ((unsigned char) buf[1]==0))
		  /* Zeilenende */
		  {
		    ypos--;
		    xpos = 0;
		  }
		if (((unsigned char) buf[0]==0) && ((unsigned char) buf[1]==1))
		  /* Bitmapende */
		  {
		    break;
		  }
		if (((unsigned char) buf[0]==0) && ((unsigned char) buf[1]==2))
		  /* Deltarecord */
		  {
		    xpos += (unsigned char) buf[2];
		    ypos += (unsigned char) buf[3];
		  }
	      }
	    break;
	  }
	}
    }

  fclose (fd);
  if (bpp < 24)
    {
      unsigned char *temp2, *temp3;
      unsigned char index;
      temp2 = temp = image;
      XMALLOC (image, width * height * 3 * sizeof (unsigned char));
      temp3 = image;
      for (ypos = 0; ypos < height; ypos++)
        {
          for (xpos = 0; xpos < width; xpos++)
             {
               index = *temp2++;
               *temp3++ = cmap[index][0];
               *temp3++ = cmap[index][1];
               *temp3++ = cmap[index][2];
           }
        }
      free (temp);
  }

  return image;
}

FILE  *errorfile;
char *prog_name = "bmp";
char *filename;
int   interactive_bmp;

struct Bitmap_File_Head_Struct Bitmap_File_Head;
struct Bitmap_Head_Struct Bitmap_Head;
struct Bitmap_OS2_Head_Struct Bitmap_OS2_Head;

static long
ToL (unsigned char *puffer)
{
  return (puffer[0] | puffer[1]<<8 | puffer[2]<<16 | puffer[3]<<24);
}

static short
ToS (unsigned char *puffer)
{
  return (puffer[0] | puffer[1]<<8);
}

static void
FromL (long  wert,
       unsigned char *bopuffer)
{
  bopuffer[0] = (wert & 0x000000ff)>>0x00;
  bopuffer[1] = (wert & 0x0000ff00)>>0x08;
  bopuffer[2] = (wert & 0x00ff0000)>>0x10;
  bopuffer[3] = (wert & 0xff000000)>>0x18;
}

static void
FromS (short  wert,
       unsigned char *bopuffer)
{
  bopuffer[0] = (wert & 0x00ff)>>0x00;
  bopuffer[1] = (wert & 0xff00)>>0x08;
}

static void dump_file_head (struct Bitmap_File_Head_Struct *p)
{
  printf("bfSize=%ld\n",p->bfSize);
  printf("bfoffs=%ld\n",p->bfOffs);
  printf("biSize=%ld\n",p->biSize);	
}

static void dump_os2_head (struct Bitmap_OS2_Head_Struct *p)
{
  printf("bcWidth =%4d ",p->bcWidth);
  printf("bcHeigth=%4d\n",p->bcHeight);
  printf("bcPlanes=%4d ",p->bcPlanes);
  printf("bcBitCnt=%4d\n",p->bcBitCnt);
}

static int read_os2_head1 (FILE *fd, int headsz, struct Bitmap_Head_Struct *p)
{
  unsigned char puffer[150];

  if (!ReadOK (fd, puffer, headsz))
    {
      return 0;
    }
  
  Bitmap_OS2_Head.bcWidth  = ToS (&puffer[0]);
  Bitmap_OS2_Head.bcHeight = ToS (&puffer[2]);
  Bitmap_OS2_Head.bcPlanes = ToS (&puffer[4]);
  Bitmap_OS2_Head.bcBitCnt = ToS (&puffer[6]);
#if 0
  dump_os2_head (&Bitmap_OS2_Head);
#endif
  p->biHeight    = Bitmap_OS2_Head.bcHeight;
  p->biWidth     = Bitmap_OS2_Head.bcWidth;
  p->biPlanes    = Bitmap_OS2_Head.bcPlanes;
  p->biBitCnt    = Bitmap_OS2_Head.bcBitCnt;
  return 1;
}

/* version 0.24 */