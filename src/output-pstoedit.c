/* output-pstoedit.c: utility routines for libpstoedit.so functions

   Copyright (C) 2002 2003 Masatake YAMATO

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

#include "autotrace.h"
#include "output.h"

#include <stdio.h>
#include <string.h>

/* Forward declare pstoedit's plain C API functions.
 *
 * We cannot include <pstoedit/pstoedit.h> from C code because it contains
 * C++ declarations (std::ostream, etc.) that break C compilation. This has
 * always been an issue, though older pstoedit versions or GCC may have been
 * more lenient. Forward declaring just the C functions we need avoids the
 * C++ dependency entirely.
 */

/* Driver description structure */
struct DriverDescription_S {
        const char *    symbolicname;
        const char *    explanation;
        const char *    suffix;
        const char *    additionalInfo;
        int                     backendSupportsSubPaths;
        int                     backendSupportsCurveto;
        int                 backendSupportsMerging;
        int                 backendSupportsText;
        int                 backendSupportsImages;
        int                     backendSupportsMultiplePages;
    int             formatGroup;
};


extern int pstoedit_plainC(
    const char * infile,
    const char * outfile,
    const char * outputFormat,
    const char * options
);


extern int pstoedit_checkversion(unsigned int callersversion);

extern void* getPstoeditDriverInfo_plainC(void);

extern void clearPstoeditDriverInfo_plainC(void* ptr);

/* #define OUTPUT_PSTOEDIT_DEBUG */

static int output_pstoedit_writer(FILE * file, gchar * name, int llx, int lly, int urx, int ury, at_output_opts_type * opts, at_spline_list_array_type shape, at_msg_func msg_func, gpointer msg_data, gpointer user_data);

static gboolean unusable_writer_p(const gchar * name);

static FILE *make_temporary_file(char *template, char *mode);

/* This output routine uses two temporary files to keep the
   both the command line syntax of autotrace and the
   pstoedit API.

   shape -> bo file(tmpfile_name_p2e)
   -> specified formatted file(tmpfile_name_pstoedit)
   -> file */
static int output_pstoedit_writer(FILE * file, gchar * name, int llx, int lly, int urx, int ury, at_output_opts_type * opts, at_spline_list_array_type shape, at_msg_func msg_func, gpointer msg_data, gpointer user_data)
{
  at_spline_writer *p2e_writer = NULL;
  char tmpfile_name_p2e[] = "/tmp/at-bo-XXXXXX";
  char tmpfile_name_pstoedit[] = "/tmp/at-fo-XXXXXX";
  const gchar *symbolicname = (const gchar *)user_data;
  FILE *tmpfile;
  int result = 0;

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
   * bo file -> specified formatted file via pstoedit
   */
  pstoedit_plainC(tmpfile_name_p2e,           /* input file */
                  tmpfile_name_pstoedit,      /* output file */
                  symbolicname,               /* output format */
                  "-bo");                     /* options */

  fclose(tmpfile);

remove_tmp_pstoedit:
  remove(tmpfile_name_pstoedit);
remove_tmp_p2e:
  remove(tmpfile_name_p2e);
  return result;
}

gboolean unusable_writer_p(const gchar * suffix)
{
  if (0 == strcmp(suffix, "sam")
      || 0 == strcmp(suffix, "dbg")
      || 0 == strcmp(suffix, "gs")
      || 0 == strcmp(suffix, "psf")
      || 0 == strcmp(suffix, "fps")
      || 0 == strcmp(suffix, "ps")
      || 0 == strcmp(suffix, "spsc")
      || 0 == strcmp(suffix, "debug")
      || 0 == strcmp(suffix, "dump")
      || 0 == strcmp(suffix, "ps2as"))
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

/* Minimum pstoedit version we require (3.01).
 * We need clearPstoeditDriverInfo_plainC which was added in version 301.
 * Note: pstoeditdllversion has static linkage in pstoedit.h and cannot
 * be referenced from C code.
 */
#define REQUIRED_PSTOEDIT_VERSION 0x301

  pstoedit_checkversion(REQUIRED_PSTOEDIT_VERSION);
  dd_start = getPstoeditDriverInfo_plainC();

  if (dd_start) {
    dd_tmp = dd_start;
    while (dd_tmp->symbolicname) {
      if (unusable_writer_p(dd_tmp->suffix)) {
        dd_tmp++;
        continue;
      }
      if (!at_output_get_handler_by_suffix(dd_tmp->suffix))
        at_output_add_handler_full(dd_tmp->suffix, dd_tmp->explanation, output_pstoedit_writer, 0, g_strdup(dd_tmp->symbolicname), g_free);
      if (!at_output_get_handler_by_suffix(dd_tmp->symbolicname))
        at_output_add_handler_full(dd_tmp->symbolicname, dd_tmp->explanation, output_pstoedit_writer, 0, g_strdup(dd_tmp->symbolicname), g_free);
      dd_tmp++;
    }
  }
  clearPstoeditDriverInfo_plainC(dd_start);
  return 0;
}
