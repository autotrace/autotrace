/* output.c: interface for output handlers

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

/* TODO: Unify output codes and input codes. */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* Def: HAVE_CONFIG_H */

#include "autotrace.h"
#include "private.h"
#include "output.h"
#include "xstd.h"
#include "filename.h"
#include <string.h>
#include <glib.h>

typedef struct _at_output_format_entry at_output_format_entry;
struct _at_output_format_entry {
  at_spline_writer writer;
  const char *descr;
  GDestroyNotify user_data_destroy_func;
};

static GHashTable *at_output_formats = NULL;
static at_output_format_entry *at_output_format_new(const char *descr, at_output_func writer, gpointer user_data, GDestroyNotify user_data_destroy_func);
static void at_output_format_free(at_output_format_entry * entry);

/*
 * Helper functions
 */
static void output_list_set(gpointer key, gpointer value, gpointer user_data);
static void output_list_strlen(gpointer key, gpointer value, gpointer user_data);
static void output_list_strcat(gpointer key, gpointer value, gpointer user_data);

int at_output_init(void)
{
  if (at_output_formats)
    return 1;

  at_output_formats = g_hash_table_new_full(g_str_hash, (GEqualFunc) g_str_equal, g_free, (GDestroyNotify) at_output_format_free);
  if (!at_output_formats)
    return 0;
  return 1;
}

static at_output_format_entry *at_output_format_new(const gchar * descr, at_output_func writer, gpointer user_data, GDestroyNotify user_data_destroy_func)
{
  at_output_format_entry *entry;
  entry = g_malloc(sizeof(at_output_format_entry));
  if (entry) {
    entry->writer.func = writer;
    entry->writer.data = user_data;
    entry->descr = g_strdup(descr);
    entry->user_data_destroy_func = user_data_destroy_func;
  }
  return entry;
}

static void at_output_format_free(at_output_format_entry * entry)
{
  g_free((gpointer) entry->descr);
  if (entry->user_data_destroy_func)
    entry->user_data_destroy_func(entry->writer.data);
  g_free(entry);

}

int at_output_add_handler(const gchar * suffix, const gchar * description, at_output_func writer)
{
  return at_output_add_handler_full(suffix, description, writer, 0, NULL, NULL);
}

int at_output_add_handler_full(const gchar * suffix, const gchar * description, at_output_func writer, gboolean override, gpointer user_data, GDestroyNotify user_data_destroy_func)
{
  gchar *gsuffix_raw;
  gchar *gsuffix;
  const gchar *gdescription;
  at_output_format_entry *old_entry;
  at_output_format_entry *new_entry;

  g_return_val_if_fail(suffix, 0);
  g_return_val_if_fail(description, 0);
  g_return_val_if_fail(writer, 0);

  gsuffix_raw = g_strdup((gchar *) suffix);
  g_return_val_if_fail(gsuffix_raw, 0);
  gsuffix = g_ascii_strdown(gsuffix_raw, strlen(gsuffix_raw));
  g_free(gsuffix_raw);

  gdescription = (const gchar *)description;

  old_entry = g_hash_table_lookup(at_output_formats, gsuffix);
  if (old_entry && !override) {
    g_free(gsuffix);
    return 1;
  }

  new_entry = at_output_format_new(gdescription, writer, user_data, user_data_destroy_func);
  g_return_val_if_fail(new_entry, 0);

  g_hash_table_replace(at_output_formats, gsuffix, new_entry);
  return 1;
}

at_spline_writer *at_output_get_handler(gchar * filename)
{
  char *ext = find_suffix(filename);
  if (ext == NULL)
    ext = "";

  return at_output_get_handler_by_suffix(ext);
}

at_spline_writer *at_output_get_handler_by_suffix(gchar * suffix)
{
  at_output_format_entry *format;
  gchar *gsuffix_raw;
  gchar *gsuffix;

  if (!suffix || suffix[0] == '\0')
    return NULL;

  gsuffix_raw = g_strdup(suffix);
  g_return_val_if_fail(gsuffix_raw, NULL);
  gsuffix = g_ascii_strdown(gsuffix_raw, strlen(gsuffix_raw));
  g_free(gsuffix_raw);
  format = g_hash_table_lookup(at_output_formats, gsuffix);
  g_free(gsuffix);

  if (format)
    return &(format->writer);
  else
    return NULL;
}

const char **at_output_list_new(void)
{
  char **list, **tmp;
  gint format_count;
  gint list_count;

  format_count = g_hash_table_size(at_output_formats);
  list_count = 2 * format_count;
  list = g_new(gchar *, list_count + 1);
  list[list_count] = NULL;

  tmp = list;
  g_hash_table_foreach(at_output_formats, output_list_set, &tmp);
  return (const char **)list;
}

void at_output_list_free(const char **list)
{
  free((char **)list);
}

char *at_output_shortlist(void)
{
  gint length = 0, count;
  char *list, *tmp;
  g_hash_table_foreach(at_output_formats, output_list_strlen, &length);
  count = g_hash_table_size(at_output_formats);

  /* 2 for ", " */
  length += (2 * count);
  list = g_malloc(length + 1);
  list[0] = '\0';

  tmp = list;
  g_hash_table_foreach(at_output_formats, output_list_strcat, &tmp);

  /* remove final ", " */
  g_return_val_if_fail(list[length - 2] == ',', NULL);
  list[length - 2] = '\0';
  return list;
}

static void output_list_set(gpointer key, gpointer value, gpointer user_data)
{
  at_output_format_entry *format = value;
  const char ***list_ptr = user_data;
  const char **list = *list_ptr;
  list[0] = key;
  list[1] = format->descr;
  *list_ptr = &(list[2]);
}

static void output_list_strlen(gpointer key, gpointer value, gpointer user_data)
{
  gint *length;
  g_return_if_fail(key);
  g_return_if_fail(user_data);

  length = user_data;
  *length += strlen(key);
}

static void output_list_strcat(gpointer key, gpointer value, gpointer user_data)
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

void at_spline_list_foreach(at_spline_list_type * list, AtSplineListForeachFunc func, gpointer user_data)
{
  unsigned i;
  for (i = 0; i < AT_SPLINE_LIST_LENGTH(list); i++) {
    func(list, AT_SPLINE_LIST_ELT(list, i), i, user_data);
  }
}

void at_spline_list_array_foreach(at_spline_list_array_type * list_array, AtSplineListArrayForeachFunc func, gpointer user_data)
{
  unsigned i;
  for (i = 0; i < AT_SPLINE_LIST_ARRAY_LENGTH(list_array); i++) {
    func(list_array, AT_SPLINE_LIST_ARRAY_ELT(list_array, i), i, user_data);
  }
}
