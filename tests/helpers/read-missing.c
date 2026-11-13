/* SPDX-FileCopyrightText: 2026 AutoTrace contributors
 *
 * SPDX-License-Identifier: CC0-1.0
 */

/* Ask libautotrace to read files that don't exist, with a message handler
   that returns instead of exiting (as a plugin's would).  Every reader must
   report the error and return; before #322 the BMP and TGA readers went on
   to use the NULL FILE pointer and crashed.

   Usage: read-missing FILE...   Each FILE's suffix selects the reader.  */

#include <stdio.h>
#include <string.h>

#include "autotrace.h"

static int fatal_count;

static void count_fatal(const gchar *msg, at_msg_type type, gpointer data)
{
  if (type == AT_MSG_FATAL)
    fatal_count++;
}

int main(int argc, char **argv)
{
  int failures = 0;

  autotrace_init();
  for (int i = 1; i < argc; i++) {
    at_bitmap_reader *reader = at_input_get_handler(argv[i]);
    if (!reader) {
      printf("%s: no reader in this build, skipped\n", argv[i]);
      continue;
    }
    fatal_count = 0;
    at_bitmap *bitmap = at_bitmap_read(reader, argv[i], NULL, count_fatal, NULL);
    if (fatal_count == 0) {
      printf("%s: no error reported\n", argv[i]);
      failures++;
    } else
      printf("%s: error reported\n", argv[i]);
    if (bitmap)
      at_bitmap_free(bitmap);
  }
  return failures ? 1 : 0;
}
