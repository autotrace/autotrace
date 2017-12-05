/* input-pnm.c: import pnm and pbm files.

   Copyright (C) 1999, 2000, 2001 Erik Nygren <nygren@mit.edu>

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

/*
 * The pnm reading and writing code was written from scratch by Erik Nygren
 * (nygren@mit.edu) based on the specifications in the man pages and
 * does not contain any code from the netpbm or pbmplus distributions.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* Def: HAVE_CONFIG_H */

#include "types.h"
#include "bitmap.h"
#include "input-pnm.h"
#include "logreport.h"
#include "xstd.h"

#include <math.h>
#include <ctype.h>

/* Declare local data types
 */

typedef struct _PNMScanner {
  FILE *fd;                     /* The file descriptor of the file being read */
  char cur;                     /* The current character in the input stream */
  int eof;                      /* Have we reached end of file? */
  char *inbuf;                  /* Input buffer - initially 0 */
  int inbufsize;                /* Size of input buffer */
  int inbufvalidsize;           /* Size of input buffer with valid data */
  int inbufpos;                 /* Position in input buffer */
} PNMScanner;

typedef struct _PNMInfo {
  unsigned int xres, yres;      /* The size of the image */
  int maxval;                   /* For ascii image files, the max value
                                 * which we need to normalize to */
  int np;                       /* Number of image planes (0 for pbm) */
  int asciibody;                /* 1 if ascii body, 0 if raw body */
  /* Routine to use to load the pnm body */
  void (*loader) (PNMScanner *, struct _PNMInfo *, unsigned char *, at_exception_type * excep);
} PNMInfo;

#define BUFLEN 512              /* The input buffer size for data returned
                                 * from the scanner.  Note that lines
                                 * aren't allowed to be over 256 characters
                                 * by the spec anyways so this shouldn't
                                 * be an issue. */

/* Declare some local functions.
 */

static void pnm_load_ascii(PNMScanner * scan, PNMInfo * info, unsigned char *pixel_rgn, at_exception_type * excep);
static void pnm_load_raw(PNMScanner * scan, PNMInfo * info, unsigned char *pixel_rgn, at_exception_type * excep);
static void pnm_load_rawpbm(PNMScanner * scan, PNMInfo * info, unsigned char *pixel_rgn, at_exception_type * excep);

static void pnmscanner_destroy(PNMScanner * s);
static void pnmscanner_createbuffer(PNMScanner * s, unsigned int bufsize);
static void pnmscanner_getchar(PNMScanner * s);
static void pnmscanner_eatwhitespace(PNMScanner * s);
static void pnmscanner_gettoken(PNMScanner * s, unsigned char *buf, unsigned int bufsize);
static void pnmscanner_getsmalltoken(PNMScanner * s, unsigned char *buf);

static PNMScanner *pnmscanner_create(FILE * fd);

#define pnmscanner_eof(s) ((s)->eof)
#define pnmscanner_fd(s) ((s)->fd)

static struct struct_pnm_types {
  char name;
  int np;
  int asciibody;
  int maxval;
  void (*loader) (PNMScanner *, struct _PNMInfo *, unsigned char *pixel_rgn, at_exception_type * excep);
} pnm_types[] = {
  {
  '1', 0, 1, 1, pnm_load_ascii},  /* ASCII PBM */
  {
  '2', 1, 1, 255, pnm_load_ascii},  /* ASCII PGM */
  {
  '3', 3, 1, 255, pnm_load_ascii},  /* ASCII PPM */
  {
  '4', 0, 0, 1, pnm_load_rawpbm}, /* RAW   PBM */
  {
  '5', 1, 0, 255, pnm_load_raw},  /* RAW   PGM */
  {
  '6', 3, 0, 255, pnm_load_raw},  /* RAW   PPM */
  {
  0, 0, 0, 0, NULL}
};

at_bitmap input_pnm_reader(gchar * filename, at_input_opts_type * opts, at_msg_func msg_func, gpointer msg_data, gpointer user_data)
{
  char buf[BUFLEN];             /* buffer for random things like scanning */
  PNMInfo *pnminfo;
  PNMScanner *volatile scan;
  int ctr;
  FILE *fd;
  at_bitmap bitmap = at_bitmap_init(NULL, 0, 0, 0);
  at_exception_type excep = at_exception_new(msg_func, msg_data);

  /* open the file */
  fd = fopen(filename, "rb");

  if (fd == NULL) {
    LOG("pnm filter: can't open file\n");
    at_exception_fatal(&excep, "pnm filter: can't open file");
    return (bitmap);
  }

  /* allocate the necessary structures */
  pnminfo = (PNMInfo *) malloc(sizeof(PNMInfo));

  scan = NULL;
  /* set error handling */

  scan = pnmscanner_create(fd);

  /* Get magic number */
  pnmscanner_gettoken(scan, (unsigned char *)buf, BUFLEN);
  if (pnmscanner_eof(scan)) {
    LOG("pnm filter: premature end of file\n");
    at_exception_fatal(&excep, "pnm filter: premature end of file");
    goto cleanup;
  }
  if (buf[0] != 'P' || buf[2]) {
    LOG("pnm filter: %s is not a valid file\n", filename);
    at_exception_fatal(&excep, "pnm filter: invalid file");
    goto cleanup;
  }

  /* Look up magic number to see what type of PNM this is */
  for (ctr = 0; pnm_types[ctr].name; ctr++)
    if (buf[1] == pnm_types[ctr].name) {
      pnminfo->np = pnm_types[ctr].np;
      pnminfo->asciibody = pnm_types[ctr].asciibody;
      pnminfo->maxval = pnm_types[ctr].maxval;
      pnminfo->loader = pnm_types[ctr].loader;
    }
  if (!pnminfo->loader) {
    LOG("pnm filter: file not in a supported format\n");
    at_exception_fatal(&excep, "pnm filter: file not in a supported format");
    goto cleanup;
  }

  pnmscanner_gettoken(scan, (unsigned char *)buf, BUFLEN);
  if (pnmscanner_eof(scan)) {
    LOG("pnm filter: premature end of file\n");
    at_exception_fatal(&excep, "pnm filter: premature end of file");
    goto cleanup;
  }
  pnminfo->xres = isdigit(*buf) ? atoi(buf) : 0;
  if (pnminfo->xres <= 0) {
    LOG("pnm filter: invalid xres while loading\n");
    at_exception_fatal(&excep, "pnm filter: premature end of file");
    goto cleanup;
  }

  pnmscanner_gettoken(scan, (unsigned char *)buf, BUFLEN);
  if (pnmscanner_eof(scan)) {
    LOG("pnm filter: premature end of file\n");
    at_exception_fatal(&excep, "pnm filter: premature end of file");
    goto cleanup;
  }
  pnminfo->yres = isdigit(*buf) ? atoi(buf) : 0;
  if (pnminfo->yres <= 0) {
    LOG("pnm filter: invalid yres while loading\n");
    at_exception_fatal(&excep, "pnm filter: invalid yres while loading");
    goto cleanup;
  }

  if (pnminfo->np != 0) {       /* pbm's don't have a maxval field */
    pnmscanner_gettoken(scan, (unsigned char *)buf, BUFLEN);
    if (pnmscanner_eof(scan)) {
      LOG("pnm filter: premature end of file\n");
      at_exception_fatal(&excep, "pnm filter: invalid yres while loading");
      goto cleanup;
    }

    pnminfo->maxval = isdigit(*buf) ? atoi(buf) : 0;
    if ((pnminfo->maxval <= 0)
        || (pnminfo->maxval > 255 && !pnminfo->asciibody)) {
      LOG("pnm filter: invalid maxval while loading\n");
      at_exception_fatal(&excep, "pnm filter: invalid maxval while loading");
      goto cleanup;
    }
  }

  bitmap = at_bitmap_init(NULL, (unsigned short)pnminfo->xres, (unsigned short)pnminfo->yres, (pnminfo->np) ? (pnminfo->np) : 1);
  pnminfo->loader(scan, pnminfo, AT_BITMAP_BITS(&bitmap), &excep);

cleanup:
  /* Destroy the scanner */
  pnmscanner_destroy(scan);

  /* free the structures */
  free(pnminfo);

  /* close the file */
  fclose(fd);

  return (bitmap);
}

static void pnm_load_ascii(PNMScanner * scan, PNMInfo * info, unsigned char *data, at_exception_type * excep)
{
  unsigned char *d;
  unsigned int x;
  int i, b;
  int start, end, scanlines;
  int np;
  char buf[BUFLEN];

  np = (info->np) ? (info->np) : 1;

  /* Buffer reads to increase performance */
  pnmscanner_createbuffer(scan, 4096);

  start = 0;
  end = info->yres;
  scanlines = end - start;
  d = data;

  for (i = 0; i < scanlines; i++)
    for (x = 0; x < info->xres; x++) {
      for (b = 0; b < np; b++) {
        /* Truncated files will just have all 0's at the end of the images */
        if (pnmscanner_eof(scan)) {
          LOG("pnm filter: premature end of file\n");
          at_exception_fatal(excep, "pnm filter: premature end of file");
          return;
        }
        if (info->np)
          pnmscanner_gettoken(scan, (unsigned char *)buf, BUFLEN);
        else
          pnmscanner_getsmalltoken(scan, (unsigned char *)buf);
        switch (info->maxval) {
        case 255:
          d[b] = (unsigned char)(isdigit(*buf) ? atoi(buf) : 0);
          break;
        case 1:
          d[b] = (*buf == '0') ? 0xff : 0x00;
          break;
        default:
          d[b] = (unsigned char)(255.0 * (((double)(isdigit(*buf) ? atoi(buf) : 0))
                                          / (double)(info->maxval)));
        }
      }

      d += np;
    }
}

static void pnm_load_raw(PNMScanner * scan, PNMInfo * info, unsigned char *data, at_exception_type * excep)
{
  unsigned char *d;
  unsigned int x, i;
  unsigned int start, end, scanlines;
  FILE *fd;

  fd = pnmscanner_fd(scan);

  start = 0;
  end = info->yres;
  scanlines = end - start;
  d = data;

  for (i = 0; i < scanlines; i++) {
    if (info->xres * info->np != fread(d, 1, info->xres * info->np, fd)) {
      LOG("pnm filter: premature end of file\n");
      at_exception_fatal(excep, "pnm filter: premature end of file\n");
      return;
    }

    if (info->maxval != 255) {  /* Normalize if needed */
      for (x = 0; x < info->xres * info->np; x++)
        d[x] = (unsigned char)(255.0 * (double)(d[x]) / (double)(info->maxval));
    }

    d += info->xres * info->np;
  }
}

static void pnm_load_rawpbm(PNMScanner * scan, PNMInfo * info, unsigned char *data, at_exception_type * excep)
{
  unsigned char *buf;
  unsigned char curbyte;
  unsigned char *d;
  unsigned int x, i;
  unsigned int start, end, scanlines;
  FILE *fd;
  unsigned int rowlen, bufpos;

  fd = pnmscanner_fd(scan);
  rowlen = (unsigned int)ceil((double)(info->xres) / 8.0);
  buf = (unsigned char *)malloc(rowlen * sizeof(unsigned char));

  start = 0;
  end = info->yres;
  scanlines = end - start;
  d = data;

  for (i = 0; i < scanlines; i++) {
    if (rowlen != fread(buf, 1, rowlen, fd)) {
      LOG("pnm filter: error reading file\n");
      at_exception_fatal(excep, "pnm filter: error reading file");
      goto cleanup;
    }
    bufpos = 0;
    curbyte = buf[0];

    for (x = 0; x < info->xres; x++) {
      if ((x % 8) == 0)
        curbyte = buf[bufpos++];
      d[x] = (curbyte & 0x80) ? 0x00 : 0xff;
      curbyte <<= 1;
    }

    d += info->xres;
  }
cleanup:
  free(buf);
}

/**************** FILE SCANNER UTILITIES **************/

/* pnmscanner_create ---
 *    Creates a new scanner based on a file descriptor.  The
 *    look ahead buffer is one character initially.
 */
static PNMScanner *pnmscanner_create(FILE * fd)
{
  PNMScanner *s;

  XMALLOC(s, sizeof(PNMScanner));
  s->fd = fd;
  s->inbuf = 0;
  s->eof = !fread(&(s->cur), 1, 1, s->fd);
  return (s);
}

/* pnmscanner_destroy ---
 *    Destroys a scanner and its resources.  Doesn't close the fd.
 */
static void pnmscanner_destroy(PNMScanner * s)
{
  free(s->inbuf);
  free(s);
}

/* pnmscanner_createbuffer ---
 *    Creates a buffer so we can do buffered reads.
 */
static void pnmscanner_createbuffer(PNMScanner * s, unsigned int bufsize)
{
  s->inbuf = (char *)malloc(sizeof(char) * bufsize);
  s->inbufsize = bufsize;
  s->inbufpos = 0;
  s->inbufvalidsize = fread(s->inbuf, 1, bufsize, s->fd);
}

/* pnmscanner_gettoken ---
 *    Gets the next token, eating any leading whitespace.
 */
static void pnmscanner_gettoken(PNMScanner * s, unsigned char *buf, unsigned int bufsize)
{
  unsigned int ctr = 0;

  pnmscanner_eatwhitespace(s);
  while (!(s->eof) && !isspace(s->cur) && (s->cur != '#') && (ctr < bufsize)) {
    buf[ctr++] = s->cur;
    pnmscanner_getchar(s);
  }
  buf[ctr] = '\0';
}

/* pnmscanner_getsmalltoken ---
 *    Gets the next char, eating any leading whitespace.
 */
static void pnmscanner_getsmalltoken(PNMScanner * s, unsigned char *buf)
{
  pnmscanner_eatwhitespace(s);
  if (!(s->eof) && !isspace(s->cur) && (s->cur != '#')) {
    *buf = s->cur;
    pnmscanner_getchar(s);
  }
}

/* pnmscanner_getchar ---
 *    Reads a character from the input stream
 */
static void pnmscanner_getchar(PNMScanner * s)
{
  if (s->inbuf) {
    s->cur = s->inbuf[s->inbufpos++];
    if (s->inbufpos >= s->inbufvalidsize) {
      if (s->inbufsize > s->inbufvalidsize)
        s->eof = 1;
      else
        s->inbufvalidsize = fread(s->inbuf, 1, s->inbufsize, s->fd);
      s->inbufpos = 0;
    }
  } else
    s->eof = !fread(&(s->cur), 1, 1, s->fd);
}

/* pnmscanner_eatwhitespace ---
 *    Eats up whitespace from the input and returns when done or eof.
 *    Also deals with comments.
 */
static void pnmscanner_eatwhitespace(PNMScanner * s)
{
  int state = 0;

  while (!(s->eof) && (state != -1)) {
    switch (state) {
    case 0:                    /* in whitespace */
      if (s->cur == '#') {
        state = 1;              /* goto comment */
        pnmscanner_getchar(s);
      } else if (!isspace(s->cur))
        state = -1;
      else
        pnmscanner_getchar(s);
      break;

    case 1:                    /* in comment */
      if (s->cur == '\n')
        state = 0;              /* goto whitespace */
      pnmscanner_getchar(s);
      break;
    }
  }
}
