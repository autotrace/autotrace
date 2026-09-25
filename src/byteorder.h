/*
 * SPDX-FileCopyrightText: © 2026 Peter Lemenkov
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

/* byteorder.h: store integers in a fixed byte order into a byte buffer.

   The binary writers assemble their headers in unsigned char buffers.
   Doing that with shifts and masks by hand is error prone and, for values
   with the top bit set, undefined behaviour on int; memcpy plus GLib's byte
   order macros is neither.  */

#ifndef BYTEORDER_H
#define BYTEORDER_H

#include <string.h>
#include <glib.h>

static inline void at_put_u16le(guchar *p, guint16 v)
{
  v = GUINT16_TO_LE(v);
  memcpy(p, &v, sizeof v);
}

static inline void at_put_u32le(guchar *p, guint32 v)
{
  v = GUINT32_TO_LE(v);
  memcpy(p, &v, sizeof v);
}

static inline void at_put_u16be(guchar *p, guint16 v)
{
  v = GUINT16_TO_BE(v);
  memcpy(p, &v, sizeof v);
}

static inline void at_put_u32be(guchar *p, guint32 v)
{
  v = GUINT32_TO_BE(v);
  memcpy(p, &v, sizeof v);
}

#endif /* not BYTEORDER_H */
