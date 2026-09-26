/*
 * Copyright (C) 1999, 2000, 2001 Bernhard Herzog
 * Copyright (C) 2003 Masatake YAMATO
 * SPDX-FileCopyrightText: © 2000 MenTaLguY
 * SPDX-FileCopyrightText: © 2000-2003 Martin Weber
 * SPDX-FileCopyrightText: © 2000-2003 Masatake YAMATO
 * SPDX-FileCopyrightText: © 2017-2025 Peter Lemenkov
 * SPDX-FileCopyrightText: © 2020 Han Mertens
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "autotrace.h"
#include "private.h"
#include "input.h"
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
static at_input_format_entry *at_input_format_new(const char *descr, at_input_func reader,
                                                  gpointer user_data,
                                                  GDestroyNotify user_data_destroy_func);
static void at_input_format_free(at_input_format_entry *entry);

/*
 * Helper functions
 */
static void input_list_set(gpointer key, gpointer value, gpointer user_data);
static void input_list_append(gpointer key, gpointer value, gpointer user_data);

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

  at_input_formats = g_hash_table_new_full(g_str_hash, (GEqualFunc)g_str_equal, g_free,
                                           (GDestroyNotify)at_input_format_free);
  if (!at_input_formats)
    return 0;
  return 1;
}

static at_input_format_entry *at_input_format_new(const gchar *descr, at_input_func reader,
                                                  gpointer user_data,
                                                  GDestroyNotify user_data_destroy_func)
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

static void at_input_format_free(at_input_format_entry *entry)
{
  g_free((gpointer)entry->descr);
  if (entry->user_data_destroy_func)
    entry->user_data_destroy_func(entry->reader.data);
  g_free(entry);
}

int at_input_add_handler(const gchar *suffix, const gchar *description, at_input_func reader)
{
  return at_input_add_handler_full(suffix, description, reader, 0, NULL, NULL);
}

int at_input_add_handler_full(const gchar *suffix, const gchar *description, at_input_func reader,
                              gboolean override, gpointer user_data,
                              GDestroyNotify user_data_destroy_func)
{
  gchar *gsuffix;
  const gchar *gdescription;
  at_input_format_entry *old_entry;
  at_input_format_entry *new_entry;

  g_return_val_if_fail(suffix, 0);
  g_return_val_if_fail(description, 0);
  g_return_val_if_fail(reader, 0);

  g_autofree gchar *gsuffix_raw = g_strdup((gchar *)suffix);
  g_return_val_if_fail(gsuffix_raw, 0);
  gsuffix = g_ascii_strdown(gsuffix_raw, strlen(gsuffix_raw));

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

at_bitmap_reader *at_input_get_handler(gchar *filename)
{
  char *ext = find_suffix(filename);
  if (ext == NULL)
    ext = "";

  return at_input_get_handler_by_suffix(ext);
}

at_bitmap_reader *at_input_get_handler_by_suffix(const gchar *suffix)
{
  at_input_format_entry *format;

  if (!suffix || suffix[0] == '\0')
    return NULL;

  g_autofree gchar *gsuffix_raw = g_strdup(suffix);
  g_return_val_if_fail(gsuffix_raw, NULL);
  g_autofree gchar *gsuffix = g_ascii_strdown(gsuffix_raw, strlen(gsuffix_raw));
  format = g_hash_table_lookup(at_input_formats, gsuffix);

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
  g_free((char **)list);
}

char *at_input_shortlist(void)
{
  GString *list = g_string_new(NULL);

  g_hash_table_foreach(at_input_formats, input_list_append, list);
  return g_string_free(list, FALSE);
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

static void input_list_append(gpointer key, gpointer value, gpointer user_data)
{
  GString *list = user_data;

  if (list->len > 0)
    g_string_append(list, ", ");
  g_string_append(list, key);
}
