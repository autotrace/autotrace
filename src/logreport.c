/*
 * SPDX-FileCopyrightText: © 2000-2001 Martin Weber
 * SPDX-FileCopyrightText: © 2002 Masatake YAMATO
 * SPDX-FileCopyrightText: © 2017-2025 Peter Lemenkov
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

/* logreport.c: showing information to the user. */

#include "logreport.h"
#include <glib.h>

static int current_log_level = G_LOG_LEVEL_WARNING;

/* Set by init_logging(): the command line program filters messages
   through custom_log_handler.  */
static gboolean own_handler = FALSE;

gboolean log_level_enabled(GLogLevelFlags level)
{
  if (own_handler)
    return current_log_level >= (int)level;
  /* A library client may have a handler of its own for our domain
     (g_log_set_handler()), which GLib gives no way to find out about, so
     every message goes to g_log(), as before.  */
  return TRUE;
}

gboolean log_shows_debug(void)
{
  return own_handler && current_log_level >= G_LOG_LEVEL_DEBUG;
}

gboolean log_filters_messages(void)
{
  return own_handler;
}

static void custom_log_handler(const gchar *log_domain, GLogLevelFlags log_level,
                               const gchar *message, gpointer user_data)
{
  /* GLib log levels are bit flags where lower values = higher severity,
   * so >= comparison creates the right threshold */
  if (current_log_level < log_level)
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
    current_log_level = G_LOG_LEVEL_ERROR;
  } else if (g_ascii_strcasecmp(level, "warning") == 0) {
    current_log_level = G_LOG_LEVEL_WARNING;
  } else if (g_ascii_strcasecmp(level, "info") == 0) {
    current_log_level = G_LOG_LEVEL_INFO;
  } else if (g_ascii_strcasecmp(level, "debug") == 0) {
    current_log_level = G_LOG_LEVEL_DEBUG;
  } else {
    g_warning("Unknown log level '%s', using 'warning'", level);
    current_log_level = G_LOG_LEVEL_WARNING;
  }
}

void init_logging(void)
{
  g_log_set_default_handler(custom_log_handler, NULL);
  own_handler = TRUE;
}
