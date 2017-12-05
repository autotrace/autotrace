/* color.h: declarations for color handling.

  Copyright (C) 2000, 2001, 2002 Martin Weber

  The author can be contacted at <martweb@gmx.net>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA. */

#ifndef AT_COLOR_H
#define AT_COLOR_H

#include <glib.h>
#include <glib-object.h>

typedef struct _at_color at_color;
struct _at_color {
  guint8 r;
  guint8 g;
  guint8 b;
};

at_color *at_color_new(guint8 r, guint8 g, guint8 b);
at_color *at_color_parse(const gchar * string, GError ** err);
at_color *at_color_copy(const at_color * original);
gboolean at_color_equal(const at_color * c1, const at_color * c2);
void at_color_set(at_color * c1, guint8 r, guint8 g, guint8 b);
/* RGB to grayscale */
unsigned char at_color_luminance(const at_color * color);
void at_color_free(at_color * color);

GType at_color_get_type(void);
#define AT_TYPE_COLOR (at_color_get_type ())

#endif /* not AT_COLOR_H */
