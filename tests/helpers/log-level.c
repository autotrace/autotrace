/* SPDX-FileCopyrightText: 2026 AutoTrace contributors
 *
 * SPDX-License-Identifier: CC0-1.0
 */

/* A library client with its own GLib log handler for the "autotrace"
   domain traces a small bitmap three times: without calling
   at_set_log_level(), after at_set_log_level(G_LOG_LEVEL_WARNING), and
   after at_set_log_level(G_LOG_LEVEL_DEBUG).  Its handler must see
   debugging messages the first and third time, as before the function
   existed, and none the second time (#388).

   Usage: log-level   Exits 0 if all three hold.  */

#include <stdio.h>
#include <string.h>

#include "autotrace.h"

static unsigned debug_messages, other_messages;

static void count(const gchar *domain, GLogLevelFlags level, const gchar *message, gpointer data)
{
  if (level & G_LOG_LEVEL_DEBUG)
    debug_messages++;
  else
    other_messages++;
}

/* Trace a 20x20 white bitmap with a black square, and return how many
   debugging messages the handler saw.  */
static unsigned trace(void)
{
  at_bitmap *bitmap = at_bitmap_new(20, 20, 1);
  at_fitting_opts_type *opts = at_fitting_opts_new();
  at_splines_type *splines;
  unsigned y;

  memset(bitmap->bitmap, 255, 20 * 20);
  for (y = 5; y < 15; y++)
    memset(bitmap->bitmap + y * 20 + 5, 0, 10);
  debug_messages = other_messages = 0;
  splines = at_splines_new(bitmap, opts, NULL, NULL);
  if (splines)
    at_splines_free(splines);
  at_fitting_opts_free(opts);
  at_bitmap_free(bitmap);
  return debug_messages;
}

int main(void)
{
  unsigned before, warning, debug;

  autotrace_init();
  g_log_set_handler("autotrace", G_LOG_LEVEL_MASK, count, NULL);

  before = trace();
  at_set_log_level(G_LOG_LEVEL_WARNING);
  warning = trace();
  at_set_log_level(G_LOG_LEVEL_DEBUG);
  debug = trace();

  printf("debugging messages: %u without at_set_log_level(), %u at WARNING, %u at DEBUG\n", before,
         warning, debug);
  return before > 0 && warning == 0 && debug > 0 ? 0 : 1;
}
