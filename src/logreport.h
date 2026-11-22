/*
 * SPDX-FileCopyrightText: © 2000-2001 Martin Weber
 * SPDX-FileCopyrightText: © 2001-2002 Masatake YAMATO
 * SPDX-FileCopyrightText: © 2017-2025 Peter Lemenkov
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

/* logreport.h: status reporting routines. */

#ifndef LOGREPORT_H
#define LOGREPORT_H

#include <stdlib.h>
#include <glib.h>

#define LOG_DOMAIN "autotrace"

/* Whether a message of LEVEL may be shown: FALSE only when the handler of
   the autotrace program (see init_logging()) would drop it.  DEBUG and LOG
   ask before they format anything: tracing writes a debugging message for
   nearly every point and spline, and formatting them all only for the
   handler to drop them took most of the running time.  */
gboolean log_level_enabled(GLogLevelFlags level);

/* Whether the autotrace program prints debugging messages (-log debug).  */
gboolean log_shows_debug(void);

/* Whether the autotrace program filters the messages (init_logging()), so
   that DEBUG and LOG skip the ones it would drop.  */
gboolean log_filters_messages(void);

#define LOG_IF_ENABLED(level, ...)                                                                 \
  G_STMT_START                                                                                     \
  {                                                                                                \
    if (log_level_enabled(level))                                                                  \
      g_log(G_LOG_DOMAIN, level, __VA_ARGS__);                                                     \
  }                                                                                                \
  G_STMT_END

#define DEBUG(...) LOG_IF_ENABLED(G_LOG_LEVEL_DEBUG, __VA_ARGS__)
#define LOG(...) LOG_IF_ENABLED(G_LOG_LEVEL_MESSAGE, __VA_ARGS__)
#define WARNING(...) g_warning(__VA_ARGS__)

/* For the command line program only: print the message and stop with exit
   status 1.  Library code must not use this; it reports errors through
   at_exception instead, so that the caller decides what happens.  */
#define FATAL(...)                                                                                 \
  G_STMT_START                                                                                     \
  {                                                                                                \
    g_printerr(__VA_ARGS__);                                                                       \
    g_printerr("\n");                                                                              \
    exit(EXIT_FAILURE);                                                                            \
  }                                                                                                \
  G_STMT_END

/* Initialize logging system - call once at program startup */
void init_logging(void);

/* Set log level: "error", "warning", "info", "debug" */
void set_log_level(const gchar *level);

#endif /* not LOGREPORT_H */
