/*
 * Copyright (C) 2000, 2001, 2002 Martin Weber
 * SPDX-FileCopyrightText: © 2000-2002 Martin Weber
 * SPDX-FileCopyrightText: © 2000-2003 Masatake YAMATO
 * SPDX-FileCopyrightText: © 2017-2025 Peter Lemenkov
 * SPDX-FileCopyrightText: © 2021 Harald van Dijk
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef AT_COLOR_H
#define AT_COLOR_H

#include <glib.h>
#include <glib-object.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef struct _at_color at_color;
struct _at_color {
  guint8 r;
  guint8 g;
  guint8 b;
};

at_color *at_color_new(guint8 r, guint8 g, guint8 b);
at_color *at_color_parse(const gchar *string, GError **err);
at_color *at_color_copy(const at_color *original);
gboolean at_color_equal(const at_color *c1, const at_color *c2);
void at_color_set(at_color *c1, guint8 r, guint8 g, guint8 b);
/* RGB to grayscale */
unsigned char at_color_luminance(const at_color *color);
void at_color_free(at_color *color);

GType at_color_get_type(void);
#define AT_TYPE_COLOR (at_color_get_type())

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* not AT_COLOR_H */
