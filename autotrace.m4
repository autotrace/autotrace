# a macro to get the libs/cflags for libautotrace
# Copyed from libglade.m4.
# serial 1

dnl AM_PATH_AUTOTRACE([ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND]])
dnl Test to see if libautotrace is installed, and define AUTOTRACE_CFLAGS, LIBS
dnl
AC_DEFUN(AM_PATH_AUTOTRACE,
[dnl
dnl Get the cflags and libraries from the autotrace-config script
dnl
AC_ARG_WITH(autotrace-config,
[  --with-autotrace-config=AUTOTRACE_CONFIG  Location of autotrace-config],
AUTOTRACE_CONFIG="$withval")

AC_PATH_PROG(AUTOTRACE_CONFIG, autotrace-config, no)
AC_MSG_CHECKING(for autotrace)
if test "$AUTOTRACE_CONFIG" = "no"; then
  AC_MSG_RESULT(no)
  ifelse([$2], , :, [$2])
else
  AUTOTRACE_CFLAGS=`$AUTOTRACE_CONFIG --cflags`
  AUTOTRACE_LIBS=`$AUTOTRACE_CONFIG --libs`
  AC_MSG_RESULT(yes)
  ifelse([$1], , :, [$1])
fi
AC_SUBST(AUTOTRACE_CFLAGS)
AC_SUBST(AUTOTRACE_LIBS)
])