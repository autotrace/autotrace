/* output-pstoedit.c: utility routines for libpstoedit.so function

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
   tmpfile returns file poineter, not file name 

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

static void remove_tmpfile (at_string tmpfile_name);

int
output_pstoedit_writer (FILE* file, at_string name,
			int llx, int lly, int urx, int ury, int dpi,
			at_spline_list_array_type shape,
			at_msg_func msg_func, 
			at_address msg_data)
{
  char  tmpfile_name[] = "at-bo-" "XXXXXX";
  char * suffix;
  int tmpfd;
  FILE * tmpfile;

  int argc;
#define CMD_INDEX         0
#define SUFFIX_FLAG_INDEX 1
#define SUFFIX_VAL_INDEX  2
#define BO_FLAG_INDEX     3
#define INPUT_INDEX       4
#define OUTPUT_INDEX      5
  char * argv[]  = {"pstoedit", "-f", 0, "-bo", 0, 0};

  argc = sizeof(argv) / sizeof(char *);
  
  pstoedit_checkversion(pstoeditdllversion);
  
  suffix = find_suffix(name);	/* Hack here */
  if (NULL == suffix)
    {
      if (msg_func) 
	msg_func ("Cannot find suffix in given file name", 
		  AT_MSG_FATAL, msg_data);
      return -1;
    }
  
  tmpfd  = mkstemp(tmpfile_name);
  if (tmpfd < 0)
    {
      if (msg_func)
	{
	  msg_func ("Fail to create a tmp file that is passed to pstoedit(mkstemp)", 
		    AT_MSG_WARNING, msg_data);
	  msg_func (strerror(errno), AT_MSG_FATAL, msg_data);
	}
      return -1;
    }

  tmpfile = fdopen(tmpfd, "w");
  if (NULL == tmpfile)
    {
      if (msg_func)
	{
	  msg_func ("Fail to create a tmp file that is passed to pstoedit(fdopen)", 
		    AT_MSG_WARNING, msg_data);
	  msg_func (strerror(errno), AT_MSG_FATAL, msg_data);
	}
      return -1;
    }
  
  output_p2e_writer(tmpfile, tmpfile_name,
		    llx, lly, urx, ury, dpi,
		    shape, msg_func, msg_data);
  
  argv[SUFFIX_VAL_INDEX] = suffix;
  argv[INPUT_INDEX]      = tmpfile_name;
  argv[OUTPUT_INDEX]     = name;
  pstoedit_plainC(argc, argv, NULL);
  
  fclose(tmpfile);
  remove_tmpfile(tmpfile_name);
  return 0;
}

static void
remove_tmpfile (at_string tmpfile_name)
{
#if BO_DEBUG == 0
  remove (tmpfile_name);
#endif /* Not BO_DEBUG == 0 */  
}

at_bool
output_pstoedit_is_unusable_writer(at_string name)
{
  if (0 == strcmp(name, "sam")
      || 0 == strcmp(name, "dbg")
      || 0 == strcmp(name, "gs"))
    return true;
  else
    return false;
}
