/*
 * The pnm reading and writing code was written from scratch by Erik Nygren
 * (nygren@mit.edu) based on the specifications in the man pages and
 * does not contain any code from the netpbm or pbmplus distributions.
 */

#include "bitmap.h"
#include "input-pnm.h"
#include "message.h"
#include "xstd.h"

#include <math.h>
#include <ctype.h>

/* Declare local data types
 */

typedef struct _PNMScanner
{
  FILE   *fd;		      /* The file descriptor of the file being read */
  char   cur;		      /* The current character in the input stream */
  int    eof;		      /* Have we reached end of file? */
  char  *inbuf;	      /* Input buffer - initially 0 */
  int    inbufsize;	      /* Size of input buffer */
  int    inbufvalidsize;     /* Size of input buffer with valid data */
  int    inbufpos;           /* Position in input buffer */
} PNMScanner;

typedef struct _PNMInfo
{
  unsigned int       xres, yres;	/* The size of the image */
  int       maxval;		/* For ascii image files, the max value
				 * which we need to normalize to */
  int       np;		/* Number of image planes (0 for pbm) */
  int       asciibody;		/* 1 if ascii body, 0 if raw body */
  /* Routine to use to load the pnm body */
  void    (* loader) (PNMScanner *, struct _PNMInfo *, unsigned char *);
} PNMInfo;

/* Contains the information needed to write out PNM rows */
typedef struct _PNMRowInfo
{
  FILE   *fd;		/* File descriptor */
  char  *rowbuf;	/* Buffer for writing out rows */
  int    xres;		/* X resolution */
  int    np;		/* Number of planes */
  unsigned char *red;		/* Colormap red */
  unsigned char *grn;		/* Colormap green */
  unsigned char *blu;		/* Colormap blue */
} PNMRowInfo;

/* Save info  */
typedef struct
{
  int  raw;  /*  raw or ascii  */
} PNMSaveVals;

typedef struct
{
  int  run;  /*  run  */
} PNMSaveInterface;

#define BUFLEN 512		/* The input buffer size for data returned
				 * from the scanner.  Note that lines
				 * aren't allowed to be over 256 characters
				 * by the spec anyways so this shouldn't
				 * be an issue. */

#define SAVE_COMMENT_STRING "# CREATOR: The GIMP's PNM Filter Version 1.0\n"

/* Declare some local functions.
 */

static void   pnm_load_ascii           (PNMScanner *scan,
					PNMInfo    *info,
					unsigned char  *pixel_rgn);
static void   pnm_load_raw             (PNMScanner *scan,
					PNMInfo    *info,
					unsigned char  *pixel_rgn);
static void   pnm_load_rawpbm          (PNMScanner *scan,
					PNMInfo    *info,
					unsigned char  *pixel_rgn);

static void   pnmscanner_destroy       (PNMScanner *s);
static void   pnmscanner_createbuffer  (PNMScanner *s,
					unsigned int bufsize);
static void   pnmscanner_getchar       (PNMScanner *s);
static void   pnmscanner_eatwhitespace (PNMScanner *s);
static void   pnmscanner_gettoken      (PNMScanner *s,
					unsigned char *buf,
					unsigned int bufsize);
static void   pnmscanner_getsmalltoken (PNMScanner *s,
					unsigned char *buf);

static PNMScanner * pnmscanner_create  (FILE        *fd);


#define pnmscanner_eof(s) ((s)->eof)
#define pnmscanner_fd(s) ((s)->fd)

static struct struct_pnm_types
{
  char   name;
  int    np;
  int    asciibody;
  int    maxval;
  void (* loader) (PNMScanner *, struct _PNMInfo *, unsigned char *pixel_rgn);
} pnm_types[] =
{
  { '1', 0, 1,   1, pnm_load_ascii },  /* ASCII PBM */
  { '2', 1, 1, 255, pnm_load_ascii },  /* ASCII PGM */
  { '3', 3, 1, 255, pnm_load_ascii },  /* ASCII PPM */
  { '4', 0, 0,   1, pnm_load_rawpbm }, /* RAW   PBM */
  { '5', 1, 0, 255, pnm_load_raw },    /* RAW   PGM */
  { '6', 3, 0, 255, pnm_load_raw },    /* RAW   PPM */
  {  0 , 0, 0,   0, NULL}
};

bitmap_type pnm_load_image (string filename)
{
  char buf[BUFLEN];		/* buffer for random things like scanning */
  PNMInfo *pnminfo;
  PNMScanner * volatile scan;
  int ctr;
  FILE* fd;
  bitmap_type bitmap;

  /* open the file */
  fd = fopen (filename, "rb");

  if (fd == NULL)
    {
      FATAL("pnm filter: can't open file\n");
      BITMAP_BITS (bitmap) = NULL;
      DIMENSIONS_WIDTH (bitmap.dimensions) = 0;
      DIMENSIONS_HEIGHT (bitmap.dimensions) = 0;
      BITMAP_PLANES (bitmap) = 0;
      return (bitmap);
    }

  /* allocate the necessary structures */
  pnminfo = (PNMInfo *) malloc (sizeof (PNMInfo));

  scan = NULL;
  /* set error handling */

  scan = pnmscanner_create(fd);

  /* Get magic number */
  pnmscanner_gettoken (scan, (unsigned char *)buf, BUFLEN); 
  if (pnmscanner_eof(scan))
    FATAL ("pnm filter: premature end of file\n");
  if (buf[0] != 'P' || buf[2])
    FATAL ("pnm filter: %s is not a valid file\n");

  /* Look up magic number to see what type of PNM this is */
  for (ctr=0; pnm_types[ctr].name; ctr++)
    if (buf[1] == pnm_types[ctr].name)
      {
	pnminfo->np        = pnm_types[ctr].np;
	pnminfo->asciibody = pnm_types[ctr].asciibody;
	pnminfo->maxval    = pnm_types[ctr].maxval;
	pnminfo->loader    = pnm_types[ctr].loader;
      }
  if (!pnminfo->loader)
      FATAL ("pnm filter: file not in a supported format\n");

  pnmscanner_gettoken(scan, (unsigned char *)buf, BUFLEN); 
  if (pnmscanner_eof(scan))
    FATAL ("pnm filter: premature end of file\n");
  pnminfo->xres = isdigit(*buf)?atoi(buf):0;
  if (pnminfo->xres<=0)
    FATAL ("pnm filter: invalid xres while loading\n");

  pnmscanner_gettoken(scan, (unsigned char *)buf, BUFLEN); 
  if (pnmscanner_eof(scan))
    FATAL ("pnm filter: premature end of file\n");
  pnminfo->yres = isdigit(*buf)?atoi(buf):0;
  if (pnminfo->yres<=0)
    FATAL ("pnm filter: invalid yres while loading\n");

  if (pnminfo->np != 0)		/* pbm's don't have a maxval field */
    {
      pnmscanner_gettoken(scan, (unsigned char *)buf, BUFLEN);
      if (pnmscanner_eof(scan))
        FATAL ("pnm filter: premature end of file\n");

      pnminfo->maxval = isdigit(*buf)?atoi(buf):0;
      if ((pnminfo->maxval<=0)
		|| (pnminfo->maxval>255 && !pnminfo->asciibody))
        FATAL ("pnm filter: invalid maxval while loading\n");
    }

  DIMENSIONS_WIDTH (BITMAP_DIMENSIONS (bitmap)) = pnminfo->xres;
  DIMENSIONS_HEIGHT (BITMAP_DIMENSIONS (bitmap)) = pnminfo->yres;

  BITMAP_PLANES (bitmap) = (pnminfo->np)?(pnminfo->np):1;
  BITMAP_BITS (bitmap) = (unsigned char *) malloc (pnminfo->yres *
    pnminfo->xres * BITMAP_PLANES (bitmap));
  pnminfo->loader (scan, pnminfo, BITMAP_BITS (bitmap));

  /* Destroy the scanner */
  pnmscanner_destroy (scan);

  /* free the structures */
  free (pnminfo);

  /* close the file */
  fclose (fd);
  
  return (bitmap);
}

static void
pnm_load_ascii (PNMScanner *scan,
		PNMInfo    *info,
		unsigned char *data)
{
  unsigned char *d;
  unsigned int x;
  int   i, b;
  int   start, end, scanlines;
  int   np;
  char           buf[BUFLEN];

  np = (info->np)?(info->np):1;

  /* Buffer reads to increase performance */
  pnmscanner_createbuffer(scan, 4096);

      start = 0;
      end = info->yres;
      scanlines = end - start;
      d = data;

      for (i = 0; i < scanlines; i++)
	for (x = 0; x < info->xres; x++)
	  {
	    for (b = 0; b < np; b++)
	      {
		/* Truncated files will just have all 0's at the end of the images */
		if (pnmscanner_eof(scan))
          FATAL ("pnm filter: premature end of file\n");
		if (info->np)
		  pnmscanner_gettoken(scan, (unsigned char *)buf, BUFLEN);
		else
		  pnmscanner_getsmalltoken(scan, (unsigned char *)buf);
		switch (info->maxval)
		  {
		  case 255:
		    d[b] = isdigit(*buf)?atoi(buf):0;
		    break;
		  case 1:
		    d[b] = (*buf=='0')?0xff:0x00;
		    break;
		  default:
		    d[b] = (unsigned char)(255.0*(((double)(isdigit(*buf)?atoi(buf):0))
						  / (double)(info->maxval)));
		  }
	      }

	    d += np;
	  }
}

static void
pnm_load_raw (PNMScanner *scan,
	      PNMInfo    *info,
	      unsigned char  *data)
{
  unsigned char *d;
  unsigned int   x, i;
  unsigned int   start, end, scanlines;
  FILE          *fd;

  fd = pnmscanner_fd(scan);

      start = 0;
      end = info->yres;
      scanlines = end - start;
      d = data;

      for (i = 0; i < scanlines; i++)
	{
	  if (info->xres*info->np
		!= fread(d, 1, info->xres*info->np, fd))
        FATAL ("pnm filter: premature end of file\n");

	  if (info->maxval != 255)	/* Normalize if needed */
	    {
	      for (x = 0; x < info->xres * info->np; x++)
		d[x] = (unsigned char)(255.0*(double)(d[x]) / (double)(info->maxval));
	    }

	  d += info->xres * info->np;
	}
}

static void
pnm_load_rawpbm (PNMScanner *scan,
		 PNMInfo    *info,
		 unsigned char  *data)
{
  unsigned char *buf;
  unsigned char  curbyte;
  unsigned char *d;
  unsigned int   x, i;
  unsigned int   start, end, scanlines;
  FILE          *fd;
  unsigned int            rowlen, bufpos;

  fd = pnmscanner_fd(scan);
  rowlen = (unsigned int)ceil((double)(info->xres)/8.0);
  buf = (unsigned char *)malloc(rowlen*sizeof(unsigned char));

      start = 0;
      end = info->yres;
      scanlines = end - start;
      d = data;

      for (i = 0; i < scanlines; i++)
	{
	  if (rowlen != fread(buf, 1, rowlen, fd))
        FATAL ("pnm filter: error reading file\n");
	  bufpos = 0;
	  curbyte = buf[0];

	  for (x = 0; x < info->xres; x++)
	    {
	      if ((x % 8) == 0)
		curbyte = buf[bufpos++];
	      d[x] = (curbyte&0x80) ? 0x00 : 0xff;
	      curbyte <<= 1;
	    }

	  d += info->xres;
	}

  free(buf);
}

/**************** FILE SCANNER UTILITIES **************/

/* pnmscanner_create ---
 *    Creates a new scanner based on a file descriptor.  The
 *    look ahead buffer is one character initially.
 */
static PNMScanner *
pnmscanner_create (FILE *fd)
{
  PNMScanner *s;

  XMALLOC (s, sizeof(PNMScanner));
  s->fd = fd;
  s->inbuf = 0;
  s->eof = !fread(&(s->cur), 1, 1, s->fd);
  return(s);
}

/* pnmscanner_destroy ---
 *    Destroys a scanner and its resources.  Doesn't close the fd.
 */
static void
pnmscanner_destroy (PNMScanner *s)
{
  if (s->inbuf) free(s->inbuf);
  free(s);
}

/* pnmscanner_createbuffer ---
 *    Creates a buffer so we can do buffered reads.
 */
static void
pnmscanner_createbuffer (PNMScanner *s,
			 unsigned int bufsize)
{
  s->inbuf = (char *)malloc(sizeof(char)*bufsize);
  s->inbufsize = bufsize;
  s->inbufpos = 0;
  s->inbufvalidsize = fread(s->inbuf, 1, bufsize, s->fd);
}

/* pnmscanner_gettoken ---
 *    Gets the next token, eating any leading whitespace.
 */
static void
pnmscanner_gettoken (PNMScanner *s,
		     unsigned char *buf,
		     unsigned int bufsize)
{
  unsigned int ctr=0;

  pnmscanner_eatwhitespace(s);
  while (!(s->eof) && !isspace(s->cur) && (s->cur != '#') && (ctr<bufsize))
    {
      buf[ctr++] = s->cur;
      pnmscanner_getchar(s);
    }
  buf[ctr] = '\0';
}

/* pnmscanner_getsmalltoken ---
 *    Gets the next char, eating any leading whitespace.
 */
static void
pnmscanner_getsmalltoken (PNMScanner *s,
			  unsigned char *buf)
{
  pnmscanner_eatwhitespace(s);
  if (!(s->eof) && !isspace(s->cur) && (s->cur != '#'))
    {
      *buf = s->cur;
      pnmscanner_getchar(s);
    }
}

/* pnmscanner_getchar ---
 *    Reads a character from the input stream
 */
static void
pnmscanner_getchar (PNMScanner *s)
{
  if (s->inbuf)
    {
      s->cur = s->inbuf[s->inbufpos++];
      if (s->inbufpos >= s->inbufvalidsize)
	{
	  if (s->inbufsize > s->inbufvalidsize)
	    s->eof = 1;
	  else
	    s->inbufvalidsize = fread(s->inbuf, 1, s->inbufsize, s->fd);
	  s->inbufpos = 0;
	}
    }
  else
    s->eof = !fread(&(s->cur), 1, 1, s->fd);
}

/* pnmscanner_eatwhitespace ---
 *    Eats up whitespace from the input and returns when done or eof.
 *    Also deals with comments.
 */
static void
pnmscanner_eatwhitespace (PNMScanner *s)
{
  int state = 0;

  while (!(s->eof) && (state != -1))
    {
      switch (state)
	{
	case 0:  /* in whitespace */
	  if (s->cur == '#')
	    {
	      state = 1;  /* goto comment */
	      pnmscanner_getchar(s);
	    }
	  else if (!isspace(s->cur))
	    state = -1;
	  else
	    pnmscanner_getchar(s);
	  break;

	case 1:  /* in comment */
	  if (s->cur == '\n')
	    state = 0;  /* goto whitespace */
	  pnmscanner_getchar(s);
	  break;
	}
    }
}

/* version 0.26 */
