/* output-pstoedit.c: utility routines for libpstoedit.so functions

   Copyright (C) 2002 Masatake YAMATO

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

/* This module uses a temporary file to pass a file to libpstoedit
   Which functions should I use to create tmpfile?
   -------------------------------------------------------------------
   Function || Security   | Availability
   ---------++------------+-------------------------------------------
   mktemp   || risky      | BSD 4.3
   mkstemp  || no problem | BSD 4.3
   tempnam  || risky      | SVID 2, BSD 4.3
   tmpfile  || no problem | SVID 3, POSIX, BSD 4.3, ISO 9899, SUSv2
   tmpnam   || risky      | SVID 2, POSIX, BSD 4.3, ISO 9899 
   -------------------------------------------------------------------
   tmpfile returns file pointer, not file name 

   mkstemp is the best in the security aspect, however it is not portable.
   (Read http://groups.yahoo.com/group/autotrace/message/369) */

#include "output-pstoedit.h"
#include "output-p2e.h"
#include "filename.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#define BO_DEBUG 0
#define TMPDIR "/tmp/"

static void remove_tmpfile (const at_string tmpfile_name);
static const at_string get_symbolicname(const at_string suffix);
static void   set_last_suffix (const at_string suffix);
static at_string get_last_suffix (void);
static at_string output_pstoedit_suffix = NULL;	/* Don't use directly. */
static FILE * make_temporary_file(char *template, char * mode);

at_output_write_func
output_pstoedit_get_writer(const at_string suffix)
{
  pstoedit_checkversion(pstoeditdllversion);
    
  if (get_symbolicname(suffix))
    {
      set_last_suffix (suffix);
      return (at_output_write_func) output_pstoedit_writer;
    }
  else
    return NULL;
}

/* This output routine uses two temporary files to keep the
   both the command line syntax of autotrace and the 
   pstoedit API. 

   shape -> bo file(tmpfile_name_p2e) 
   -> specified formatted file(tmpfile_name_pstoedit) 
   -> file */
int
output_pstoedit_writer (FILE* file, at_string name,
			int llx, int lly, int urx, int ury, 
			at_output_opts_type * opts,
			at_spline_list_array_type shape,
			at_msg_func msg_func, 
			at_address msg_data)
{
  char  tmpfile_name_p2e[] = TMPDIR "at-bo-" "XXXXXX";
  char  tmpfile_name_pstoedit[] = TMPDIR "at-fo-" "XXXXXX";
  char * symbolicname;
  FILE * tmpfile;
  int result = 0;
  int c;

  int argc;
#define CMD_INDEX         0
#define SYMBOLICNAME_FLAG_INDEX 1
#define SYMBOLICNAME_VAL_INDEX  2
#define BO_FLAG_INDEX     3
#define INPUT_INDEX       4
#define OUTPUT_INDEX      5
  char * argv[]  = {"pstoedit", "-f", 0, "-bo", 0, 0};

  argc = sizeof(argv) / sizeof(char *);
  
  pstoedit_checkversion(pstoeditdllversion);

  if (NULL == get_last_suffix())
    {
      if (msg_func) 
	msg_func ("Suffix or pstoedit backend driver name is not given", 
		  AT_MSG_WARNING, msg_data);
      return -1;
    }

  symbolicname = get_symbolicname(get_last_suffix());
  if (NULL == symbolicname)
    {
      if (msg_func) 
	msg_func ("Suffix or pstoedit backend driver name is wrong", 
		  AT_MSG_WARNING, msg_data);
      return -1;
    }

  tmpfile = make_temporary_file(tmpfile_name_p2e, "w");
  if (NULL == tmpfile)
    {
      result = -1;
      goto remove_tmp_p2e;
    }

  /* 
   * shape -> bo file 
   */
  output_p2e_writer(tmpfile, tmpfile_name_p2e,
		    llx, lly, urx, ury, opts,
		    shape, msg_func, msg_data);
  fclose(tmpfile);

  tmpfile = make_temporary_file(tmpfile_name_pstoedit, "r");
  if (NULL == tmpfile)
    {
      result = -1;
      goto remove_tmp_pstoedit;
    }

  /*
   * bo file -> specified formatted file 
   */
  argv[SYMBOLICNAME_VAL_INDEX] = symbolicname;
  argv[INPUT_INDEX]      = tmpfile_name_p2e;
  argv[OUTPUT_INDEX]     = tmpfile_name_pstoedit;
  pstoedit_plainC(argc, argv, NULL);

  /*
   * specified formatted file(tmpfile_name_pstoedit) -> file  
   */
  /* fseek(tmpfile, 0, SEEK_SET); */
  while (EOF != (c = fgetc(tmpfile)))
    fputc(c, file);
  fclose(tmpfile);
  
 remove_tmp_pstoedit:
  remove_tmpfile(tmpfile_name_pstoedit);
 remove_tmp_p2e:
  remove_tmpfile(tmpfile_name_p2e);
  return result;
}

static void
remove_tmpfile (const at_string tmpfile_name)
{
#if BO_DEBUG == 0
  remove (tmpfile_name);
#else 
  fprintf(stderr, "tmp file name: %s\n", tmpfile_name);
#endif /* Not BO_DEBUG == 0 */ 
}

at_bool
output_pstoedit_is_unusable_writer(const at_string name)
{
  if (0 == strcmp(name, "sam")
      || 0 == strcmp(name, "dbg")
      || 0 == strcmp(name, "gs"))
    return true;
  else
    return false;
}

static void
set_last_suffix (const at_string suffix)
{
  if (output_pstoedit_suffix)
    free(output_pstoedit_suffix);
  output_pstoedit_suffix = strdup (suffix);  
}

static at_string
get_last_suffix (void)
{
  return output_pstoedit_suffix;
}

/* get_symbolicname --- Return symbolicname associated with SUFFIX 
   If SUFFIX itself a symbolicname, just return SUFFIX.
   If SUFFIX doesn't have any associated symbolicname and
   it is not a suffix registered in pstoedit, reutrn NULL. */
static const at_string
get_symbolicname(const at_string suffix)
{
  struct DriverDescription_S* dd_tmp;
  
  if (!suffix)
    return NULL;
  
  dd_tmp = getPstoeditDriverInfo_plainC();
  if (dd_tmp)
    {
      while (dd_tmp->symbolicname)
	 {
	   if (0 == strcmp(dd_tmp->suffix, suffix))
	     return dd_tmp->symbolicname;
	   else if (0 == strcmp(dd_tmp->symbolicname, suffix))
	     return suffix;
	   else
	     dd_tmp++;
	 }
    }
  return NULL;
}


/* make_temporary_file --- Make a temporary file */
static FILE *
make_temporary_file(char *template, char * mode)
{
#ifdef HAVE_MKSTEMP
  /* #warning "To make temporary file, mkstemp will be used." */
  int tmpfd;
  tmpfd = mkstemp(template);
  if (tmpfd < 0)
    return NULL;
  return fdopen(tmpfd, mode);
#else
  /* #warning "To make temporary file, tmpnam will be used." */
  char * tmpname;
  tmpname = tmpnam(template);
  if (template == NULL)
    return NULL;
  return fopen(tmpname, mode);
#endif  /* HAVE_MKSTEMP */
}
