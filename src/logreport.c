/* logreport.c: showing information to the user. */

#include "logreport.h"
#include <glib.h>

static int current_log_level = G_LOG_LEVEL_WARNING;

static void custom_log_handler(const gchar *log_domain, GLogLevelFlags log_level,
                               const gchar *message, gpointer user_data)
{
  /* GLib log levels are bit flags where lower values = higher severity,
   * so >= comparison creates the right threshold */
  if (current_log_level >= log_level)
    g_log_default_handler(log_domain, log_level, message, user_data);
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
}
