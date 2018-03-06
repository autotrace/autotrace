/* output-dr2d.c: output in DR2D format

   Copyright (C) 2002 Andrew Elia

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

#include <stdlib.h>
#include <string.h>

#include "spline.h"
#include "color.h"
#include "output-dr2d.h"

/* Globals: Values are set by output_dr2d_writer() */
float XFactor;
float YFactor;
float LineThickness;

#define FIXOFFS 10

#define LF_ACTIVE 0x01
#define LF_DISPLAYED 0x02

#define FT_NONE 0
#define FT_COLOR 1

#define JT_NONE 0
#define JT_MITER 1
#define JT_BEVEL 2
#define JT_ROUND 3

#define INDICATOR 0xFFFFFFFF
#define IND_SPLINE 0x00000001
#define IND_MOVETO 0x00000002

struct Chunk {
  char ID[4];
  unsigned int Size;
  unsigned char *Data;
};

static struct Chunk *BuildDRHD(int, int, int, int);
static struct Chunk *BuildPPRF(char *, int, char *, float);
static struct Chunk *BuildCMAP(spline_list_array_type);
static struct Chunk *BuildLAYR(void);
static struct Chunk *BuildDASH(void);
static struct Chunk *BuildBBOX(spline_list_type, int);
static struct Chunk *BuildATTR(at_color, int, struct Chunk *);
static int GetCMAPEntry(at_color, struct Chunk *);
static int CountSplines(spline_list_type);
static int SizeFloat(float, char *);
static void ShortAsBytes(int, unsigned char *);
static void IntAsBytes(int, unsigned char *);
static void FloatAsIEEEBytes(float, unsigned char *);
/* static void ieee2flt(long *, float *); */
static void flt2ieee(float *, unsigned char *);
static void FreeChunk(struct Chunk *);
static void FreeChunks(struct Chunk **, int);
static int TotalSizeChunks(struct Chunk **, int);
static int SizeChunk(struct Chunk *);
static void PushPolyPoint(unsigned char *, int *, float, float);
static void PushPolyIndicator(unsigned char *, int *, unsigned int);
static struct Chunk **GeneratexPLY(struct Chunk *, spline_list_array_type, int);

static struct Chunk *BuildCMAP(spline_list_array_type shape)
{
  unsigned this_list;
  unsigned this_list_length;
  int ListSize, MaxListSize;
  int WalkCol, FoundCol;
  unsigned char Red, Green, Blue;
  unsigned char *CMAP;
  unsigned char *IndexCol;
  struct Chunk *CMAPChunk;

  MaxListSize = SPLINE_LIST_ARRAY_LENGTH(shape);

  if ((CMAPChunk = (struct Chunk *)malloc(sizeof(struct Chunk))) == NULL) {
    fprintf(stderr, "Insufficient memory to allocate CMAP chunk\n");
    return NULL;
  }

  if ((CMAP = (unsigned char *)malloc(MaxListSize * 3)) == NULL) {
    fprintf(stderr, "Insufficient memory to allocate colour map (size %d)\n", MaxListSize);
    free(CMAPChunk);
    return NULL;
  }

  ListSize = 0;
  this_list_length = SPLINE_LIST_ARRAY_LENGTH(shape);
  for (this_list = 0; this_list < this_list_length; this_list++) {
    spline_list_type list = SPLINE_LIST_ARRAY_ELT(shape, this_list);
    at_color curr_color = (list.clockwise && shape.background_color != NULL) ? *(shape.background_color) : list.color;

    Red = curr_color.r;
    Green = curr_color.g;
    Blue = curr_color.b;

    FoundCol = 0;
    for (WalkCol = 0; WalkCol < ListSize; WalkCol++) {
      IndexCol = CMAP + (WalkCol * 3);
      if ((*IndexCol == Red) && (*(IndexCol + 1) == Green) && (*(IndexCol + 2) == Blue)) {
        FoundCol = 1;
        break;
      }
    }

    if (FoundCol == 0) {
      IndexCol = CMAP + (ListSize * 3);
      *IndexCol = Red;
      *(IndexCol + 1) = Green;
      *(IndexCol + 2) = Blue;
      ++ListSize;
    }
  }

  strncpy(CMAPChunk->ID, "CMAP", 4);
  CMAPChunk->Size = ListSize * 3;
  CMAPChunk->Data = CMAP;

  return CMAPChunk;
}

static int GetCMAPEntry(at_color colour, struct Chunk *CMAPChunk)
{
  int WalkCol, ListSize;
  unsigned char Red, Green, Blue;
  unsigned char *IndexCol;
  unsigned char *CMAPTable;

  ListSize = (CMAPChunk->Size) / 3;
  CMAPTable = CMAPChunk->Data;

  Red = colour.r;
  Green = colour.g;
  Blue = colour.b;

  for (WalkCol = 0; WalkCol < ListSize; WalkCol++) {
    IndexCol = CMAPTable + (WalkCol * 3);
    if ((*IndexCol == Red) && (*(IndexCol + 1) == Green) && (*(IndexCol + 2) == Blue)) {
      return WalkCol;
    }
  }

  return -1;
}

static struct Chunk *BuildBBOX(spline_list_type list, int height)
{
  unsigned this_spline;
  unsigned this_spline_length;
  float x1, y1, x2, y2;
  float ex, ey;
  struct Chunk *BBOXChunk;
  unsigned char *BBOXData;
  spline_type s;

  if ((BBOXChunk = (struct Chunk *)malloc(sizeof(struct Chunk))) == NULL) {
    fprintf(stderr, "Insufficient memory to allocate BBOX chunk\n");
    return NULL;
  }

  if ((BBOXData = (unsigned char *)malloc(16)) == NULL) {
    fprintf(stderr, "Insufficient memory to allocate BBOX data\n");
    free(BBOXChunk);
    return NULL;
  }

  s = SPLINE_LIST_ELT(list, 0);

  x1 = START_POINT(s).x;
  y1 = START_POINT(s).y;
  x2 = START_POINT(s).x;
  y2 = START_POINT(s).y;

  this_spline_length = SPLINE_LIST_LENGTH(list);
  for (this_spline = 0; this_spline < this_spline_length; this_spline++) {
    s = SPLINE_LIST_ELT(list, this_spline);
    ex = END_POINT(s).x;
    ey = height - END_POINT(s).y;

    if (x1 > ex) {
      x1 = ex;
    }

    if (y1 > ey) {
      y1 = ey;
    }

    if (x2 < ex) {
      x2 = ex;
    }

    if (y2 < ey) {
      y2 = ey;
    }
  }

  FloatAsIEEEBytes(x1 * XFactor, BBOXData);
  FloatAsIEEEBytes(y1 * YFactor, BBOXData + 4);
  FloatAsIEEEBytes(x2 * XFactor, BBOXData + 8);
  FloatAsIEEEBytes(y2 * YFactor, BBOXData + 12);

  strncpy(BBOXChunk->ID, "BBOX", 4);
  BBOXChunk->Size = 16;
  BBOXChunk->Data = BBOXData;

  return BBOXChunk;
}

static struct Chunk *BuildATTR(at_color colour, int StrokeOrFill, struct Chunk *CMAPChunk)
{
  struct Chunk *ATTRChunk;
  unsigned char *ATTRData;
  int ColourIndex;

  if ((ATTRChunk = (struct Chunk *)malloc(sizeof(struct Chunk))) == NULL) {
    fprintf(stderr, "Insufficient memory to allocate ATTR chunk\n");
    return NULL;
  }

  if ((ATTRData = (unsigned char *)malloc(14)) == NULL) {
    fprintf(stderr, "Insufficient memory to allocate ATTR data\n");
    free(ATTRChunk);
    return NULL;
  }

  ColourIndex = GetCMAPEntry(colour, CMAPChunk);

  ATTRData[0] = (StrokeOrFill) ? FT_NONE : FT_COLOR;
  ATTRData[1] = JT_ROUND;
  ATTRData[2] = 1;
  ATTRData[3] = 0;
  ShortAsBytes(ColourIndex, ATTRData + 4);
  ShortAsBytes(ColourIndex, ATTRData + 6);
  ShortAsBytes(0, ATTRData + 8);
  FloatAsIEEEBytes(LineThickness, ATTRData + 10);

  strncpy(ATTRChunk->ID, "ATTR", 4);
  ATTRChunk->Size = 14;
  ATTRChunk->Data = ATTRData;

  return ATTRChunk;
}

static struct Chunk *BuildDRHD(int x1, int y1, int x2, int y2)
{
  struct Chunk *DRHDChunk;
  unsigned char *DRHDData;

  if ((DRHDChunk = (struct Chunk *)malloc(sizeof(struct Chunk))) == NULL) {
    fprintf(stderr, "Insufficient memory to allocate DRHD chunk\n");
    return NULL;
  }

  if ((DRHDData = (unsigned char *)malloc(16)) == NULL) {
    fprintf(stderr, "Insufficient memory to allocate DRHD data\n");
    free(DRHDChunk);
    return NULL;
  }

  FloatAsIEEEBytes(x1 * XFactor, DRHDData);
  FloatAsIEEEBytes(y1 * YFactor, DRHDData + 4);
  FloatAsIEEEBytes(x2 * XFactor, DRHDData + 8);
  FloatAsIEEEBytes(y2 * YFactor, DRHDData + 12);

  strncpy(DRHDChunk->ID, "DRHD", 4);
  DRHDChunk->Size = 16;
  DRHDChunk->Data = DRHDData;

  return DRHDChunk;
}

static struct Chunk *BuildPPRF(char *Units, int Portrait, char *PageType, float GridSize)
{
  struct Chunk *PPRFChunk;
  char *PPRFData;
  char *PPRFPos;
  int ChunkSize;

  if ((PPRFChunk = (struct Chunk *)malloc(sizeof(struct Chunk))) == NULL) {
    fprintf(stderr, "Insufficient memory to allocate PPRF chunk\n");
    return NULL;
  }

  ChunkSize = strlen("Units=") + strlen(Units) + 1;
  ChunkSize += strlen("Portrait=") + (Portrait ? 4 : 5) + 1;
  ChunkSize += strlen("PageType=") + strlen(PageType) + 1;
  ChunkSize += strlen("GridSize=") + SizeFloat(GridSize, "%f") + 1;

  if ((PPRFData = (char *)malloc(ChunkSize)) == NULL) {
    fprintf(stderr, "Insufficient memory to allocate PPRF data\n");
    free(PPRFChunk);
    return NULL;
  }

  PPRFPos = PPRFData;
  sprintf(PPRFPos, "Units=%s", Units);
  PPRFPos += strlen(PPRFPos) + 1;
  sprintf(PPRFPos, "Portrait=%s", (Portrait ? "True" : "False"));
  PPRFPos += strlen(PPRFPos) + 1;
  sprintf(PPRFPos, "PageType=%s", PageType);
  PPRFPos += strlen(PPRFPos) + 1;
  sprintf(PPRFPos, "GridSize=%f", GridSize);

  strncpy(PPRFChunk->ID, "PPRF", 4);
  PPRFChunk->Size = ChunkSize;
  PPRFChunk->Data = (unsigned char *)PPRFData;

  return PPRFChunk;
}

static struct Chunk *BuildLAYR()
{
  struct Chunk *LAYRChunk;
  unsigned char *LAYRData;

  if ((LAYRChunk = (struct Chunk *)malloc(sizeof(struct Chunk))) == NULL) {
    fprintf(stderr, "Insufficient memory to allocate LAYR chunk\n");
    return NULL;
  }

  if ((LAYRData = (unsigned char *)malloc(20)) == NULL) {
    fprintf(stderr, "Insufficient memory to allocate LAYR data\n");
    free(LAYRChunk);
    return NULL;
  }

  ShortAsBytes(0, LAYRData);
  memset(LAYRData + 2, 0, 16);
  strcpy((char *)(LAYRData + 2), "Default layer");
  *(LAYRData + 18) = LF_ACTIVE | LF_DISPLAYED;
  *(LAYRData + 19) = 0;

  strncpy(LAYRChunk->ID, "LAYR", 4);
  LAYRChunk->Size = 20;
  LAYRChunk->Data = LAYRData;

  return LAYRChunk;
}

static struct Chunk *BuildDASH(void)
{
  struct Chunk *DASHChunk;
  unsigned char *DASHData;

  if ((DASHChunk = (struct Chunk *)malloc(sizeof(struct Chunk))) == NULL) {
    fprintf(stderr, "Insufficient memory to allocate DASH chunk\n");
    return NULL;
  }

  if ((DASHData = (unsigned char *)malloc(4)) == NULL) {
    fprintf(stderr, "Insufficient memory to allocate DASH data\n");
    free(DASHChunk);
    return NULL;
  }

  ShortAsBytes(1, DASHData);
  ShortAsBytes(0, DASHData + 2);

  strncpy(DASHChunk->ID, "DASH", 4);
  DASHChunk->Size = 4;
  DASHChunk->Data = DASHData;

  return DASHChunk;
}

static struct Chunk **GeneratexPLY(struct Chunk *CMAP, spline_list_array_type shape, int height)
{
  unsigned this_list;
  unsigned this_list_length;
  unsigned this_spline;
  unsigned this_spline_length;
  spline_type s;
  struct Chunk **ChunkList;
  struct Chunk *PolyChunk;
  int ListPoint, PolySize, PolyPoint, NumPoints;
  int StrokeOrFill;
  unsigned char *PolyData;

  this_list_length = SPLINE_LIST_ARRAY_LENGTH(shape);

  /* We store three chunks for every spline (one for BBOX, one for ATTR, and one for xPLY) */
  if ((ChunkList = (struct Chunk **)malloc(sizeof(struct Chunk) * (this_list_length * 3))) == NULL) {
    fprintf(stderr, "Insufficient memory to allocate chunk list\n");
    return NULL;
  }

  ListPoint = 0;
  for (this_list = 0; this_list < this_list_length; this_list++) {
    spline_list_type list = SPLINE_LIST_ARRAY_ELT(shape, this_list);
    spline_type first = SPLINE_LIST_ELT(list, 0);
    at_color curr_color = (list.clockwise && shape.background_color != NULL) ? *(shape.background_color) : list.color;

    StrokeOrFill = (shape.centerline || list.open);
    this_spline_length = SPLINE_LIST_LENGTH(list);

    ChunkList[ListPoint++] = BuildBBOX(list, height);
    ChunkList[ListPoint++] = BuildATTR(curr_color, StrokeOrFill, CMAP);

    if ((PolyChunk = (struct Chunk *)malloc(sizeof(struct Chunk))) == NULL) {
      fprintf(stderr, "Insufficient memory to allocate xPLY chunk\n");
      FreeChunks(ChunkList, ListPoint);
      return NULL;
    }

    NumPoints = CountSplines(list);

    /* Store an extra 2 bytes for length header */
    PolySize = (NumPoints << 3) + 2;
    if ((PolyData = (unsigned char *)malloc(PolySize)) == NULL) {
      fprintf(stderr, "Insufficient memory to allocate xPLY data\n");
      free(PolyChunk);
      free(PolyData);
      FreeChunks(ChunkList, ListPoint);
      return NULL;
    }

    ChunkList[ListPoint++] = PolyChunk;
    strncpy(PolyChunk->ID, (StrokeOrFill) ? "OPLY" : "CPLY", 4);
    PolyChunk->Size = PolySize;
    PolyChunk->Data = PolyData;

    ShortAsBytes(NumPoints, PolyData);
    PolyPoint = 2;

    if (SPLINE_DEGREE(first) == LINEARTYPE) {
      PushPolyPoint(PolyData, &PolyPoint, START_POINT(first).x, height - START_POINT(first).y);
    }

    for (this_spline = 0; this_spline < this_spline_length; this_spline++) {
      s = SPLINE_LIST_ELT(list, this_spline);

      if (SPLINE_DEGREE(s) == LINEARTYPE) {
        PushPolyPoint(PolyData, &PolyPoint, END_POINT(s).x, height - END_POINT(s).y);
      } else {
        PushPolyIndicator(PolyData, &PolyPoint, IND_SPLINE);
        PushPolyPoint(PolyData, &PolyPoint, START_POINT(s).x, height - START_POINT(s).y);
        PushPolyPoint(PolyData, &PolyPoint, CONTROL1(s).x, height - CONTROL1(s).y);
        PushPolyPoint(PolyData, &PolyPoint, CONTROL2(s).x, height - CONTROL2(s).y);
        PushPolyPoint(PolyData, &PolyPoint, END_POINT(s).x, height - END_POINT(s).y);
      }
    }
  }

  return ChunkList;
}

static int CountSplines(spline_list_type list)
{
  unsigned this_spline;
  unsigned this_spline_length;
  int Total;

  Total = 0;

  if (SPLINE_DEGREE(SPLINE_LIST_ELT(list, 0)) == LINEARTYPE) {
    ++Total;
  }

  this_spline_length = SPLINE_LIST_LENGTH(list);
  for (this_spline = 0; this_spline < this_spline_length; this_spline++) {
    if (SPLINE_DEGREE(SPLINE_LIST_ELT(list, this_spline)) == LINEARTYPE) {
      ++Total;
    } else {
      Total += 5;
    }
  }

  return Total;
}

static void PushPolyPoint(unsigned char *PolyData, int *PolyPoint, float x, float y)
{
  int PolyLocal;

  PolyLocal = *PolyPoint;

  FloatAsIEEEBytes(x * XFactor, PolyData + PolyLocal);
  PolyLocal += 4;
  FloatAsIEEEBytes(y * YFactor, PolyData + PolyLocal);

  *PolyPoint = PolyLocal + 4;
}

static void PushPolyIndicator(unsigned char *PolyData, int *PolyPoint, unsigned int flags)
{
  int PolyLocal;

  PolyLocal = *PolyPoint;

  IntAsBytes(INDICATOR, PolyData + PolyLocal);
  PolyLocal += 4;
  IntAsBytes(flags, PolyData + PolyLocal);

  *PolyPoint = PolyLocal + 4;
}

static void WriteChunk(FILE * file, struct Chunk *Chunk)
{
  unsigned char SizeBytes[4];
  int Size;

  Size = Chunk->Size;
  IntAsBytes(Size, SizeBytes);

  fwrite(Chunk->ID, 4, 1, file);
  fwrite(SizeBytes, 4, 1, file);
  fwrite(Chunk->Data, Size, 1, file);
  if (Size & 0x01) {
    fprintf(file, "%c", 0);
  }
}

static void WriteChunks(FILE * file, struct Chunk **ChunkList, int NumChunks)
{
  int WalkChunks;

  for (WalkChunks = 0; WalkChunks < NumChunks; WalkChunks++) {
    WriteChunk(file, ChunkList[WalkChunks]);
  }
}

static int TotalSizeChunks(struct Chunk **ChunkList, int NumChunks)
{
  int WalkChunks;
  int Size;
  int Total;

  Total = 0;
  for (WalkChunks = 0; WalkChunks < NumChunks; WalkChunks++) {
    /* 4 bytes for ID and 4 bytes for length */
    Size = ChunkList[WalkChunks]->Size;
    Size += Size & 0x01;
    Total += (Size) + 8;
  }

  return Total;
}

static int SizeChunk(struct Chunk *ThisChunk)
{
  int Size;

  Size = ThisChunk->Size;
  Size += Size & 0x01;

  return Size;
}

static void FreeChunk(struct Chunk *ThisChunk)
{
  free(ThisChunk->Data);
  free(ThisChunk);
}

static void FreeChunks(struct Chunk **ChunkList, int NumChunks)
{
  int WalkChunks;

  for (WalkChunks = 0; WalkChunks < NumChunks; WalkChunks++) {
    FreeChunk(ChunkList[WalkChunks]);
  }
}

int output_dr2d_writer(FILE * file, gchar * name, int llx, int lly, int urx, int ury, at_output_opts_type * opts, spline_list_array_type shape, at_msg_func msg_func, gpointer msg_data, gpointer user_data)
{
  int width = urx - llx;
  int height = ury - lly;
  int NumSplines, FORMSize;
  int Portrait;
  struct Chunk *DRHDChunk;
  struct Chunk *PPRFChunk;
  struct Chunk *LAYRChunk;
  struct Chunk *DASHChunk;
  struct Chunk *CMAPChunk;
  struct Chunk **ChunkList;
  unsigned char SizeBytes[4];

  Portrait = width < height;

  if (Portrait) {
    XFactor = ((float)11.6930 / (float)width) * (1 << FIXOFFS);
    YFactor = XFactor;
  } else {
    YFactor = ((float)8.2681 / (float)height) * (1 << FIXOFFS);
    XFactor = YFactor;
  }

  LineThickness = (float)1.0 / opts->dpi;

  DRHDChunk = BuildDRHD(llx, lly, urx, ury);
  PPRFChunk = BuildPPRF("Inch", Portrait, "A4", 1.0);
  LAYRChunk = BuildLAYR();
  DASHChunk = BuildDASH();
  CMAPChunk = BuildCMAP(shape);

  ChunkList = GeneratexPLY(CMAPChunk, shape, height);

  NumSplines = SPLINE_LIST_ARRAY_LENGTH(shape) * 3;
  FORMSize = 4 + (SizeChunk(DRHDChunk) + 8) + (SizeChunk(PPRFChunk) + 8) + (SizeChunk(LAYRChunk) + 8) + (SizeChunk(DASHChunk) + 8) + (SizeChunk(CMAPChunk) + 8) + TotalSizeChunks(ChunkList, NumSplines);

  IntAsBytes(FORMSize, SizeBytes);
  fprintf(file, "FORM");
  fwrite(SizeBytes, 4, 1, file);
  fprintf(file, "DR2D");

  WriteChunk(file, DRHDChunk);
  FreeChunk(DRHDChunk);
  WriteChunk(file, PPRFChunk);
  FreeChunk(PPRFChunk);
  WriteChunk(file, LAYRChunk);
  FreeChunk(LAYRChunk);
  WriteChunk(file, DASHChunk);
  FreeChunk(DASHChunk);
  WriteChunk(file, CMAPChunk);
  FreeChunk(CMAPChunk);
  WriteChunks(file, ChunkList, NumSplines);
  FreeChunks(ChunkList, NumSplines);

  return 0;
}

static int SizeFloat(float f, char *Format)
{
  char FloatString[100];

  return (sprintf(FloatString, Format, f));
}

static void IntAsBytes(int value, unsigned char *bytes)
{
  *bytes = (unsigned char)((value >> 24) & 0xFF);
  *(bytes + 1) = (unsigned char)((value >> 16) & 0xFF);
  *(bytes + 2) = (unsigned char)((value >> 8) & 0xFF);
  *(bytes + 3) = (unsigned char)(value & 0xFF);
}

static void ShortAsBytes(int value, unsigned char *bytes)
{
  *(bytes + 0) = (unsigned char)((value >> 8) & 0xFF);
  *(bytes + 1) = (unsigned char)(value & 0xFF);
}

static void FloatAsIEEEBytes(float value, unsigned char *bytes)
{
  flt2ieee(&value, bytes);
}

static void flt2ieee(float *flt, unsigned char *bytes)
{
  long RealMant, RealMask, RealExp;
  long MoveExp;

  RealMant = (long)*flt;

  *bytes = 0;
  *(bytes + 1) = 0;
  *(bytes + 2) = 0;
  *(bytes + 3) = 0;

  if (RealMant) {
    if (RealMant < 0) {
      *bytes |= 0x80;
      RealMant = -RealMant;
    }

    for (RealMask = 0x40000000, RealExp = 31; RealMask; RealMask >>= 1, RealExp--) {
      if (RealMant & RealMask) {
        break;
      }
    }

    if (RealExp > 24) {
      RealMant >>= RealExp - 24;
    } else {
      RealMant <<= 24 - RealExp;
    }
    RealExp -= FIXOFFS;
    RealExp += 126;

    MoveExp = RealExp << 23;
    *bytes |= (MoveExp >> 24) & 0x7F;
    *(bytes + 1) |= ((MoveExp >> 16) & 0x80) | ((RealMant >> 16) & 0x7F);
    *(bytes + 2) |= (RealMant >> 8) & 0xFF;
    *(bytes + 3) |= RealMant & 0xFF;
  }
}
