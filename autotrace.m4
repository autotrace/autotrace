# a macro to get the libs/cflags for libautotrace
# Copyed from gdk-pixbuf.m4

dnl AM_PATH_AUTOTRACE([MINIMUM-VERSION, [ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND]])
dnl Test to see if libautotrace is installed, and define AUTOTRACE_CFLAGS, LIBS
dnl
AC_DEFUN(AM_PATH_AUTOTRACE,
[dnl
dnl Get the cflags and libraries from the autotrace-config script
dnl
AC_ARG_WITH(autotrace-prefix,[  --with-autotrace-prefix=PFX   Prefix where Autotrace is installed (optional)],
            autotrace_prefix="$withval", autotrace_prefix="")
AC_ARG_WITH(autotrace-exec-prefix,[  --with-autotrace-exec-prefix=PFX Exec prefix where Autotrace is installed (optional)],
            autotrace_exec_prefix="$withval", autotrace_exec_prefix="")
AC_ARG_ENABLE(autotracetest, [  --disable-autotracetest       Do not try to compile and run a test Autotrace program],
		    , enable_autotracetest=yes)

  if test x$autotrace_exec_prefix != x ; then
     autotrace_args="$autotrace_args --exec_prefix=$autotrace_exec_prefix"
     if test x${AUTOTRACE_CONFIG+set} != xset ; then
        AUTOTRACE_CONFIG=$autotrace_exec_prefix/bin/autotrace-config
     fi
  fi
  if test x$autotrace_prefix != x ; then
     autotrace_args="$autotrace_args --prefix=$autotrace_prefix"
     if test x${AUTOTRACE_CONFIG+set} != xset ; then
        AUTOTRACE_CONFIG=$autotrace_prefix/bin/autotrace-config
     fi
  fi

  AC_PATH_PROG(AUTOTRACE_CONFIG, autotrace-config, no)
  min_autotrace_version=ifelse([$1], ,0.30.1,$1)
  AC_MSG_CHECKING(for AUTOTRACE - version >= $min_autotrace_version)
  no_autotrace=""
  if test "$AUTOTRACE_CONFIG" = "no" ; then
    no_autotrace=yes
  else
    AUTOTRACE_CFLAGS=`$AUTOTRACE_CONFIG $autotrace_args --cflags`
    AUTOTRACE_LIBS=`$AUTOTRACE_CONFIG $autotrace_args --libs`

    autotrace_major_version=`$AUTOTRACE_CONFIG $autotrace_args --version | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\1/'`
    autotrace_minor_version=`$AUTOTRACE_CONFIG $autotrace_args --version | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\2/'`
    autotrace_micro_version=`$AUTOTRACE_CONFIG $autotrace_args --version | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\3/'`
    if test "x$enable_autotracetest" = "xyes" ; then
      ac_save_CFLAGS="$CFLAGS"
      ac_save_LIBS="$LIBS"
      CFLAGS="$CFLAGS $AUTOTRACE_CFLAGS"
      LIBS="$AUTOTRACE_LIBS $LIBS"
dnl
dnl Now check if the installed AUTOTRACE is sufficiently new. (Also sanity
dnl checks the results of autotrace-config to some extent
dnl
      rm -f conf.autotracetest
      AC_TRY_RUN([
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <autotrace/autotrace.h>

char*
my_strdup (char *str)
{
  char *new_str;
  
  if (str)
    {
      new_str = malloc ((strlen (str) + 1) * sizeof(char));
      strcpy (new_str, str);
    }
  else
    new_str = NULL;
  
  return new_str;
}

int main ()
{
  int major, minor, micro;
  char *tmp_version;

  system ("touch conf.autotracetest");

  /* HP/UX 9 (%@#!) writes to sscanf strings */
  tmp_version = my_strdup("$min_autotrace_version");
  if (sscanf(tmp_version, "%d.%d.%d", &major, &minor, &micro) != 3) {
     printf("%s, bad version string\n", "$min_autotrace_version");
     exit(1);
   }

   if (($autotrace_major_version > major) ||
      (($autotrace_major_version == major) && ($autotrace_minor_version > minor)) ||
      (($autotrace_major_version == major) && ($autotrace_minor_version == minor) && ($autotrace_micro_version >= micro)))
    {
      return 0;
    }
  else
    {
      printf("\n*** 'autotrace-config --version' returned %d.%d.%d, but the minimum version\n", $autotrace_major_version, $autotrace_minor_version, $autotrace_micro_version);
      printf("*** of AUTOTRACE required is %d.%d.%d. If autotrace-config is correct, then it is\n", major, minor, micro);
      printf("*** best to upgrade to the required version.\n");
      printf("*** If autotrace-config was wrong, set the environment variable AUTOTRACE_CONFIG\n");
      printf("*** to point to the correct copy of autotrace-config, and remove the file\n");
      printf("*** config.cache before re-running configure\n");
      return 1;
    }
}
],, no_autotrace=yes,[echo $ac_n "cross compiling; assumed OK... $ac_c"])
       CFLAGS="$ac_save_CFLAGS"
       LIBS="$ac_save_LIBS"
     fi
  fi
  if test "x$no_autotrace" = x ; then
     AC_MSG_RESULT(yes)
     ifelse([$2], , :, [$2])     
  else
     AC_MSG_RESULT(no)
     if test "$AUTOTRACE_CONFIG" = "no" ; then
       echo "*** The autotrace-config script installed by AUTOTRACE could not be found"
       echo "*** If AUTOTRACE was installed in PREFIX, make sure PREFIX/bin is in"
       echo "*** your path, or set the AUTOTRACE_CONFIG environment variable to the"
       echo "*** full path to autotrace-config."
     else
       if test -f conf.autotracetest ; then
        :
       else
          echo "*** Could not run AUTOTRACE test program, checking why..."
          CFLAGS="$CFLAGS $AUTOTRACE_CFLAGS"
          LIBS="$LIBS $AUTOTRACE_LIBS"
          AC_TRY_LINK([
#include <stdio.h>
#include <autotrace/autotrace.h>
],      [ return 0; ],
        [ echo "*** The test program compiled, but did not run. This usually means"
          echo "*** that the run-time linker is not finding AUTOTRACE or finding the wrong"
          echo "*** version of AUTOTRACE. If it is not finding AUTOTRACE, you'll need to set your"
          echo "*** LD_LIBRARY_PATH environment variable, or edit /etc/ld.so.conf to point"
          echo "*** to the installed location  Also, make sure you have run ldconfig if that"
          echo "*** is required on your system"
	  echo "***"
          echo "*** If you have an old version installed, it is best to remove it, although"
          echo "*** you may also be able to get things to work by modifying LD_LIBRARY_PATH"],
        [ echo "*** The test program failed to compile or link. See the file config.log for the"
          echo "*** exact error that occured. This usually means AUTOTRACE was incorrectly installed"
          echo "*** or that you have moved AUTOTRACE since it was installed. In the latter case, you"
          echo "*** may want to edit the autotrace-config script: $AUTOTRACE_CONFIG" ])
          CFLAGS="$ac_save_CFLAGS"
          LIBS="$ac_save_LIBS"
       fi
     fi
     AUTOTRACE_CFLAGS=""
     AUTOTRACE_LIBS=""
     ifelse([$3], , :, [$3])
  fi
  AC_SUBST(AUTOTRACE_CFLAGS)
  AC_SUBST(AUTOTRACE_LIBS)
  rm -f conf.autotracetest
])