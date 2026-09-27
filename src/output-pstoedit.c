/*
 * Copyright (C) 2002, 2003 Masatake YAMATO
 * SPDX-FileCopyrightText: © 2002-2003 Martin Weber
 * SPDX-FileCopyrightText: © 2002-2003 Masatake YAMATO
 * SPDX-FileCopyrightText: © 2017-2025 Peter Lemenkov
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "autotrace.h"
#include "output.h"
#include "output-pstoedit.h"
#include "logreport.h"

#include <stdio.h>
#include <string.h>

/* pstoedit.h is a C++ header; pstoedll.h is the C-compatible description of
   the plain C entry points, meant for clients like us.  */
#include <pstoedit/pstoedll.h>

extern pstoedit_checkversion_func pstoedit_checkversion;
extern pstoedit_plainC_func pstoedit_plainC;
extern getPstoeditDriverInfo_plainC_func getPstoeditDriverInfo_plainC;
extern clearPstoeditDriverInfo_plainC_func clearPstoeditDriverInfo_plainC;

/* #define OUTPUT_PSTOEDIT_DEBUG */

static int output_pstoedit_writer(FILE *file, gchar *name, int llx, int lly, int urx, int ury,
                                  at_output_opts_type *opts, at_spline_list_array_type shape,
                                  at_msg_func msg_func, gpointer msg_data, gpointer user_data);

static gboolean unusable_writer_p(const gchar *name);

static FILE *make_temporary_file(char *template, char *mode);

/* This output routine uses two temporary files to keep the
   both the command line syntax of autotrace and the
   pstoedit API.

   shape -> bo file(tmpfile_name_p2e)
   -> specified formatted file(tmpfile_name_pstoedit)
   -> file */
static int output_pstoedit_writer(FILE *file, gchar *name, int llx, int lly, int urx, int ury,
                                  at_output_opts_type *opts, at_spline_list_array_type shape,
                                  at_msg_func msg_func, gpointer msg_data, gpointer user_data)
{
  at_spline_writer *p2e_writer = NULL;
  char tmpfile_name_p2e[] = "/tmp/at-bo-XXXXXX";
  char tmpfile_name_pstoedit[] = "/tmp/at-fo-XXXXXX";
  const gchar *symbolicname = (const gchar *)user_data;
  FILE *tmpfile;
  int result = 0;
  int c;
  int argc = 6;
  const char *argv[] = {
      "pstoedit", // argv[0] - program name
      "-f",       // format flag
      symbolicname,
      "-bo",                // backend options flag
      tmpfile_name_p2e,     // input file
      tmpfile_name_pstoedit // output file
  };

  tmpfile = make_temporary_file(tmpfile_name_p2e, "w");
  if (NULL == tmpfile) {
    result = -1;
    goto remove_tmp_p2e;
  }

  /*
   * shape -> bo file
   */
  p2e_writer = at_output_get_handler_by_suffix("p2e");
  at_splines_write(p2e_writer, tmpfile, tmpfile_name_p2e, opts, &shape, msg_func, msg_data);

  fclose(tmpfile);

  tmpfile = make_temporary_file(tmpfile_name_pstoedit, "r");
  if (NULL == tmpfile) {
    result = -1;
    goto remove_tmp_pstoedit;
  }

  /*
   * bo file -> specified formatted file
   */
  pstoedit_plainC(argc, argv, NULL);

  /*
   * specified formatted file(tmpfile_name_pstoedit) -> file
   */
  /* fseek(tmpfile, 0, SEEK_SET); */
  while (EOF != (c = fgetc(tmpfile)))
    fputc(c, file);
  fclose(tmpfile);

remove_tmp_pstoedit:
  remove(tmpfile_name_pstoedit);
remove_tmp_p2e:
  remove(tmpfile_name_p2e);
  return result;
}

gboolean unusable_writer_p(const gchar *suffix)
{
  if (0 == strcmp(suffix, "sam") || 0 == strcmp(suffix, "dbg") || 0 == strcmp(suffix, "gs") ||
      0 == strcmp(suffix, "psf") || 0 == strcmp(suffix, "fps") || 0 == strcmp(suffix, "ps") ||
      0 == strcmp(suffix, "spsc") || 0 == strcmp(suffix, "debug") || 0 == strcmp(suffix, "dump") ||
      0 == strcmp(suffix, "ps2as")
      /* plot-* drivers crash with segfault */
      || 0 == strcmp(suffix, "svg") || 0 == strcmp(suffix, "ai"))
    return TRUE;
  else
    return FALSE;
}

/* make_temporary_file --- Make a temporary file */
static FILE *make_temporary_file(char *template, char *mode)
{
  int tmpfd;
  tmpfd = g_mkstemp(template);
  if (tmpfd < 0)
    return NULL;
  return fdopen(tmpfd, mode);
}

int install_output_pstoedit_writers(void)
{
  struct DriverDescription_S *dd_start, *dd_tmp;

  /* The interface number is checked for equality: the struct layout below
     is the one from the header we were compiled against, so the library
     must be the same generation.  */
  if (!pstoedit_checkversion(pstoeditdllversion)) {
    WARNING("this autotrace was built for pstoedit interface %u, which the installed "
            "pstoedit library does not provide; pstoedit output formats are disabled",
            pstoeditdllversion);
    return 0;
  }

  dd_start = getPstoeditDriverInfo_plainC();

  if (dd_start) {
    dd_tmp = dd_start;
    while (dd_tmp->symbolicname) {
      if (unusable_writer_p(dd_tmp->suffix)) {
        dd_tmp++;
        continue;
      }
      if (!at_output_get_handler_by_suffix(dd_tmp->suffix))
        at_output_add_handler_full(dd_tmp->suffix, dd_tmp->explanation, output_pstoedit_writer, 0,
                                   g_strdup(dd_tmp->symbolicname), g_free);
      if (!at_output_get_handler_by_suffix(dd_tmp->symbolicname))
        at_output_add_handler_full(dd_tmp->symbolicname, dd_tmp->explanation,
                                   output_pstoedit_writer, 0, g_strdup(dd_tmp->symbolicname),
                                   g_free);
      dd_tmp++;
    }
  }
  clearPstoeditDriverInfo_plainC(dd_start);
  return 0;
}
