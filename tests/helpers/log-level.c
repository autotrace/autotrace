/* SPDX-FileCopyrightText: 2026 AutoTrace contributors
 *
 * SPDX-License-Identifier: CC0-1.0
 */

/* A library client with its own GLib log handler for the "autotrace"
   domain traces a small bitmap after each of a series of
   at_set_log_level() calls, and checks whether its handler saw debugging
   messages (#388, #416):
   - without a call: yes, as before the function existed;
   - G_LOG_LEVEL_WARNING: no;
   - G_LOG_LEVEL_DEBUG: yes;
   - G_LOG_LEVEL_ERROR: no;
   - G_LOG_LEVEL_MASK (every level): yes;
   - G_LOG_LEVEL_ERROR, then 0 (back to the default): yes.
   It also checks that libautotrace's WARNING() follows the level: its
   warning is dropped at G_LOG_LEVEL_ERROR and kept by default.

   Usage: log-level   Exits 0 if all of them hold.  */

#include <stdio.h>
#include <string.h>

#include "autotrace.h"
#include "logreport.h"

/* Fitting may call the handler from several threads.  */
static gint debug_messages, warnings;

static void count(const gchar *domain, GLogLevelFlags level, const gchar *message, gpointer data)
{
  if (level & G_LOG_LEVEL_DEBUG)
    g_atomic_int_inc(&debug_messages);
  if (level & G_LOG_LEVEL_WARNING)
    g_atomic_int_inc(&warnings);
}

/* Write one WARNING() and return whether the handler saw it.  */
static gboolean warning_seen(void)
{
  g_atomic_int_set(&warnings, 0);
  WARNING("test warning");
  return g_atomic_int_get(&warnings) > 0;
}

/* Trace a 20x20 white bitmap with a black square, and return how many
   debugging messages the handler saw.  */
static gint trace(void)
{
  at_bitmap *bitmap = at_bitmap_new(20, 20, 1);
  at_fitting_opts_type *opts = at_fitting_opts_new();
  at_splines_type *splines;
  unsigned y;

  memset(bitmap->bitmap, 255, 20 * 20);
  for (y = 5; y < 15; y++)
    memset(bitmap->bitmap + y * 20 + 5, 0, 10);
  g_atomic_int_set(&debug_messages, 0);
  splines = at_splines_new(bitmap, opts, NULL, NULL);
  if (splines)
    at_splines_free(splines);
  at_fitting_opts_free(opts);
  at_bitmap_free(bitmap);
  return g_atomic_int_get(&debug_messages);
}

static int failures;

static void expect(const char *what, gint seen, gboolean wanted)
{
  printf("%s: %d%s\n", what, seen, (seen > 0) == wanted ? "" : "  <- wrong");
  if ((seen > 0) != wanted)
    failures++;
}

int main(void)
{
  autotrace_init();
  g_log_set_handler("autotrace", G_LOG_LEVEL_MASK, count, NULL);

  expect("without at_set_log_level()", trace(), TRUE);
  at_set_log_level(G_LOG_LEVEL_WARNING);
  expect("G_LOG_LEVEL_WARNING", trace(), FALSE);
  at_set_log_level(G_LOG_LEVEL_DEBUG);
  expect("G_LOG_LEVEL_DEBUG", trace(), TRUE);
  at_set_log_level(G_LOG_LEVEL_ERROR);
  expect("G_LOG_LEVEL_ERROR", trace(), FALSE);
  expect("WARNING() at G_LOG_LEVEL_ERROR", warning_seen(), FALSE);
  at_set_log_level(G_LOG_LEVEL_MASK);
  expect("G_LOG_LEVEL_MASK", trace(), TRUE);
  at_set_log_level(G_LOG_LEVEL_ERROR);
  at_set_log_level((GLogLevelFlags)0);
  expect("G_LOG_LEVEL_ERROR, then 0", trace(), TRUE);
  expect("WARNING() by default", warning_seen(), TRUE);
  return failures ? 1 : 0;
}
