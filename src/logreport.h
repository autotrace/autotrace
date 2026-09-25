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

/* Simple macros using GLib logging */
#define DEBUG(...) g_debug(__VA_ARGS__)
#define LOG(...) g_message(__VA_ARGS__)
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
