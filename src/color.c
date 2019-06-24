/* color.c: declarations for color handling.

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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* Def: HAVE_CONFIG_H */

#include "intl.h"
#include "color.h"
#include "exception.h"

#include <string.h>
#include <stdlib.h>

at_color *at_color_new(unsigned char r, unsigned char g, unsigned char b)
{
  at_color *color;
  color = g_new(at_color, 1);
  color->r = r;
  color->g = g;
  color->b = b;
  return color;
}

at_color *at_color_parse(const gchar * string, GError ** err)
{
  GError *local_err = NULL;
  unsigned char c[6];
  int i;

  if (!string)
    return NULL;
  else if (string[0] == '\0')
    return NULL;
  else if (strlen(string) != 6) {
    g_set_error(err, AT_ERROR, AT_ERROR_WRONG_COLOR_STRING, _("color string is too short: %s"), string);
    return NULL;
  }

  for (i = 0; i < 6; i++) {
    char ch = string[i];
    if (ch >= '0' && ch <= '9')
      c[i] = ch - '0';
    else if (ch >= 'A' && ch <= 'F')
      c[i] = ch - 'A' + 10;
    else if (ch >= 'a' && ch <= 'f')
      c[i] = ch - 'a' + 10;
    else {
      g_set_error(&local_err, AT_ERROR, AT_ERROR_WRONG_COLOR_STRING, _("wrong char in color string: %c"), string[i]);
      g_propagate_error(err, local_err);
      return NULL;
    }
  }
  return at_color_new((unsigned char)(16 * c[0] + c[1]), (unsigned char)(16 * c[2] + c[3]), (unsigned char)(16 * c[4] + c[5]));
}

at_color *at_color_copy(const at_color * original)
{
  if (original == NULL)
    return NULL;

  return at_color_new(original->r, original->g, original->b);
}

gboolean at_color_equal(const at_color * c1, const at_color * c2)
{
  if (c1 == c2 || ((c1->r == c2->r) && (c1->g == c2->g) && (c1->b == c2->b)))
    return TRUE;

  return FALSE;
}

void at_color_set(at_color * c, unsigned char r, unsigned char g, unsigned char b)
{
  g_return_if_fail(c);
  c->r = r;
  c->g = g;
  c->b = b;
}

unsigned char at_color_luminance(const at_color * color)
{
  return ((unsigned char)((color->r) * 0.30 + (color->g) * 0.59 + (color->b) * 0.11 + 0.5));
}

void at_color_free(at_color * color)
{
  g_free(color);
}

GType at_color_get_type(void)
{
  static GType our_type = 0;
  if (our_type == 0)
    our_type = g_boxed_type_register_static("AtColor", (GBoxedCopyFunc) at_color_copy, (GBoxedFreeFunc) at_color_free);
  return our_type;
}
