/*
 * SPDX-FileCopyrightText: © 2026 AutoTrace contributors
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

/* antialias.h: snap anti-aliased edge pixels to the flat colours.  */

#ifndef ANTIALIAS_H
#define ANTIALIAS_H

#include "autotrace.h"

/* Give every pixel that lies between two of the image's flat colours the
   nearer of the two, so that anti-aliased edges become hard ones (issue
   #348).  Images with fewer than two flat colours, or with too many, such
   as photographs, are left alone.  Once a row, NOTIFY_PROGRESS, if not
   NULL, gets the progress of the pass from 0 to 1, and then TEST_CANCEL,
   if not NULL, is asked whether to stop.  The pass returns FALSE as soon
   as it says so, leaving the bitmap partly snapped, and TRUE otherwise.
   CHANGED_PIXELS, if not NULL, gets the number of pixels changed.  */
gboolean snap_antialiased(at_bitmap *bitmap, at_progress_func notify_progress,
                          gpointer progress_data, at_testcancel_func test_cancel,
                          gpointer testcancel_data, unsigned long *changed_pixels);

#endif /* not ANTIALIAS_H */
