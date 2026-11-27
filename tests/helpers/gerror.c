/* SPDX-FileCopyrightText: 2026 AutoTrace contributors
 *
 * SPDX-License-Identifier: CC0-1.0
 */

/* The functions that report failures through a GError (#345):
   at_bitmap_read_with_error, at_splines_trace and
   at_splines_write_with_error.  Each must set the error exactly when it
   fails, in the AT_ERROR domain with the right code, and pass only
   warnings to the client's message callback.

   Usage: gerror IN.png OUT.svg   IN.png must be readable; OUT.svg is
   written and then reopened for reading.  */

#include <stdio.h>
#include <string.h>

#include "autotrace.h"

static int failures;
static int fatal_messages;

static void count_fatal(const gchar *msg, at_msg_type type, gpointer data)
{
  if (type == AT_MSG_FATAL)
    fatal_messages++;
}

static gboolean cancel_at_once(gpointer data)
{
  return TRUE;
}

static gboolean never_cancel(gpointer data)
{
  int *calls = data;
  (*calls)++;
  return FALSE;
}

static void check(gboolean ok, const char *what)
{
  printf("%s: %s\n", ok ? "ok" : "FAILED", what);
  if (!ok)
    failures++;
}

/* Whether ERROR is set, in AT_ERROR with CODE and a message.  */
static gboolean is_error(const GError *error, AtError code)
{
  return error != NULL && error->domain == AT_ERROR && error->code == (gint)code &&
         error->message != NULL && error->message[0] != '\0';
}

int main(int argc, char **argv)
{
  GError *error = NULL;
  at_bitmap_reader *reader;
  at_bitmap *bitmap;
  at_fitting_opts_type *opts;
  at_splines_type *splines;
  at_spline_writer *writer;
  gchar *missing;
  FILE *out;
  int calls = 0;

  if (argc != 3) {
    fprintf(stderr, "usage: gerror IN.png OUT.svg\n");
    return 2;
  }
  autotrace_init();

  /* The value of the old code must stay 0, for clients built before.  */
  check(AT_ERROR_WRONG_COLOR_STRING == 0, "AT_ERROR_WRONG_COLOR_STRING is still 0");

  reader = at_input_get_handler(argv[1]);
  check(reader != NULL, "a reader for the input");
  if (reader == NULL)
    return 1;

  /* Reading a missing file.  */
  missing = g_strconcat(argv[2], ".missing.png", NULL);
  fatal_messages = 0;
  bitmap = at_bitmap_read_with_error(reader, missing, NULL, count_fatal, NULL, &error);
  check(bitmap == NULL, "a missing file gives no bitmap");
  check(is_error(error, AT_ERROR_FAILED), "a missing file sets AT_ERROR_FAILED");
  check(fatal_messages == 0, "the fatal message goes to the error, not the callback");
  if (error)
    printf("  (%s)\n", error->message);
  g_clear_error(&error);
  bitmap = at_bitmap_read_with_error(reader, missing, NULL, NULL, NULL, NULL);
  check(bitmap == NULL, "a missing file with no error and no callback");
  g_free(missing);

  /* Reading a good file.  */
  bitmap = at_bitmap_read_with_error(reader, argv[1], NULL, count_fatal, NULL, &error);
  check(bitmap != NULL && error == NULL, "a good file gives a bitmap and no error");
  if (bitmap == NULL)
    return 1;

  /* Tracing, cancelled at the first question.  */
  opts = at_fitting_opts_new();
  splines = at_splines_trace(bitmap, opts, count_fatal, NULL, NULL, NULL, cancel_at_once, NULL,
                             &error);
  check(splines == NULL, "a cancelled trace gives no splines");
  check(is_error(error, AT_ERROR_CANCELLED), "a cancelled trace sets AT_ERROR_CANCELLED");
  g_clear_error(&error);

  /* Tracing to the end.  Copy the bitmap: tracing may change it.  */
  {
    at_bitmap *copy = at_bitmap_copy(bitmap);
    splines = at_splines_trace(copy, opts, count_fatal, NULL, NULL, NULL, never_cancel, &calls,
                               &error);
    at_bitmap_free(copy);
  }
  check(splines != NULL && error == NULL, "a trace gives splines and no error");
  check(calls > 0, "the client's test_cancel was asked");
  if (splines == NULL)
    return 1;

  /* Writing.  */
  writer = at_output_get_handler_by_suffix("svg");
  check(writer != NULL, "an SVG writer");
  if (writer == NULL)
    return 1;
  out = fopen(argv[2], "w");
  check(out != NULL, "the output file opens");
  if (out == NULL)
    return 1;
  check(at_splines_write_with_error(writer, out, argv[2], NULL, splines, count_fatal, NULL,
                                    &error) &&
            error == NULL,
        "a write succeeds with no error");
  fclose(out);

  /* Writing to a stream opened for reading must fail.  */
  out = fopen(argv[2], "r");
  check(out != NULL, "the output file opens for reading");
  if (out != NULL) {
    gboolean ok =
        at_splines_write_with_error(writer, out, argv[2], NULL, splines, NULL, NULL, &error);
    check(!ok && is_error(error, AT_ERROR_FAILED), "a write to a read-only stream fails");
    if (error)
      printf("  (%s)\n", error->message);
    g_clear_error(&error);
    fclose(out);
  }

  check(fatal_messages == 0, "no fatal message reached the callback");
  at_splines_free(splines);
  at_fitting_opts_free(opts);
  at_bitmap_free(bitmap);
  return failures ? 1 : 0;
}
