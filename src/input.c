/* input.c: interface for input handlers

   Copyright (C) 1999, 2000, 2001 Bernhard Herzog.
   Copyright (C) 2003 Masatake YAMATO

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public License
   as published by the Free Software Foundation; either version 2.1 of
   the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with this library; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
   USA. */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* Def: HAVE_CONFIG_H */

#include "autotrace.h"
#include "private.h"
#include "input.h"
#include "xstd.h"
#include "filename.h"
#include <string.h>
#include <glib.h>

typedef struct _at_input_format_entry at_input_format_entry;
struct _at_input_format_entry {
  at_bitmap_reader reader;
  const gchar *descr;
  GDestroyNotify user_data_destroy_func;
};

static GHashTable *at_input_formats = NULL;
static at_input_format_entry *at_input_format_new(const char *descr, at_input_func reader, gpointer user_data, GDestroyNotify user_data_destroy_func);
static void at_input_format_free(at_input_format_entry * entry);

/*
 * Helper functions
 */
static void input_list_set(gpointer key, gpointer value, gpointer user_data);
static void input_list_strlen(gpointer key, gpointer value, gpointer user_data);
static void input_list_strcat(gpointer key, gpointer value, gpointer user_data);

/**
 * at_input_init:
 * Initialize at_input input plugin sub system.
 *
 * Return value: 1 for success, else for failure
 **/
int at_input_init(void)
{
  if (at_input_formats)
    return 1;

  at_input_formats = g_hash_table_new_full(g_str_hash, (GEqualFunc) g_str_equal, g_free, (GDestroyNotify) at_input_format_free);
  if (!at_input_formats)
    return 0;
  return 1;
}

static at_input_format_entry *at_input_format_new(const gchar * descr, at_input_func reader, gpointer user_data, GDestroyNotify user_data_destroy_func)
{
  at_input_format_entry *entry;
  entry = g_malloc(sizeof(at_input_format_entry));
  if (entry) {
    entry->reader.func = reader;
    entry->reader.data = user_data;
    entry->descr = g_strdup(descr);
    entry->user_data_destroy_func = user_data_destroy_func;
  }
  return entry;
}

static void at_input_format_free(at_input_format_entry * entry)
{
  g_free((gpointer) entry->descr);
  if (entry->user_data_destroy_func)
    entry->user_data_destroy_func(entry->reader.data);
  g_free(entry);

}

int at_input_add_handler(const gchar * suffix, const gchar * description, at_input_func reader)
{
  return at_input_add_handler_full(suffix, description, reader, 0, NULL, NULL);
}

int at_input_add_handler_full(const gchar * suffix, const gchar * description, at_input_func reader, gboolean override, gpointer user_data, GDestroyNotify user_data_destroy_func)
{
  gchar *gsuffix_raw;
  gchar *gsuffix;
  const gchar *gdescription;
  at_input_format_entry *old_entry;
  at_input_format_entry *new_entry;

  g_return_val_if_fail(suffix, 0);
  g_return_val_if_fail(description, 0);
  g_return_val_if_fail(reader, 0);

  gsuffix_raw = g_strdup((gchar *) suffix);
  g_return_val_if_fail(gsuffix_raw, 0);
  gsuffix = g_ascii_strdown(gsuffix_raw, strlen(gsuffix_raw));
  g_free(gsuffix_raw);

  gdescription = (const gchar *)description;

  old_entry = g_hash_table_lookup(at_input_formats, gsuffix);
  if (old_entry && !override) {
    g_free(gsuffix);
    return 1;
  }

  new_entry = at_input_format_new(gdescription, reader, user_data, user_data_destroy_func);
  g_return_val_if_fail(new_entry, 0);

  g_hash_table_replace(at_input_formats, gsuffix, new_entry);
  return 1;
}

at_bitmap_reader *at_input_get_handler(gchar * filename)
{
  char *ext = find_suffix(filename);
  if (ext == NULL)
    ext = "";

  return at_input_get_handler_by_suffix(ext);
}

at_bitmap_reader *at_input_get_handler_by_suffix(gchar * suffix)
{
  at_input_format_entry *format;
  gchar *gsuffix_raw;
  gchar *gsuffix;

  if (!suffix || suffix[0] == '\0')
    return NULL;

  gsuffix_raw = g_strdup(suffix);
  g_return_val_if_fail(gsuffix_raw, NULL);
  gsuffix = g_ascii_strdown(gsuffix_raw, strlen(gsuffix_raw));
  g_free(gsuffix_raw);
  format = g_hash_table_lookup(at_input_formats, gsuffix);
  g_free(gsuffix);

  if (format)
    return &(format->reader);
  else
    return NULL;
}

const char **at_input_list_new(void)
{
  char **list, **tmp;
  gint format_count;
  gint list_count;

  format_count = g_hash_table_size(at_input_formats);
  list_count = 2 * format_count;
  list = g_new(gchar *, list_count + 1);
  list[list_count] = NULL;

  tmp = list;
  g_hash_table_foreach(at_input_formats, input_list_set, &tmp);
  return (const char **)list;
}

void at_input_list_free(const char **list)
{
  free((char **)list);
}

char *at_input_shortlist(void)
{
  gint length = 0, count;
  char *list, *tmp;
  g_hash_table_foreach(at_input_formats, input_list_strlen, &length);
  count = g_hash_table_size(at_input_formats);

  /* 2 for ", " */
  length += (2 * count);
  list = g_malloc(length + 1);
  list[0] = '\0';

  tmp = list;
  g_hash_table_foreach(at_input_formats, input_list_strcat, &tmp);

  /* remove final ", " */
  g_return_val_if_fail(list[length - 2] == ',', NULL);
  list[length - 2] = '\0';
  return list;
}

static void input_list_set(gpointer key, gpointer value, gpointer user_data)
{
  at_input_format_entry *format = value;
  const char ***list_ptr = user_data;
  const char **list = *list_ptr;
  list[0] = key;
  list[1] = format->descr;
  *list_ptr = &(list[2]);
}

static void input_list_strlen(gpointer key, gpointer value, gpointer user_data)
{
  gint *length;
  g_return_if_fail(key);
  g_return_if_fail(user_data);

  length = user_data;
  *length += strlen(key);
}

static void input_list_strcat(gpointer key, gpointer value, gpointer user_data)
{
  gchar **list_ptr;
  gchar *list;
  list_ptr = user_data;
  list = *list_ptr;
  strcat(list, key);
  strcat(list, ", ");

  /* 2 for ", " */
  *list_ptr = list + strlen(key) + 2;
}
