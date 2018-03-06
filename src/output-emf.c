/* output-emf.c --- output in Enhanced Metafile format

   Copyright (C) 2000, 2001 Enrico Persiani

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
**  Notes:
**
**  Since EMF files are binary files, their persistent
**  format have to deal with endianess problems
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* Def: HAVE_CONFIG_H */

#include <stdio.h>
#include <string.h>
#include "spline.h"
#include "xstd.h"

/* EMF record-number definitions */

#define ENMT_HEADER                1
#define ENMT_EOF                  14
#define ENMT_POLYLINETO            6
#define ENMT_MOVETO               27
#define ENMT_POLYBEZIERTO          5
#define ENMT_BEGINPATH            59
#define ENMT_ENDPATH              60
#define ENMT_FILLPATH             62
#define ENMT_CREATEPEN            38
#define ENMT_CREATEBRUSHINDIRECT  39
#define ENMT_SELECTOBJECT         37
#define ENMT_SETWORLDTRANSFORM    35
#define ENMT_SETPOLYFILLMODE      19
#define ENMT_STROKEPATH           64
#define ENMT_LINETO               54
#define ENMT_POLYBEZIERTO16       88
//SPH: ADDED
#define ENMT_DELETEOBJECT         40

#define STOCK_NULL_PEN 0x80000008

#define FM_ALTERNATE 1

#define WDEVPIXEL 1280
#define HDEVPIXEL 1024
#define WDEVMLMTR 320
#define HDEVMLMTR 240

#define SCALE (gfloat) 1.0

#define MAKE_COLREF(r,g,b) (((r) & 0x0FF) | (((g) & 0x0FF) << 8) | (((b) & 0x0FF) << 16))
#define MK_PEN(n) ((n) * 2 + 1)
#define MK_BRUSH(n) ((n) * 2 + 2)
#define X_FLOAT_TO_UI32(num) ((uint32_t)(num * SCALE))
#define X_FLOAT_TO_UI16(num) ((uint16_t)(num * SCALE))
#define Y_FLOAT_TO_UI32(num) ((uint32_t)(y_offset - num * SCALE))
#define Y_FLOAT_TO_UI16(num) ((uint16_t)(y_offset - num * SCALE))

/* color list type */

typedef struct EMFColorListType {
  uint32_t colref;
  struct EMFColorListType *next;
} EMFColorList;

/* Emf stats needed for outputting EHNMETAHEADER*/

typedef struct {
  int ncolors;
  int nrecords;
  int filesize;
} EMFStats;

/* globals */

static EMFColorList *color_list = NULL; /* Color list */
static uint32_t *color_table = NULL;  /* Color table */
static float y_offset;

/* color list & table functions */

static int SearchColor(EMFColorList * head, uint32_t colref)
{
  while (head != NULL) {
    if (head->colref == colref)
      return 1;
    head = head->next;
  }
  return 0;
}

static void AddColor(EMFColorList ** head, uint32_t colref)
{
  EMFColorList *temp;

  XMALLOC(temp, sizeof(EMFColorList));

  temp->colref = colref;
  temp->next = *head;
  *head = temp;
}

static void ColorListToColorTable(EMFColorList ** head, uint32_t ** table, int len)
{
  EMFColorList *temp;
  int i = 0;

  XMALLOC(*table, sizeof(uint32_t) * len);

  while (*head != NULL) {
    temp = *head;
    *head = (*head)->next;
    (*table)[i] = temp->colref;
    i++;
    free(temp);
  }
}

static int ColorLookUp(uint32_t colref, uint32_t * table, int len)
{
  int i;

  for (i = 0; i < len; i++) {
    if (colref == table[i])
      return i;
  }
  return 0;
}

/* endianess independent IO functions */

static gboolean write32(FILE * fdes, uint32_t data)
{
  size_t count = 0;
  uint8_t outch;

  outch = (uint8_t) (data & 0x0FF);
  count += fwrite(&outch, 1, 1, fdes);

  outch = (uint8_t) ((data >> 8) & 0x0FF);
  count += fwrite(&outch, 1, 1, fdes);

  outch = (uint8_t) ((data >> 16) & 0x0FF);
  count += fwrite(&outch, 1, 1, fdes);

  outch = (uint8_t) ((data >> 24) & 0x0FF);
  count += fwrite(&outch, 1, 1, fdes);

  return (count == sizeof(uint32_t)) ? TRUE : FALSE;
}

static gboolean write16(FILE * fdes, uint16_t data)
{
  size_t count = 0;
  uint8_t outch;

  outch = (uint8_t) (data & 0x0FF);
  count += fwrite(&outch, 1, 1, fdes);

  outch = (uint8_t) ((data >> 8) & 0x0FF);
  count += fwrite(&outch, 1, 1, fdes);

  return (count == sizeof(uint16_t)) ? TRUE : FALSE;
}

/* EMF record-type function definitions */

static int WriteMoveTo(FILE * fdes, at_real_coord * pt)
{
  int recsize = sizeof(uint32_t) * 4;

  if (fdes != NULL) {
    write32(fdes, ENMT_MOVETO);
    write32(fdes, (uint32_t) recsize);
    write32(fdes, (uint32_t) X_FLOAT_TO_UI32(pt->x));
    write32(fdes, (uint32_t) Y_FLOAT_TO_UI32(pt->y));
  }
  return recsize;
}

static int WriteLineTo(FILE * fdes, spline_type * spl)
{
  int recsize = sizeof(uint32_t) * 4;

  if (fdes != NULL) {
    write32(fdes, ENMT_LINETO);
    write32(fdes, (uint32_t) recsize);
    write32(fdes, (uint32_t) X_FLOAT_TO_UI32(END_POINT(*spl).x));
    write32(fdes, (uint32_t) Y_FLOAT_TO_UI32(END_POINT(*spl).y));
  }
  return recsize;
}

/* CorelDraw 9 can't handle PolyLineTo nor PolyLineTo16, so we
   do not use this function but divide it to single lines instead
int WritePolyLineTo(FILE* fdes, spline_type *spl, int nlines)
{
  int i;
  int recsize = sizeof(uint32_t) * (7 + nlines * 1);

  if(fdes != NULL)
  {
    write32(fdes, ENMT_POLYLINETO);
    write32(fdes, (uint32_t) recsize);
    write32(fdes, (uint32_t) 0x0);
    write32(fdes, (uint32_t) 0x0);
    write32(fdes, (uint32_t) 0xFFFFFFFF);
    write32(fdes, (uint32_t) 0xFFFFFFFF);
    write32(fdes, (uint32_t) nlines);

    for(i=0; i<nlines; i++)
    {
      write16(fdes, (uint16_t) FLOAT_TO_UI32(END_POINT(spl[i]).x));
      write16(fdes, (uint16_t) FLOAT_TO_UI32(END_POINT(spl[i]).y));
    }
  }
  return recsize;
} */

static int MyWritePolyLineTo(FILE * fdes, spline_type * spl, int nlines)
{
  int i;
  int recsize = nlines * WriteLineTo(NULL, NULL);

  if (fdes != NULL) {
    for (i = 0; i < nlines; i++) {
      WriteLineTo(fdes, &spl[i]);
    }
  }
  return recsize;
}

/* CorelDraw 9 can't handle PolyBezierTo so we do not use this
   function but use PolyBezierTo16 instead

int WritePolyBezierTo(FILE *fdes, spline_type *spl, int ncurves)
{
  int i;
  int recsize = sizeof(uint32_t) * (7 + ncurves * 6);

  if(fdes != NULL)
  {
    write32(fdes, ENMT_POLYBEZIERTO);
    write32(fdes, (uint32_t) recsize);
    write32(fdes, (uint32_t) 0x0);
    write32(fdes, (uint32_t) 0x0);
    write32(fdes, (uint32_t) 0xFFFFFFFF);
    write32(fdes, (uint32_t) 0xFFFFFFFF);
    write32(fdes, (uint32_t) ncurves * 3);

    for(i=0; i<ncurves; i++)
    {
      write16(fdes, (uint32_t) FLOAT_TO_UI32(CONTROL1(spl[i]).x));
      write16(fdes, (uint32_t) FLOAT_TO_UI32(CONTROL1(spl[i]).y));
      write16(fdes, (uint32_t) FLOAT_TO_UI32(CONTROL2(spl[i]).x));
      write16(fdes, (uint32_t) FLOAT_TO_UI32(CONTROL2(spl[i]).y));
      write16(fdes, (uint32_t) FLOAT_TO_UI32(END_POINT(spl[i]).x));
      write16(fdes, (uint32_t) FLOAT_TO_UI32(END_POINT(spl[i]).y));
    }
  }
  return recsize;
} */

static int WritePolyBezierTo16(FILE * fdes, spline_type * spl, int ncurves)
{
  int i;
  int recsize = sizeof(uint32_t) * 7 + sizeof(uint16_t) * ncurves * 6;

  if (fdes != NULL) {
    write32(fdes, ENMT_POLYBEZIERTO16);
    write32(fdes, (uint32_t) recsize);
    write32(fdes, (uint32_t) 0x0);
    write32(fdes, (uint32_t) 0x0);
    write32(fdes, (uint32_t) 0xFFFFFFFF);
    write32(fdes, (uint32_t) 0xFFFFFFFF);
    write32(fdes, (uint32_t) ncurves * 3);

    for (i = 0; i < ncurves; i++) {
      write16(fdes, (uint16_t) X_FLOAT_TO_UI16(CONTROL1(spl[i]).x));
      write16(fdes, (uint16_t) Y_FLOAT_TO_UI16(CONTROL1(spl[i]).y));
      write16(fdes, (uint16_t) X_FLOAT_TO_UI16(CONTROL2(spl[i]).x));
      write16(fdes, (uint16_t) Y_FLOAT_TO_UI16(CONTROL2(spl[i]).y));
      write16(fdes, (uint16_t) X_FLOAT_TO_UI16(END_POINT(spl[i]).x));
      write16(fdes, (uint16_t) Y_FLOAT_TO_UI16(END_POINT(spl[i]).y));
    }
  }
  return recsize;
}

static int WriteSetPolyFillMode(FILE * fdes)
{
  int recsize = sizeof(uint32_t) * 3;

  if (fdes != NULL) {
    write32(fdes, ENMT_SETPOLYFILLMODE);
    write32(fdes, (uint32_t) recsize);
    write32(fdes, (uint32_t) FM_ALTERNATE);
  }
  return recsize;
}

static int WriteBeginPath(FILE * fdes)
{
  int recsize = sizeof(uint32_t) * 2;

  if (fdes != NULL) {
    write32(fdes, ENMT_BEGINPATH);
    write32(fdes, (uint32_t) recsize);
  }
  return recsize;
}

static int WriteEndPath(FILE * fdes)
{
  int recsize = sizeof(uint32_t) * 2;

  if (fdes != NULL) {
    write32(fdes, ENMT_ENDPATH);
    write32(fdes, (uint32_t) recsize);
  }
  return recsize;
}

static int WriteFillPath(FILE * fdes)
{
  int recsize = sizeof(uint32_t) * 6;

  if (fdes != NULL) {
    write32(fdes, ENMT_FILLPATH);
    write32(fdes, (uint32_t) recsize);
    write32(fdes, (uint32_t) 0x0);
    write32(fdes, (uint32_t) 0x0);
    write32(fdes, (uint32_t) 0xFFFFFFFF);
    write32(fdes, (uint32_t) 0xFFFFFFFF);
  }
  return recsize;
}

static int WriteStrokePath(FILE * fdes)
{
  int recsize = sizeof(uint32_t) * 6;

  if (fdes != NULL) {
    write32(fdes, ENMT_STROKEPATH);
    write32(fdes, (uint32_t) recsize);
    write32(fdes, (uint32_t) 0x0);
    write32(fdes, (uint32_t) 0x0);
    write32(fdes, (uint32_t) 0xFFFFFFFF);
    write32(fdes, (uint32_t) 0xFFFFFFFF);
  }
  return recsize;
}

#if 0
static int WriteSetWorldTransform(FILE * fdes, uint32_t height)
{
  int recsize = sizeof(uint32_t) * 8;
  float fHeight;

  if (fdes != NULL) {
    float s1 = (float)(1.0 / SCALE);
    float s2 = (float)(1.0 / SCALE);
    uint32_t t1;
    uint32_t t2;
    /* conversion to float */
    fHeight = (float)height;
    /* binary copy for serialization */
    memcpy((void *)&height, (void *)&fHeight, sizeof(uint32_t));
    memcpy((void *)&t1, (void *)&s1, sizeof(uint32_t));
    memcpy((void *)&t2, (void *)&s2, sizeof(uint32_t));

    write32(fdes, ENMT_SETWORLDTRANSFORM);
    write32(fdes, (uint32_t) recsize);
    write32(fdes, (uint32_t) t1);
    write32(fdes, (uint32_t) 0x0);
    write32(fdes, (uint32_t) 0x0);
    write32(fdes, (uint32_t) t2);
    write32(fdes, (uint32_t) 0x0);
    write32(fdes, (uint32_t) 0x0);
  }
  return recsize;
}
#endif /* 0 */

static int WriteCreateSolidPen(FILE * fdes, int hndNum, uint32_t colref)
{
  int recsize = sizeof(uint32_t) * 7;

  if (fdes != NULL) {
    write32(fdes, ENMT_CREATEPEN);
    write32(fdes, (uint32_t) recsize);
    write32(fdes, (uint32_t) hndNum);
    write32(fdes, (uint32_t) 0x0);  /* solid pen style */
    write32(fdes, (uint32_t) 0x0);  /* 1 pixel ...  */
    write32(fdes, (uint32_t) 0x0);  /* ... pen size */
    write32(fdes, (uint32_t) colref);
  }
  return recsize;
}

static int WriteCreateSolidBrush(FILE * fdes, int hndNum, uint32_t colref)
{
  int recsize = sizeof(uint32_t) * 6;

  if (fdes != NULL) {
    write32(fdes, ENMT_CREATEBRUSHINDIRECT);
    write32(fdes, (uint32_t) recsize);
    write32(fdes, (uint32_t) hndNum);
    write32(fdes, (uint32_t) 0x0);  /* solid brush style */
    write32(fdes, (uint32_t) colref);
    write32(fdes, (uint32_t) 0x0);  /* ignored when solid */
  }
  return recsize;
}

static int WriteSelectObject(FILE * fdes, int hndNum)
{
  int recsize = sizeof(uint32_t) * 3;

  if (fdes != NULL) {
    write32(fdes, ENMT_SELECTOBJECT);
    write32(fdes, (uint32_t) recsize);
    write32(fdes, (uint32_t) hndNum);
  }
  return recsize;
}

/* SPH: Added 10/14/04 to prevent resource overflow when large number of colors used */
static int WriteDeleteObject(FILE * fdes, int hndNum)
{
  int recsize = sizeof(uint32_t) * 3;

  if (fdes != NULL) {
    write32(fdes, ENMT_DELETEOBJECT);
    write32(fdes, (uint32_t) recsize);
    write32(fdes, (uint32_t) hndNum);
  }
  return recsize;
}

static int WriteEndOfMetafile(FILE * fdes)
{
  int recsize = sizeof(uint32_t) * 5;

  if (fdes != NULL) {
    write32(fdes, ENMT_EOF);
    write32(fdes, (uint32_t) recsize);
    write32(fdes, (uint32_t) 0);
    write32(fdes, (uint32_t) recsize - sizeof(uint32_t));
    write32(fdes, (uint32_t) recsize);

  }
  return recsize;
}

static int WriteHeader(FILE * fdes, gchar * name, int width, int height, int fsize, int nrec, int nhand)
{
  int i, recsize;
  size_t desclen;
  const char *editor = at_version(TRUE);

  desclen = (strlen(editor) + strlen(name) + 3);
  recsize = sizeof(uint32_t) * 25 + (desclen * 2) + ((desclen * 2) % 4);

  if (fdes != NULL) {
    write32(fdes, ENMT_HEADER);
    write32(fdes, (uint32_t) recsize);
    /* pixel bounds */
    write32(fdes, (uint32_t) 0);
    write32(fdes, (uint32_t) 0);
    write32(fdes, (uint32_t) width);
    write32(fdes, (uint32_t) height);
    /* millimeter bounds */
    write32(fdes, (uint32_t) 0);
    write32(fdes, (uint32_t) 0);
    write32(fdes, (uint32_t) width * WDEVMLMTR * 100 / WDEVPIXEL);
    write32(fdes, (uint32_t) height * HDEVMLMTR * 100 / HDEVPIXEL);
    /* signature " EMF" */
    write32(fdes, (uint32_t) 0x464D4520);
    /* current version */
    write32(fdes, (uint32_t) 0x00010000);
    /* file size */
    write32(fdes, (uint32_t) fsize);
    /* number of records */
    write32(fdes, (uint32_t) nrec);
    /* number of handles */
    write16(fdes, (uint16_t) nhand);
    /* reserved */
    write16(fdes, (uint16_t) 0);
    /* size of description */
    write32(fdes, (uint32_t) desclen);
    /* description offset */
    write32(fdes, (uint32_t) 100);
    /* palette entries */
    write32(fdes, (uint32_t) 0);
    /* device width & height in pixel & millimeters */
    write32(fdes, (uint32_t) WDEVPIXEL);
    write32(fdes, (uint32_t) HDEVPIXEL);
    write32(fdes, (uint32_t) WDEVMLMTR);
    write32(fdes, (uint32_t) HDEVMLMTR);
    /* pixel format & opengl (not used) */
    write32(fdes, (uint32_t) 0);
    write32(fdes, (uint32_t) 0);
    write32(fdes, (uint32_t) 0);
    /* description string in Unicode */
    for (i = 0; editor[i]; i++) {
      write16(fdes, ((uint16_t) (editor[i] & 0x07F)));
    }
    write16(fdes, (uint16_t) 0);
    for (i = 0; name[i]; i++) {
      write16(fdes, ((uint16_t) (name[i] & 0x07F)));
    }
    write32(fdes, (uint32_t) 0);
    if ((desclen * 2) % 4)
      write16(fdes, (uint16_t) 0);
  }
  return recsize;
}

/* EMF stats collector */

/* SPH: Added 10/14/04
        Replaced the next two routine with my own versions, which write out each spline as a separate
        vector. (Original version combined all adjacent splines of SAME color into single vector.)
        Also, calls are made to DeleteObject (added WriteDeleteObject) whenever a pen/brush
        is deselected from the Metafile device context. This avoids resource depletion
*/
/*
static void GetEmfStats(EMFStats *stats, gchar* name, spline_list_array_type shape)
{
  unsigned int i, j;
  int ncolors = 0;
  int ncolorchng = 0;
  int nrecords = 0;
  int filesize = 0;
  uint32_t last_color = 0xFFFFFFFF, curr_color;
  spline_list_type curr_list;
  spline_type curr_spline;
  int last_degree;
  int nlines;

  // visit each spline-list
  for(i=0; i<SPLINE_LIST_ARRAY_LENGTH(shape); i++)
    {
      curr_list = SPLINE_LIST_ARRAY_ELT(shape, i);
      curr_color = MAKE_COLREF(curr_list.color.r,curr_list.color.g,curr_list.color.b);
      if(i == 0 || curr_color != last_color)
        {
          ncolorchng++;
          if(!SearchColor(color_list, curr_color))
            {
              ncolors++;
              AddColor(&color_list, curr_color);
            }
          last_color = curr_color;
          // emf stats :: BeginPath + EndPath + FillPath
          nrecords += 3;
          filesize += WriteBeginPath(NULL) + WriteEndPath(NULL) + WriteFillPath(NULL);
        }
      // emf stats :: MoveTo
      nrecords ++;
      filesize += WriteMoveTo(NULL,NULL);
      // visit each spline
	  j = 0;
      last_degree = -1;
      // the outer loop iterates through spline
      // groups of the same degree
      while(j<SPLINE_LIST_LENGTH(curr_list))
        {
          nlines = 0;
          curr_spline = SPLINE_LIST_ELT(curr_list, j);
          last_degree = ((int)SPLINE_DEGREE(curr_spline));

          // the inner loop iterates through lists
          // of spline having the same degree
          while(last_degree == ((int)SPLINE_DEGREE(curr_spline)))
            {
              nlines++;
              j++;
              if(j>=SPLINE_LIST_LENGTH(curr_list))
                break;
              curr_spline = SPLINE_LIST_ELT(curr_list, j);
            }
          switch((polynomial_degree)last_degree)
            {
              case LINEARTYPE:
                //emf stats :: PolyLineTo
                nrecords += nlines;
                filesize += MyWritePolyLineTo(NULL, NULL, nlines);
                break;
              default:
                // emf stats :: PolyBezierTo
                nrecords++;
                filesize += WritePolyBezierTo16(NULL, NULL, nlines);
                break;
            }
        }
    }

  // emf stats :: CreateSolidPen & CreateSolidBrush
  nrecords += ncolors * 2;
  filesize += (WriteCreateSolidPen(NULL, 0, 0) + WriteCreateSolidBrush(NULL, 0, 0)) * ncolors;

  // emf stats :: SelectObject
  nrecords += ncolorchng * 2;
  filesize += WriteSelectObject(NULL, 0) * ncolorchng * 2;
  // emf stats :: header + footer
  nrecords += 2;
  filesize += WriteEndOfMetafile(NULL) + WriteHeader(NULL, name, 0, 0, 0, 0, 0);

  // emf stats :: SetPolyFillMode
  nrecords++;
  filesize += WriteSetPolyFillMode(NULL);

  stats->ncolors  = ncolors;
  stats->nrecords = nrecords;
  stats->filesize = filesize;

  // convert the color list into a color table
  ColorListToColorTable(&color_list, &color_table, ncolors);
}

static void OutputEmf(FILE* fdes, EMFStats *stats, gchar* name, int width, int height, spline_list_array_type shape)
{
  unsigned int i, j;
  int color_index;
  uint32_t last_color = 0xFFFFFFFF, curr_color;
  spline_list_type curr_list;
  spline_type curr_spline;
  int last_degree;
  int nlines;

  //output EMF header
  WriteHeader(fdes, name, width, height, stats->filesize, stats->nrecords, (stats->ncolors * 2) +1);

  y_offset = SCALE * height;
  // output pens & brushes
  for(i=0; i<(unsigned int) stats->ncolors; i++)
  {
    WriteCreateSolidPen(fdes, MK_PEN(i), color_table[i]);
    WriteCreateSolidBrush(fdes, MK_BRUSH(i), color_table[i]);
  }

  // output fill mode
  WriteSetPolyFillMode(fdes);

  // visit each spline-list
  for(i=0; i<SPLINE_LIST_ARRAY_LENGTH(shape); i++)
  {
    curr_list = SPLINE_LIST_ARRAY_ELT(shape, i);

    // output pen & brush selection
    curr_color = MAKE_COLREF(curr_list.color.r,curr_list.color.g,curr_list.color.b);
    if(i == 0 || curr_color != last_color)
    {
      if (i > 0)
        {
          //output EndPath
	      WriteEndPath(fdes);

	      if (shape.centerline)
	        //output StrokePath
	        WriteStrokePath(fdes);
	      else
	        //output StrokePath
	        WriteFillPath(fdes);
        }
      //output a BeginPath for current shape
      WriteBeginPath(fdes);

      color_index = ColorLookUp(curr_color, color_table, stats->ncolors);
      if (shape.centerline)
        WriteSelectObject(fdes, MK_PEN(color_index));
      else
        WriteSelectObject(fdes, STOCK_NULL_PEN);
      WriteSelectObject(fdes, MK_BRUSH(color_index));
      last_color = curr_color;
    }
    //output MoveTo first point
    curr_spline = SPLINE_LIST_ELT(curr_list, 0);
    WriteMoveTo(fdes, &(START_POINT(curr_spline)));

    //visit each spline
    j = 0;
    //the outer loop iterates through spline
    //groups of the same degree
    while (j<SPLINE_LIST_LENGTH(curr_list))
    {
      nlines = 0;
      curr_spline = SPLINE_LIST_ELT(curr_list, j);
      last_degree = ((int)SPLINE_DEGREE(curr_spline));

      //the inner loop iterates through lists
      //of spline having the same degree
      while(last_degree == ((int)SPLINE_DEGREE(curr_spline)))
      {
        nlines++;
        j++;
        if(j>=SPLINE_LIST_LENGTH(curr_list))
          break;
        curr_spline = SPLINE_LIST_ELT(curr_list, j);
      }
      switch((polynomial_degree)last_degree)
      {
        case LINEARTYPE:
          //output PolyLineTo
          MyWritePolyLineTo(fdes, &(SPLINE_LIST_ELT(curr_list, j - nlines)), nlines);
          break;
        default:
          //output PolyBezierTo
          WritePolyBezierTo16(fdes, &(SPLINE_LIST_ELT(curr_list, j - nlines)), nlines);
          break;
      }
    }
  }
  if (SPLINE_LIST_ARRAY_LENGTH(shape) > 0)
    {
      //output EndPath
	  WriteEndPath(fdes);

	  if (shape.centerline)
	    //output StrokePath
	    WriteStrokePath(fdes);
	  else
	    //output StrokePath
	    WriteFillPath(fdes);
    }

  //output EndOfMetafile
  WriteEndOfMetafile(fdes);

  //delete color table
  free((void *)color_table);
}

*/

static void GetEmfStats(EMFStats * stats, gchar * name, spline_list_array_type shape)
{
  unsigned int this_list, this_spline;
  int ncolors = 0;
  int ncolorchng = 0;
  int nrecords = 0;
  int filesize = 0;
  uint32_t last_color = 0xFFFFFFFF, curr_color;
  spline_list_type curr_list;
  spline_type curr_spline;
  int last_degree;
  int nlines;

  // visit each spline-list
  for (this_list = 0; this_list < SPLINE_LIST_ARRAY_LENGTH(shape); this_list++) {
    curr_list = SPLINE_LIST_ARRAY_ELT(shape, this_list);
    curr_color = MAKE_COLREF(curr_list.color.r, curr_list.color.g, curr_list.color.b);
    if (this_list == 0 || curr_color != last_color) {
      ncolorchng++;
      if (!SearchColor(color_list, curr_color)) {
        ncolors++;
        AddColor(&color_list, curr_color);
      }
      if (shape.centerline) {
        nrecords += 2;
        filesize += (WriteCreateSolidPen(NULL, 0, 0) + WriteSelectObject(NULL, 0));
        if (this_list != 0) {
          nrecords++;
          filesize += WriteDeleteObject(NULL, 0);
        }
      } else {
        nrecords++;
        filesize += WriteSelectObject(NULL, 0);
      }

      nrecords += 2;
      filesize += (WriteCreateSolidBrush(NULL, 0, 0) + WriteSelectObject(NULL, 0));
      if (this_list != 0) {
        nrecords++;
        filesize += WriteDeleteObject(NULL, 0);
      }

      last_color = curr_color;

    }
    // emf stats :: BeginPath
    nrecords += 1;
    filesize += WriteBeginPath(NULL);
    // emf stats :: MoveTo
    nrecords++;
    filesize += WriteMoveTo(NULL, NULL);
    // visit each spline
    this_spline = 0;
    last_degree = -1;
    // the outer loop iterates through spline
    // groups of the same degree
    while (this_spline < SPLINE_LIST_LENGTH(curr_list)) {
      nlines = 0;
      curr_spline = SPLINE_LIST_ELT(curr_list, this_spline);
      last_degree = ((int)SPLINE_DEGREE(curr_spline));

      // the inner loop iterates through lists
      // of spline having the same degree
      while (last_degree == ((int)SPLINE_DEGREE(curr_spline))) {
        nlines++;
        this_spline++;
        if (this_spline >= SPLINE_LIST_LENGTH(curr_list))
          break;
        curr_spline = SPLINE_LIST_ELT(curr_list, this_spline);
      }
      switch ((polynomial_degree) last_degree) {
      case LINEARTYPE:
        //emf stats :: PolyLineTo
        nrecords += nlines;
        filesize += MyWritePolyLineTo(NULL, NULL, nlines);
        break;
      default:
        // emf stats :: PolyBezierTo
        nrecords++;
        filesize += WritePolyBezierTo16(NULL, NULL, nlines);
        break;
      }
    }

    // emf stats :: EndPath + FillPath
    nrecords += 2;
    filesize += WriteEndPath(NULL) + WriteFillPath(NULL);

  }

  // cleanup Pen and Brush
  if (shape.centerline) {
    nrecords++;
    filesize += WriteDeleteObject(NULL, 0);
  }

  nrecords++;
  filesize += WriteDeleteObject(NULL, 0);

  // emf stats :: header + footer
  nrecords += 2;
  filesize += WriteEndOfMetafile(NULL) + WriteHeader(NULL, name, 0, 0, 0, 0, 0);

  // emf stats :: SetPolyFillMode
  nrecords++;
  filesize += WriteSetPolyFillMode(NULL);

  stats->ncolors = ncolors;
  stats->nrecords = nrecords;
  stats->filesize = filesize;

  // convert the color list into a color table
  ColorListToColorTable(&color_list, &color_table, ncolors);
}

//EMF output

static void OutputEmf(FILE * fdes, EMFStats * stats, gchar * name, int width, int height, spline_list_array_type shape)
{
  unsigned int this_list, this_spline;
  int color_index, last_index;
  uint32_t last_color = 0xFFFFFFFF, curr_color;
  spline_list_type curr_list;
  spline_type curr_spline;
  int last_degree;
  int nlines;

  //output EMF header
  WriteHeader(fdes, name, width, height, stats->filesize, stats->nrecords, (stats->ncolors * 2) + 1);

  y_offset = SCALE * height;

  // output fill mode
  WriteSetPolyFillMode(fdes);

  // visit each spline-list
  for (this_list = 0; this_list < SPLINE_LIST_ARRAY_LENGTH(shape); this_list++) {
    curr_list = SPLINE_LIST_ARRAY_ELT(shape, this_list);

    // output pen & brush selection
    curr_color = MAKE_COLREF(curr_list.color.r, curr_list.color.g, curr_list.color.b);
    if (this_list == 0 || curr_color != last_color) {

      color_index = ColorLookUp(curr_color, color_table, stats->ncolors);
      if (shape.centerline) {
        WriteCreateSolidPen(fdes, MK_PEN(color_index), color_table[color_index]);
        WriteSelectObject(fdes, MK_PEN(color_index));
        if (this_list != 0)
          WriteDeleteObject(fdes, MK_PEN(last_index));
      } else
        WriteSelectObject(fdes, STOCK_NULL_PEN);

      WriteCreateSolidBrush(fdes, MK_BRUSH(color_index), color_table[color_index]);
      WriteSelectObject(fdes, MK_BRUSH(color_index));
      if (this_list != 0)
        WriteDeleteObject(fdes, MK_BRUSH(last_index));

      last_color = curr_color;
      last_index = color_index;
    }
    //output a BeginPath for current shape
    WriteBeginPath(fdes);

    //output MoveTo first point
    curr_spline = SPLINE_LIST_ELT(curr_list, 0);
    WriteMoveTo(fdes, &(START_POINT(curr_spline)));

    //visit each spline
    this_spline = 0;
    //the outer loop iterates through spline
    //groups of the same degree
    while (this_spline < SPLINE_LIST_LENGTH(curr_list)) {
      nlines = 0;
      curr_spline = SPLINE_LIST_ELT(curr_list, this_spline);
      last_degree = ((int)SPLINE_DEGREE(curr_spline));

      //the inner loop iterates through lists
      //of spline having the same degree
      while (last_degree == ((int)SPLINE_DEGREE(curr_spline))) {
        nlines++;
        this_spline++;
        if (this_spline >= SPLINE_LIST_LENGTH(curr_list))
          break;
        curr_spline = SPLINE_LIST_ELT(curr_list, this_spline);
      }
      switch ((polynomial_degree) last_degree) {
      case LINEARTYPE:
        //output PolyLineTo
        MyWritePolyLineTo(fdes, &(SPLINE_LIST_ELT(curr_list, this_spline - nlines)), nlines);
        break;
      default:
        //output PolyBezierTo
        WritePolyBezierTo16(fdes, &(SPLINE_LIST_ELT(curr_list, this_spline - nlines)), nlines);
        break;
      }
    }

    //output EndPath
    WriteEndPath(fdes);

    if (shape.centerline || curr_list.open)
      //output StrokePath
      WriteStrokePath(fdes);
    else
      //output StrokePath
      WriteFillPath(fdes);

  }

  //cleanup DC
  if (shape.centerline)
    WriteDeleteObject(fdes, MK_PEN(last_index));
  WriteDeleteObject(fdes, MK_BRUSH(last_index));

  //output EndOfMetafile
  WriteEndOfMetafile(fdes);

  //delete color table
  free((void *)color_table);
}

int output_emf_writer(FILE * file, gchar * name, int llx, int lly, int urx, int ury, at_output_opts_type * opts, spline_list_array_type shape, at_msg_func msg_func, gpointer msg_data, gpointer user_data)
{
  EMFStats stats;

#ifdef _WINDOWS
  if (file == stdout) {
    fprintf(stderr, "This driver couldn't write to stdout!\n");
    return -1;
  }
#endif

  /* Get EMF stats */
  GetEmfStats(&stats, name, shape);

  /* Output EMF */
  OutputEmf(file, &stats, name, urx, ury, shape);

  return 0;
}
