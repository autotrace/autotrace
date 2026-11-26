/*
 * SPDX-FileCopyrightText: © 2026 Peter Lemenkov
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

/* progress.h: the progress at_splines_new_full() reports to its client,
   and how its passes ask the client whether to cancel. */

#ifndef PROGRESS_H
#define PROGRESS_H

#include "autotrace.h"
#include <glib.h>

/* The client's test_cancel callback, for the passes that run before the
   outlines are found.  CANCELLED is set once the callback has said so,
   and stays set: the caller checks it after the pass, without asking the
   client again.  */
typedef struct {
  at_testcancel_func test_cancel;
  gpointer data;
  gboolean cancelled;
  /* Steps of work since the last check, for cancel_after_steps().  */
  unsigned steps;
} cancel_hook;

/* How many steps of cheap work, such as pixels scanned, go between two
   cancel checks: a few milliseconds' work.  */
#define STEPS_PER_CANCEL_CHECK (1u << 20)

/* Ask the client whether to cancel, unless it already has.  CANCEL may be
   NULL, for callers that can't be cancelled.  */
static inline gboolean cancel_requested(cancel_hook *cancel)
{
  if (cancel && !cancel->cancelled && cancel->test_cancel && cancel->test_cancel(cancel->data))
    cancel->cancelled = TRUE;
  return cancel && cancel->cancelled;
}

/* An at_testcancel_func for passes that take the client's callback as it
   is: DATA is a cancel_hook, and once the client has said to cancel, the
   answer stays TRUE, even if the client's own callback says so only once
   (issue #407).  */
static inline gboolean remembered_cancel(gpointer data)
{
  return cancel_requested(data);
}

/* Whether the client has already said to cancel, without asking again.  */
static inline gboolean was_cancelled(const cancel_hook *cancel)
{
  return cancel && cancel->cancelled;
}

/* Count N more steps of work, and ask the client whether to cancel once
   STEPS_PER_CANCEL_CHECK of them have been done since the last check.
   For loops whose length isn't known in advance, such as flood fills.  */
static inline gboolean cancel_after_steps(cancel_hook *cancel, unsigned n)
{
  if (!cancel)
    return FALSE;
  cancel->steps += n;
  if (cancel->steps < STEPS_PER_CANCEL_CHECK)
    return cancel->cancelled;
  cancel->steps = 0;
  return cancel_requested(cancel);
}

/* How many rows of WIDTH pixels a cheap per-pixel scan handles between two
   cancel checks.  */
static inline unsigned rows_per_cancel_check(unsigned width)
{
  return MAX(1u, STEPS_PER_CANCEL_CHECK / MAX(1u, width));
}

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
