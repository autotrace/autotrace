/*
 * SPDX-FileCopyrightText: © 2000-2001 Martin Weber
 * SPDX-FileCopyrightText: © 2002 Masatake YAMATO
 * SPDX-FileCopyrightText: © 2017-2025 Peter Lemenkov
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

/* logreport.c: showing information to the user. */

#include "logreport.h"
#include "autotrace.h"
#include <glib.h>

/* The least severe level generated, once FILTERING is set.  Fitting threads
   read both, so they are accessed atomically.  */
static gint current_log_level = G_LOG_LEVEL_WARNING;

/* Set by at_set_log_level(), and by init_logging() for the command line
   program.  Until then a library client may have a handler of its own for
   our domain (g_log_set_handler()), which GLib gives no way to find out
   about, so every message goes to g_log(), as before (issue #371).  */
static gint filtering = FALSE;

gboolean log_level_enabled(GLogLevelFlags level)
{
  if (!g_atomic_int_get(&filtering))
    return TRUE;
  return g_atomic_int_get(&current_log_level) >= (gint)level;
}

gboolean log_shows_debug(void)
{
  return g_atomic_int_get(&filtering) && g_atomic_int_get(&current_log_level) >= G_LOG_LEVEL_DEBUG;
}

gboolean log_filters_messages(void)
{
  return g_atomic_int_get(&filtering);
}

/* GLib's six log levels.  G_LOG_LEVEL_MASK has every bit above the two
   flags set, bit 31 included, so it can't be used to pick them out.  */
#define LEVELS                                                                                     \
  (G_LOG_LEVEL_ERROR | G_LOG_LEVEL_CRITICAL | G_LOG_LEVEL_WARNING | G_LOG_LEVEL_MESSAGE |          \
   G_LOG_LEVEL_INFO | G_LOG_LEVEL_DEBUG)

void at_set_log_level(GLogLevelFlags level)
{
  guint levels = level & LEVELS;

  /* No level at all restores the default: every message is generated, and
     the program's handler, if any, passes them all on too.  */
  if (levels == 0) {
    g_atomic_int_set(&current_log_level, G_LOG_LEVEL_DEBUG);
    g_atomic_int_set(&filtering, FALSE);
    return;
  }
  /* GLib's level flags grow as severity falls: the highest flag is the
     least severe one (issue #416).  */
  g_atomic_int_set(&current_log_level, (gint)(1u << g_bit_nth_msf(levels, -1)));
  g_atomic_int_set(&filtering, TRUE);
}

static void custom_log_handler(const gchar *log_domain, GLogLevelFlags log_level,
                               const gchar *message, gpointer user_data)
{
  /* GLib log levels are bit flags where lower values = higher severity,
   * so >= comparison creates the right threshold.  Leave out the two
   * flags: G_LOG_FLAG_FATAL would put a fatal warning above the threshold
   * and drop it before GLib aborts (issue #417).  Compare unsigned, so that
   * a level of the client's own, above G_LOG_LEVEL_DEBUG, stays above.  */
  if ((guint)g_atomic_int_get(&current_log_level) < (guint)(log_level & G_LOG_LEVEL_MASK))
    return;

  if (log_level & (G_LOG_LEVEL_INFO | G_LOG_LEVEL_DEBUG)) {
    /* GLib's default handler prints these two levels on stdout, which is
       where the traced image goes when no output file is given.  Keep
       every diagnostic on stderr.  */
    g_printerr("%s-%s: %s\n", log_domain ? log_domain : "autotrace",
               (log_level & G_LOG_LEVEL_INFO) ? "INFO" : "DEBUG", message);
  } else {
    g_log_default_handler(log_domain, log_level, message, user_data);
  }
}

void set_log_level(const gchar *level)
{
  if (level == NULL)
    return;

  if (g_ascii_strcasecmp(level, "error") == 0) {
    at_set_log_level(G_LOG_LEVEL_ERROR);
  } else if (g_ascii_strcasecmp(level, "warning") == 0) {
    at_set_log_level(G_LOG_LEVEL_WARNING);
  } else if (g_ascii_strcasecmp(level, "info") == 0) {
    at_set_log_level(G_LOG_LEVEL_INFO);
  } else if (g_ascii_strcasecmp(level, "debug") == 0) {
    at_set_log_level(G_LOG_LEVEL_DEBUG);
  } else {
    g_warning("Unknown log level '%s', using 'warning'", level);
    at_set_log_level(G_LOG_LEVEL_WARNING);
  }
}

void init_logging(void)
{
  g_log_set_default_handler(custom_log_handler, NULL);
  /* The program prints warnings and errors unless -log asks for more.  */
  at_set_log_level(G_LOG_LEVEL_WARNING);
}
