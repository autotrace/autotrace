/* output-ild.c --- output in ILDA (ild) format

   Copyright (C) 2005 Arjan van Vught
             (C) 2005 Robin Adams <radams@linux-laser.org>

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

#undef ILD_DEBUG
#undef ANCHOR_DEBUG

#include <fcntl.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "spline.h"
#include "xstd.h"

#ifdef MAX
#undef MAX
#endif
#define MAX(a,b)			( (a) > (b) ? (a) : (b) )

#define _USE_MATH_DEFINES
#include <math.h>

#define ILDA_3D_DATA  0
#define ILDA_2D_DATA  1
#define ILDA_COLOR_TABLE 2
#define ILDA_TRUE_COLOR 3
#define ILDA_COLORS_NUM 256

#define POINT_ATTRIB_BLANKED 0x01

int write3DFrames = 0;
int trueColorWrite = 1;
int writeTable = 0;
int fromToZero = 1;
int insert_anchor_points = 1;

int lineDistance = 800;
int blankDistance = 1200;
int anchor_thresh = 40;

int inserted_anchor_points = 0;

typedef struct tagLaserPoint {
  void *next;
  short int x;
  short int y;
  short int z;
  unsigned char attrib;
  unsigned char r;
  unsigned char g;
  unsigned char b;
} LaserPoint;

typedef LaserPoint *pLaserPoint;

typedef struct tagLaserFrame {
  void *next;
  void *previous;
  LaserPoint *point_first;
  LaserPoint *point_last;
  int count;
  char *name;
} LaserFrame;

typedef LaserFrame *pLaserFrame;

typedef struct tagLaserSequence {
  LaserFrame *frame_first;
  LaserFrame *frame_last;
  int frame_count;
} LaserSequence;

typedef LaserSequence *pLaserSequence;

pLaserFrame drawframe = NULL;
pLaserSequence drawsequence = NULL;
static unsigned char ilda[4] = { 'I', 'L', 'D', 'A' };

// ILDA standard color palette
static unsigned char ilda_standard_color_palette[256][3] = {
  {0, 0, 0},                    // Black/blanked (fixed)
  {255, 255, 255},              // White (fixed)
  {255, 0, 0},                  // Red (fixed)
  {255, 255, 0},                // Yellow (fixed)
  {0, 255, 0},                  // Green (fixed)
  {0, 255, 255},                // Cyan (fixed)
  {0, 0, 255},                  // Blue (fixed)
  {255, 0, 255},                // Magenta (fixed)
  {255, 128, 128},              // Light red
  {255, 140, 128},
  {255, 151, 128},
  {255, 163, 128},
  {255, 174, 128},
  {255, 186, 128},
  {255, 197, 128},
  {255, 209, 128},
  {255, 220, 128},
  {255, 232, 128},
  {255, 243, 128},
  {255, 255, 128},              // Light yellow
  {243, 255, 128},
  {232, 255, 128},
  {220, 255, 128},
  {209, 255, 128},
  {197, 255, 128},
  {186, 255, 128},
  {174, 255, 128},
  {163, 255, 128},
  {151, 255, 128},
  {140, 255, 128},
  {128, 255, 128},              // Light green
  {128, 255, 140},
  {128, 255, 151},
  {128, 255, 163},
  {128, 255, 174},
  {128, 255, 186},
  {128, 255, 197},
  {128, 255, 209},
  {128, 255, 220},
  {128, 255, 232},
  {128, 255, 243},
  {128, 255, 255},              // Light cyan
  {128, 243, 255},
  {128, 232, 255},
  {128, 220, 255},
  {128, 209, 255},
  {128, 197, 255},
  {128, 186, 255},
  {128, 174, 255},
  {128, 163, 255},
  {128, 151, 255},
  {128, 140, 255},
  {128, 128, 255},              // Light blue
  {140, 128, 255},
  {151, 128, 255},
  {163, 128, 255},
  {174, 128, 255},
  {186, 128, 255},
  {197, 128, 255},
  {209, 128, 255},
  {220, 128, 255},
  {232, 128, 255},
  {243, 128, 255},
  {255, 128, 255},              // Light magenta
  {255, 128, 243},
  {255, 128, 232},
  {255, 128, 220},
  {255, 128, 209},
  {255, 128, 197},
  {255, 128, 186},
  {255, 128, 174},
  {255, 128, 163},
  {255, 128, 151},
  {255, 128, 140},
  {255, 0, 0},                  // Red (cycleable)
  {255, 23, 0},
  {255, 46, 0},
  {255, 70, 0},
  {255, 93, 0},
  {255, 116, 0},
  {255, 139, 0},
  {255, 162, 0},
  {255, 185, 0},
  {255, 209, 0},
  {255, 232, 0},
  {255, 255, 0},                //Yellow (cycleable)
  {232, 255, 0},
  {209, 255, 0},
  {185, 255, 0},
  {162, 255, 0},
  {139, 255, 0},
  {116, 255, 0},
  {93, 255, 0},
  {70, 255, 0},
  {46, 255, 0},
  {23, 255, 0},
  {0, 255, 0},                  // Green (cycleable)
  {0, 255, 23},
  {0, 255, 46},
  {0, 255, 70},
  {0, 255, 93},
  {0, 255, 116},
  {0, 255, 139},
  {0, 255, 162},
  {0, 255, 185},
  {0, 255, 209},
  {0, 255, 232},
  {0, 255, 255},                // Cyan (cycleable)
  {0, 232, 255},
  {0, 209, 255},
  {0, 185, 255},
  {0, 162, 255},
  {0, 139, 255},
  {0, 116, 255},
  {0, 93, 255},
  {0, 70, 255},
  {0, 46, 255},
  {0, 23, 255},
  {0, 0, 255},                  // Blue (cycleable)
  {23, 0, 255},
  {46, 0, 255},
  {70, 0, 255},
  {93, 0, 255},
  {116, 0, 255},
  {139, 0, 255},
  {162, 0, 255},
  {185, 0, 255},
  {209, 0, 255},
  {232, 0, 255},
  {255, 0, 255},                // Magenta (cycleable)
  {255, 0, 232},
  {255, 0, 209},
  {255, 0, 185},
  {255, 0, 162},
  {255, 0, 139},
  {255, 0, 116},
  {255, 0, 93},
  {255, 0, 70},
  {255, 0, 46},
  {255, 0, 23},
  {128, 0, 0},                  // Dark red
  {128, 12, 0},
  {128, 23, 0},
  {128, 35, 0},
  {128, 47, 0},
  {128, 58, 0},
  {128, 70, 0},
  {128, 81, 0},
  {128, 93, 0},
  {128, 105, 0},
  {128, 116, 0},
  {128, 128, 0},                // Dark yellow
  {116, 128, 0},
  {105, 128, 0},
  {93, 128, 0},
  {81, 128, 0},
  {70, 128, 0},
  {58, 128, 0},
  {47, 128, 0},
  {35, 128, 0},
  {23, 128, 0},
  {12, 128, 0},
  {0, 128, 0},                  // Dark green
  {0, 128, 12},
  {0, 128, 23},
  {0, 128, 35},
  {0, 128, 47},
  {0, 128, 58},
  {0, 128, 70},
  {0, 128, 81},
  {0, 128, 93},
  {0, 128, 105},
  {0, 128, 116},
  {0, 128, 128},                // Dark cyan
  {0, 116, 128},
  {0, 105, 128},
  {0, 93, 128},
  {0, 81, 128},
  {0, 70, 128},
  {0, 58, 128},
  {0, 47, 128},
  {0, 35, 128},
  {0, 23, 128},
  {0, 12, 128},
  {0, 0, 128},                  // Dark blue
  {12, 0, 128},
  {23, 0, 128},
  {35, 0, 128},
  {47, 0, 128},
  {58, 0, 128},
  {70, 0, 128},
  {81, 0, 128},
  {93, 0, 128},
  {105, 0, 128},
  {116, 0, 128},
  {128, 0, 128},                // Dark magenta
  {128, 0, 116},
  {128, 0, 105},
  {128, 0, 93},
  {128, 0, 81},
  {128, 0, 70},
  {128, 0, 58},
  {128, 0, 47},
  {128, 0, 35},
  {128, 0, 23},
  {128, 0, 12},
  {255, 192, 192},              // Very light red
  {255, 64, 64},                // Light-medium red
  {192, 0, 0},                  // Medium-dark red
  {64, 0, 0},                   // Very dark red
  {255, 255, 192},              // Very light yellow
  {255, 255, 64},               // Light-medium yellow
  {192, 192, 0},                // Medium-dark yellow
  {64, 64, 0},                  // Very dark yellow
  {192, 255, 192},              // Very light green
  {64, 255, 64},                // Light-medium green
  {0, 192, 0},                  // Medium-dark green
  {0, 64, 0},                   // Very dark green
  {192, 255, 255},              // Very light cyan
  {64, 255, 255},               // Light-medium cyan
  {0, 192, 192},                // Medium-dark cyan
  {0, 64, 64},                  // Very dark cyan
  {192, 192, 255},              // Very light blue
  {64, 64, 255},                // Light-medium blue
  {0, 0, 192},                  // Medium-dark blue
  {0, 0, 64},                   // Very dark blue
  {255, 192, 255},              // Very light magenta
  {255, 64, 255},               // Light-medium magenta
  {192, 0, 192},                // Medium-dark magenta
  {64, 0, 64},                  // Very dark magenta
  {255, 96, 96},                // Medium skin tone
  {255, 255, 255},              // White (cycleable)
  {245, 245, 245},
  {235, 235, 235},
  {224, 224, 224},              // Very light gray (7/8 intensity)
  {213, 213, 213},
  {203, 203, 203},
  {192, 192, 192},              // Light gray (3/4 intensity)
  {181, 181, 181},
  {171, 171, 171},
  {160, 160, 160},              // Medium-light gray (5/8 int.)
  {149, 149, 149},
  {139, 139, 139},
  {128, 128, 128},              // Medium gray (1/2 intensity)
  {117, 117, 117},
  {107, 107, 107},
  {96, 96, 96},                 // Medium-dark gray (3/8 int.)
  {85, 85, 85},
  {75, 75, 75},
  {64, 64, 64},                 // Dark gray (1/4 intensity)
  {53, 53, 53},
  {43, 43, 43},
  {32, 32, 32},                 // Very dark gray (1/8 intensity)
  {21, 21, 21},
  {11, 11, 11},
  {0, 0, 0}                     // Black
};

#ifdef _WINDOWS
// no Windows compiler seems to have rint(), although it is C99
int rint(double x)
{
  return (int)(x > 0 ? x + 0.5 : x - 0.5);
}
#endif

int find_best_match_color(unsigned char r, unsigned char g, unsigned char b)
{
  unsigned int i, dmin = 195076, d, ret;
  signed int t;

  // FIXME inefficent algorithm
  for (i = 0; i < 255; i++) {
    t = r - ilda_standard_color_palette[i][0];
    d = t * t;
    t = g - ilda_standard_color_palette[i][1];
    d += t * t;
    t = b - ilda_standard_color_palette[i][2];
    d += t * t;
    if (d < dmin) {
      dmin = d;
      ret = i;
      if (dmin == 0)
        return ret;
    }
  }
  return ret;
}

pLaserPoint newLaserPoint(void)
{
  pLaserPoint p = NULL;

  if ((p = (pLaserPoint) malloc(sizeof(LaserPoint))) == NULL)
    return (NULL);

  p->x = p->y = p->z = 0;
  p->r = p->g = p->b = 0;
  p->attrib = 0;
  p->next = NULL;

  return (p);
}

pLaserFrame newLaserFrame(void)
{
  pLaserFrame p = NULL;

  if ((p = (pLaserFrame) malloc(sizeof(LaserFrame))) == NULL)
    return (NULL);

  p->next = NULL;
  p->previous = NULL;
  p->point_first = NULL;
  p->point_last = NULL;
  p->name = NULL;
  p->count = 0;

  return (p);
}

pLaserPoint frame_point_add(pLaserFrame fra)
{
  pLaserPoint point = fra->point_last;
  pLaserPoint point2 = NULL;

  fra->count += 1;

  if (point == NULL) {
    point = newLaserPoint();
    point->next = NULL;
    fra->point_first = point;
    fra->point_last = point;
    return point;
  }

  point2 = newLaserPoint();
  point2->next = NULL;

  point->next = point2;

  fra->point_last = point->next;

  return point2;
};

int frame_point_count(LaserFrame * f)
{
  return (f->count);
}

pLaserSequence newLaserSequence(void)
{
  pLaserSequence p = NULL;

  if ((p = (pLaserSequence) malloc(sizeof(LaserSequence))) == NULL)
    return (NULL);

  p->frame_count = 0;
  p->frame_first = NULL;
  p->frame_last = NULL;

  return (p);
}

int sequence_frame_count(pLaserSequence seq)
{
  return (seq->frame_count);
}

pLaserFrame sequence_frame_add(pLaserSequence seq)
{

  pLaserFrame frame1 = seq->frame_last;
  pLaserFrame frame2 = NULL;

  seq->frame_count += 1;

  if (frame1 == NULL) {
    frame1 = newLaserFrame();
    frame1->next = NULL;
    frame1->previous = NULL;
    seq->frame_first = frame1;
    seq->frame_last = frame1;
    return frame1;
  };

  frame2 = newLaserFrame();
  frame2->previous = frame1;

  frame1->next = frame2;

  seq->frame_last = frame2;

  return frame2;
};

/** write 2D/3D Frame to file */
int writeILDAFrame(FILE * file, LaserFrame * f, int format)
{
  unsigned char lastr = 0, lastg = 0, lastb = 0;
  unsigned int lastc = 0;
  unsigned char cbuffer[8];
  int points, c, b, cpoints;

  LaserPoint *point;

  cpoints = frame_point_count(f);

  point = f->point_first;
  points = 0;

  while (point) {

    if ((point->r == lastr) && (point->g == lastg) && (point->b == lastb)) {
      c = lastc;
    } else {
      c = find_best_match_color(point->r, point->g, point->b);
      lastc = c;
      lastr = point->r;
      lastg = point->g;
      lastb = point->b;
    }

    if ((!(point->r || point->g || point->b)) || (point->attrib & POINT_ATTRIB_BLANKED)) {
      b = 0x40;                 // set blanking bit if blank
    } else {
      b = 0x00;
    }

    if (points + 1 == cpoints)
      b += 0x80;                // set last point bit

    cbuffer[0] = point->x >> 8;
    cbuffer[1] = point->x & 255;
    cbuffer[2] = point->y >> 8;
    cbuffer[3] = point->y & 255;

    if (format == ILDA_3D_DATA) {
      cbuffer[4] = point->z >> 8;
      cbuffer[5] = point->z & 255;
      cbuffer[6] = b;
      cbuffer[7] = c;
      fwrite((char *)cbuffer, sizeof(char), 8, file);
    } else {
      cbuffer[4] = b;
      cbuffer[5] = c;
      fwrite((char *)cbuffer, sizeof(char), 6, file);
    }
    point = point->next;
    points++;
  };

  return 0;
}

/** write new style header */
int writeILDAHeader(FILE * file, unsigned int format, unsigned int datalength)
{
  // write ILDA header
  unsigned char fhbuffer[12];

  memcpy(fhbuffer, ilda, 4);

  fhbuffer[4] = (format >> 24) & 0xFF;
  fhbuffer[5] = (format >> 16) & 0xFF;
  fhbuffer[6] = (format >> 8) & 0xFF;
  fhbuffer[7] = format & 0xFF;

  fhbuffer[8] = (datalength >> 24) & 0xFF;
  fhbuffer[9] = (datalength >> 16) & 0xFF;
  fhbuffer[10] = (datalength >> 8) & 0xFF;
  fhbuffer[11] = datalength & 0xFF;

  return fwrite((char *)fhbuffer, sizeof(char), ((format > 2) ? 12 : 8), file);
}

/** write old-style frame header */
int writeILDAFrameHeader(FILE * file, LaserFrame * f, int format, unsigned int frames, unsigned int cframes)
{
  unsigned int cpoints = 0;
  unsigned char fhbuffer[24];
  unsigned char emptys[] = "                ";

  writeILDAHeader(file, format, 0);

  if (f) {
#ifdef _WINDOWS
    _snprintf((char *)(fhbuffer), 16, "Frame #%04d", frames);
#endif
#ifndef _WINDOWS
    snprintf((char *)(fhbuffer), 16, "Frame #%04d", frames);
#endif
  } else {
    strncpy((char *)(fhbuffer), (char *)emptys, 16);
  }

  if (f)
    cpoints = frame_point_count(f);

  fhbuffer[16] = (cpoints >> 8) & 255;
  fhbuffer[17] = cpoints & 255;
  fhbuffer[18] = (frames >> 8) & 255;
  fhbuffer[19] = frames & 255;
  fhbuffer[20] = (cframes >> 8) & 255;
  fhbuffer[21] = cframes & 255;
  fhbuffer[22] = 0;
  fhbuffer[23] = 0;

  return fwrite((char *)fhbuffer, sizeof(char), 24, file);
}

/** write ILDA True Color information to file */
int writeILDATrueColor(FILE * file, LaserFrame * f)
{
  unsigned char cbuffer[4];
  int cpoints;

  LaserPoint *point;

  cpoints = frame_point_count(f);

  writeILDAHeader(file, ILDA_TRUE_COLOR, (cpoints * 3) + 4);

  cbuffer[0] = (cpoints >> 24) & 0xFF;
  cbuffer[1] = (cpoints >> 16) & 0xFF;
  cbuffer[2] = (cpoints >> 8) & 0xFF;
  cbuffer[3] = cpoints & 0xFF;

  fwrite((char *)cbuffer, sizeof(char), 4, file);

  point = f->point_first;

  while (point) {
    cbuffer[0] = point->r;
    cbuffer[1] = point->g;
    cbuffer[2] = point->b;

    fwrite((char *)cbuffer, sizeof(char), 3, file);

    point = point->next;
  };

  return 0;
}

/** write color table to file */
int writeILDAColorTable(FILE * file)
{
  unsigned int i, palette = 0, colors = ILDA_COLORS_NUM;
  unsigned char fhbuffer[24];
  unsigned char emptys[] = "Color Table     ";

  writeILDAHeader(file, ILDA_COLOR_TABLE, 0);

  strncpy((char *)(fhbuffer), (char *)emptys, 16);
  fhbuffer[16] = (colors >> 8) & 255;
  fhbuffer[17] = colors & 255;
  fhbuffer[18] = (palette >> 8) & 255;
  fhbuffer[19] = palette & 255;
  fhbuffer[20] = 0;
  fhbuffer[21] = 0;
  fhbuffer[22] = 0;
  fhbuffer[23] = 0;
  fwrite((char *)fhbuffer, sizeof(char), 24, file);

  for (i = 0; i < colors; i++) {
    fhbuffer[0] = ilda_standard_color_palette[i][0];
    fhbuffer[1] = ilda_standard_color_palette[i][1];
    fhbuffer[2] = ilda_standard_color_palette[i][2];
    fwrite((char *)fhbuffer, sizeof(char), 3, file);
  }

  return 0;
}

/** write Sequence to ILDA file */
int writeILDA(FILE * file, LaserSequence * s)
{
  int format = (write3DFrames) ? ILDA_3D_DATA : ILDA_2D_DATA;
  int frames = 0, cframes, palettes = 0;
  LaserFrame *f;

  if (writeTable) {
    writeILDAColorTable(file);
  }

  cframes = sequence_frame_count(s);

  f = s->frame_first;

  while (f) {

    if (trueColorWrite)
      writeILDATrueColor(file, f);
    // write ILDA header for frame
    writeILDAFrameHeader(file, f, format, frames, cframes);
    writeILDAFrame(file, f, format);

    f = f->next;
    frames++;
  };

  // write empty ILDA header at EOF
  writeILDAFrameHeader(file, NULL, format, 0, cframes);

  return 0;
};

static inline short int clip(double x)
{
  if (x > 32767.0)
    x = 32767.0;
  if (x < -32768.0)
    x = -32768.0;
  return (unsigned short)rint(x);
}

/** No descriptions */
void blankingPath(int x1, int y1, int x2, int y2)
{
  int len, steps, i;
  double lx, ly, t;
  LaserPoint *p;
  lx = x2 - x1;
  ly = y2 - y1;
  len = rint(sqrt(lx * lx + ly * ly));

  if (!len)
    return;

  if (len < blankDistance) {
    steps = 1;
  } else {
    steps = len / blankDistance;
  }

  for (i = 0; i <= steps; i++) {
    t = (double)i / steps;
    p = frame_point_add(drawframe);
    p->x = clip((1 - t) * x1 + x2 * t);
    p->y = clip((1 - t) * y1 + y2 * t);
    p->z = 0;
    p->r = 0;
    p->g = 0;
    p->b = 0;
    p->attrib = POINT_ATTRIB_BLANKED;
  }
}

/** No descriptions */
void blankingPathTo(int x, int y)
{
  if ((!drawframe) || (!drawframe->point_last))
    return;
  blankingPath(drawframe->point_last->x, drawframe->point_last->y, x, y);
}

/** No descriptions */
void frameDrawInit(int x, int y, unsigned char r, unsigned char g, unsigned char b)
{
  if (!drawframe)
    drawframe = sequence_frame_add(drawsequence); // we can't do frameInit here, because we don't know where the first point will be.
  if (!frame_point_count(drawframe)) {
    if (drawframe->previous && ((LaserFrame *) drawframe->previous)->point_last) {
      blankingPath(((LaserFrame *) drawframe->previous)->point_last->x, ((LaserFrame *) drawframe->previous)->point_last->y, x, y);
    } else {
      if (fromToZero)
        blankingPath(0, 0, x, y);
    }
  } else {
    blankingPathTo(x, y);
  }
}

double getAngle(double b1x, double b1y, double b2x, double b2y)
{
  double acosa;
  double b1v = sqrt(b1x * b1x + b1y * b1y);
  double b2v = sqrt(b2x * b2x + b2y * b2y);

  if ((b1v == 0) || (b2v == 0))
    return 0.0;
  acosa = (b1x * b2x + b1y * b2y) / (b1v * b2v);
  if (acosa > 1.0)
    acosa = 1.0;
  if (acosa < -1.0)
    acosa = -1.0;
  return acos(acosa) * 180.0 / M_PI;
}

void insertAnchorPoints()
{
  LaserPoint *p = drawframe->point_first, *pn;
  double dx, dy, dx1, dy1, a;

  if ((!p) || (!p->next))
    return;

  dx1 = ((LaserPoint *) p->next)->x - p->x;
  dy1 = ((LaserPoint *) p->next)->y - p->y;
  p = p->next;

  while (p && p->next) {

    dx = ((LaserPoint *) p->next)->x - p->x;
    dy = ((LaserPoint *) p->next)->y - p->y;

#ifdef ANCHOR_DEBUG
    printf("x:%d y:%d", p->x, p->y);
    printf(" x:%d y:%d", ((LaserPoint *) p->next)->x, ((LaserPoint *) p->next)->y);
    printf(" dx1: %f dy1:%f dx: %f dy:%f\n", dx1, dy1, dx, dy);
#endif

    if (dx || dy) {
      a = getAngle(dx1, dy1, dx, dy);
      while (a > anchor_thresh) {
        pn = newLaserPoint();
        pn->x = p->x;
        pn->y = p->y;
        pn->z = p->z;
        pn->r = p->r;
        pn->g = p->g;
        pn->b = p->b;
#ifdef ANCHOR_DEBUG
        pn->r = 255;
        pn->g = 255;
        pn->b = 0;
#endif
        pn->attrib = p->attrib;
        pn->next = p->next;
        p->next = pn;
        drawframe->count += 1;
        inserted_anchor_points++;
        p = p->next;
        a -= anchor_thresh;
      }
      dx1 = dx;
      dy1 = dy;
    };

    p = p->next;
  }
}

void frameDrawFinish()
{
  LaserPoint *p;

  if (fromToZero)
    blankingPathTo(0, 0);

  if (sequence_frame_count(drawsequence) < 1) {
    frameDrawInit(0, 0, 0, 0, 0);

    if (frame_point_count(drawframe) < 1) {
      p = frame_point_add(drawframe); // add 0 point, else ILDA write will fail
      p->x = 0;
      p->y = 0;
      p->z = 0;
      p->r = 0;
      p->g = 0;
      p->b = 0;
      p->attrib = POINT_ATTRIB_BLANKED;
    }
  }

  if (insert_anchor_points)
    insertAnchorPoints();
}

void drawLine(double x1, double y1, double x2, double y2, unsigned char r1, unsigned char g1, unsigned char b1)
{
  int i, len, steps;
  double t, lx, ly;
  LaserPoint *p;
#ifdef ILD_DEBUG
  printf("Line from %f %f to %f %f", x1, y1, x2, y2);
  printf(" color %d %d %d\n", r1, g1, b1);
#endif

  frameDrawInit(rint(x1), rint(y1), r1, g1, b1);

  lx = x2 - x1;
  ly = y2 - y1;
  len = rint(sqrt(lx * lx + ly * ly));

  if (len < lineDistance) {
    steps = 1;
  } else {
    steps = len / lineDistance;
  }

  for (i = 0; i <= steps; i++) {
    t = (double)i / steps;
    p = frame_point_add(drawframe);
    p->x = clip((1 - t) * x1 + x2 * t);
    p->y = clip((1 - t) * y1 + y2 * t);
    p->z = 0;
    p->r = r1;
    p->g = g1;
    p->b = b1;
    p->attrib = 0;
  }

}

void drawCubicBezier(double x1, double y1, double cx1, double cy1, double cx2, double cy2, double x2, double y2, unsigned char r1, unsigned char g1, unsigned char b1)
{
  int len, steps, i;
  double t, lx, ly;
  LaserPoint *p;
#ifdef ILD_DEBUG
  printf("Cubic from %f %f over %f %f and %f %f to %f %f", x1, y1, cx1, cy1, cx2, cy2, x2, y2);
  printf(" color %d %d %d\n", r1, g1, b1);
#endif

  frameDrawInit(rint(x1), rint(y1), r1, g1, b1);

  // estimate arclength by convex hull FIXME: more precision
  lx = cx1 - x1;
  ly = cy1 - y1;
  len = rint(sqrt(lx * lx + ly * ly));
  lx = cx2 - cx1;
  ly = cy2 - cy1;
  len += rint(sqrt(lx * lx + ly * ly));
  lx = x2 - cx2;
  ly = y2 - cy2;
  len += rint(sqrt(lx * lx + ly * ly));

  if (len < lineDistance) {
    steps = 1;
  } else {
    steps = len / lineDistance;
  }

  for (i = 0; i <= steps; i++) {
    t = (double)i / steps;
    p = frame_point_add(drawframe);
    p->x = clip((1 - t) * (1 - t) * (1 - t) * x1 + cx1 * 3 * t * (1 - t) * (1 - t) + cx2 * 3 * t * t * (1 - t) + x2 * t * t * t);
    p->y = clip((1 - t) * (1 - t) * (1 - t) * y1 + cy1 * 3 * t * (1 - t) * (1 - t) + cy2 * 3 * t * t * (1 - t) + y2 * t * t * t);
    p->z = 0;
    p->r = r1;
    p->g = g1;
    p->b = b1;
    p->attrib = 0;
  }

}

/* Parses the spline data and writes out ILDA (*.ILD) formatted file */
static void OutputILDA(FILE * fdes, int llx, int lly, int urx, int ury, spline_list_array_type shape)
{
  unsigned int this_list, this_spline;
  spline_list_type curr_list;
  spline_type curr_spline;
  int last_degree;
  at_real_coord LastPoint;

  double sx = 65535.0 / (MAX(urx - llx, ury - lly));
  double sy = 65535.0 / (MAX(urx - llx, ury - lly));

  double ox = (llx + urx) / 2.0;
  double oy = (lly + ury) / 2.0;

  if (fdes == NULL)
    return;

  drawsequence = newLaserSequence();

  LastPoint.x = 0;
  LastPoint.y = 0;

  // visit each spline-list
  for (this_list = 0; this_list < SPLINE_LIST_ARRAY_LENGTH(shape); this_list++) {
    curr_list = SPLINE_LIST_ARRAY_ELT(shape, this_list);

    curr_spline = SPLINE_LIST_ELT(curr_list, 0);
    LastPoint = START_POINT(curr_spline);

    //visit each spline
    for (this_spline = 0; this_spline < SPLINE_LIST_LENGTH(curr_list); this_spline++) {
      curr_spline = SPLINE_LIST_ELT(curr_list, this_spline);
      last_degree = ((int)SPLINE_DEGREE(curr_spline));

      switch ((polynomial_degree) last_degree) {
      case LINEARTYPE:
        //output Line
        drawLine((LastPoint.x - ox) * sx, (LastPoint.y - oy) * sy, (END_POINT(curr_spline).x - ox) * sx, (END_POINT(curr_spline).y - oy) * sy, curr_list.color.r, curr_list.color.g, curr_list.color.b);
        LastPoint = END_POINT(curr_spline);
        break;

      default:
        //output Bezier curve
        drawCubicBezier((LastPoint.x - ox) * sx, (LastPoint.y - oy) * sy, (CONTROL1(curr_spline).x - ox) * sx, (CONTROL1(curr_spline).y - oy) * sy, (CONTROL2(curr_spline).x - ox) * sx, (CONTROL2(curr_spline).y - oy) * sy, (END_POINT(curr_spline).x - ox) * sx, (END_POINT(curr_spline).y - oy) * sy, curr_list.color.r, curr_list.color.g, curr_list.color.b);
        LastPoint = END_POINT(curr_spline);
        break;
      }
    }
  }

  frameDrawFinish();
  writeILDA(fdes, drawsequence);
}

int output_ild_writer(FILE * file, gchar * name, int llx, int lly, int urx, int ury, at_output_opts_type * opts, at_spline_list_array_type shape, at_msg_func msg_func, gpointer msg_data, gpointer user_data)
{

#ifdef _WINDOWS
  if (file == stdout) {
    fprintf(stderr, "This driver couldn't write to stdout!\n");
    return -1;
  }
#endif

  /* This should be user-adjustable. */
  write3DFrames = 0;
  trueColorWrite = 1;
  writeTable = 0;
  fromToZero = 1;
  lineDistance = 800;
  blankDistance = 1200;
  insert_anchor_points = 1;
  anchor_thresh = 40;

  /* Output ILDA */
  OutputILDA(file, llx, lly, urx, ury, shape);

  if (file == stdout)
    return 0;

  printf("Wrote %d frame with %d points (%d anchors", sequence_frame_count(drawsequence), frame_point_count(drawframe), inserted_anchor_points);
  if (trueColorWrite)
    printf(", True Color Header");
  if (writeTable)
    printf(", Color Table");
  printf(").\n");
  return 0;
}
