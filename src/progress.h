/*
 * SPDX-FileCopyrightText: © 2026 Peter Lemenkov
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

/* progress.h: the progress at_splines_new_full() reports to its client. */

#ifndef PROGRESS_H
#define PROGRESS_H

#include <glib.h>

/* Tracing reports its progress in three equal steps: finding the outlines
   (STEP 0), fitting curves to them (1), and freeing the curve lists (2).
   Return the progress when item DONE of TOTAL in STEP is about to be
   handled.

   Computing it in double keeps the steps in order: the last value of a
   step is never above the first value of the next, so the progress never
   goes backwards, even for images with more pixels than a gfloat counts
   exactly.  */
static inline gfloat step_progress(unsigned step, gsize done, gsize total)
{
  return (gfloat)((step + (total ? (double)done / total : 0.0)) / 3.0);
}

#endif /* not PROGRESS_H */
