/* output-cgm.c: CGM output

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

#include "types.h"
#include "spline.h"
#include "color.h"
#include "output-cgm.h"
#include "xstd.h"
#include "autotrace.h"
#include <string.h>

#define CGM_BEGINMETAFILE        0x0020
#define CGM_BEGINPICTURE         0x0060
#define CGM_METAFILEVERSION      0x1022
#define CGM_METAFILEDESCRIPTION  0x1040

/* endianess independent IO functions */

static gboolean write16(FILE * fdes, uint16_t data)
{
  size_t count = 0;
  uint8_t outch;

  outch = (uint8_t) ((data >> 8) & 0x0FF);
  count += fwrite(&outch, 1, 1, fdes);

  outch = (uint8_t) (data & 0x0FF);
  count += fwrite(&outch, 1, 1, fdes);

  return (count == sizeof(uint16_t)) ? TRUE : FALSE;
}

static gboolean write8(FILE * fdes, uint8_t data)
{
  size_t count = 0;

  count = fwrite(&data, 1, 1, fdes);

  return (count == sizeof(uint8_t)) ? TRUE : FALSE;
}

static gboolean output_beginmetafilename(FILE * fdes, const char *string)
{
  size_t len = strlen(string);

  if (len + 1 < 0x001F)
    write16(fdes, (uint16_t) (CGM_BEGINMETAFILE + len + 1));
  else {
    write16(fdes, CGM_BEGINMETAFILE + 0x001F);
    write16(fdes, (uint16_t) (len + 1));
  }

  write8(fdes, (uint8_t) len);

  while (*string != '\0') {
    write8(fdes, *string);
    string++;
  }

  if (len % 2 == 0)
    write8(fdes, 0);

  return TRUE;
}

static gboolean output_beginpicture(FILE * fdes, const char *string)
{
  int len = strlen(string);

  if (len + 1 < 0x001F)
    write16(fdes, (uint16_t) (CGM_BEGINPICTURE + len + 1));
  else {
    write16(fdes, CGM_BEGINPICTURE + 0x001F);
    write16(fdes, (uint16_t) (len + 1));
  }

  write8(fdes, (uint8_t) len);

  while (*string != '\0') {
    write8(fdes, *string);
    string++;
  }

  if (len % 2 == 0)
    write8(fdes, 0);

  return TRUE;
}

static gboolean output_metafiledescription(FILE * fdes, const char *string)
{
  int len = strlen(string);

  if (len + 1 < 0x001F)
    write16(fdes, (uint16_t) (CGM_METAFILEDESCRIPTION + len + 1));
  else {
    write16(fdes, CGM_METAFILEDESCRIPTION + 0x001F);
    write16(fdes, (uint16_t) (len + 1));
  }

  write8(fdes, (uint8_t) len);

  while (*string != '\0') {
    write8(fdes, *string);
    string++;
  }

  if (len % 2 == 0)
    write8(fdes, 0);

  return TRUE;
}

int output_cgm_writer(FILE * cgm_file, gchar * name, int llx, int lly, int urx, int ury, at_output_opts_type * opts, spline_list_array_type shape, at_msg_func msg_func, gpointer msg_data, gpointer user_data)
{
  unsigned this_list;
  char *des;
  const char *version_string = at_version(TRUE);

  output_beginmetafilename(cgm_file, name);

  write16(cgm_file, CGM_METAFILEVERSION);
  write16(cgm_file, 0x0002);

  des = (char *)malloc(strlen("created by ") + strlen(version_string) + 1);
  strcpy(des, "created by ");
  strcat(des, version_string);
  output_metafiledescription(cgm_file, des);
  free(des);

  write16(cgm_file, 0x1166);    /* metafile element list */
  write16(cgm_file, 0x0001);
  write16(cgm_file, 0xFFFF);
  write16(cgm_file, 0x0001);

  output_beginpicture(cgm_file, "pic1");

  write16(cgm_file, 0x2042);    /* color selection modes (1 = direct) */
  write16(cgm_file, 0x0001);

  write16(cgm_file, 0x20C8);    /* vdc extend */
  write16(cgm_file, (uint16_t) llx /*0x0000 */ );
  write16(cgm_file, (uint16_t) urx /*0x7FFF */ );
  write16(cgm_file, (uint16_t) ury /*0x7FFF */ );
  write16(cgm_file, (uint16_t) lly /*0x0000 */ );

  write16(cgm_file, 0x0080);    /* begin picture body */

  for (this_list = 0; this_list < SPLINE_LIST_ARRAY_LENGTH(shape); this_list++) {
    unsigned this_spline;

    spline_list_type list = SPLINE_LIST_ARRAY_ELT(shape, this_list);

    if (this_list > 0) {
      if (shape.centerline)
        write16(cgm_file, 0x0200);  /* end a compound line */
      else
        write16(cgm_file, 0x0120);  /* end a figure */
    }

    if (shape.centerline)
      write16(cgm_file, 0x5083);  /* line */
    else
      write16(cgm_file, 0x52E3); /* fill */ ;

    /* color output */
    if (list.clockwise && shape.background_color != NULL) {
      write8(cgm_file, shape.background_color->r);
      write8(cgm_file, shape.background_color->g);
      write8(cgm_file, shape.background_color->b);
    } else {
      write8(cgm_file, list.color.r);
      write8(cgm_file, list.color.g);
      write8(cgm_file, list.color.b);
    }
    write8(cgm_file, 0);

    if (shape.centerline) {
      write16(cgm_file, 0x53C2);  /* edge visibility on */
      write16(cgm_file, 0x0001);
    } else {
      write16(cgm_file, 0x52C2);  /* interior style solid */
      write16(cgm_file, 0x0001);
    }

    if (shape.centerline)
      write16(cgm_file, 0x01E0);  /* begin a compound line */
    else
      write16(cgm_file, 0x0100);  /* begin a figure */

    for (this_spline = 0; this_spline < SPLINE_LIST_LENGTH(list); this_spline++) {
      spline_type s = SPLINE_LIST_ELT(list, this_spline);

      if (SPLINE_DEGREE(s) == LINEARTYPE) {
        write16(cgm_file, 0x4028);  /* polyline */
        write16(cgm_file, (uint16_t) START_POINT(s).x);
        write16(cgm_file, (uint16_t) (ury - START_POINT(s).y));
        write16(cgm_file, (uint16_t) END_POINT(s).x);
        write16(cgm_file, (uint16_t) (ury - END_POINT(s).y));
      } else {
        write16(cgm_file, 0x4352);  /* polybezier */
        write16(cgm_file, 0x0002);  /* continuous */
        write16(cgm_file, (uint16_t) START_POINT(s).x);
        write16(cgm_file, (uint16_t) (ury - START_POINT(s).y));
        write16(cgm_file, (uint16_t) CONTROL1(s).x);
        write16(cgm_file, (uint16_t) (ury - CONTROL1(s).y));
        write16(cgm_file, (uint16_t) CONTROL2(s).x);
        write16(cgm_file, (uint16_t) (ury - CONTROL2(s).y));
        write16(cgm_file, (uint16_t) END_POINT(s).x);
        write16(cgm_file, (uint16_t) (ury - END_POINT(s).y));
      }
    }
  }

  if (SPLINE_LIST_ARRAY_LENGTH(shape) > 0) {
    if (shape.centerline)
      write16(cgm_file, 0x0200);  /* end a compound line */
    else
      write16(cgm_file, 0x0120);  /* end a figure */
  }

  write16(cgm_file, 0x00A0);    /* end picture */

  write16(cgm_file, 0x0040);    /* end metafile */

  return (0);

}
