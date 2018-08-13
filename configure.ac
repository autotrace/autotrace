# configure.in for frontline
#
# Author: Masatake YAMATO (yamato@redhat.com)
#
AC_INIT([autotrace], [0.40.0])
AC_CONFIG_SRCDIR([src/main.c])
AM_INIT_AUTOMAKE([subdir-objects])
AC_CONFIG_MACRO_DIR([m4])

PACKAGE=autotrace
#
# version setting up for Automake
#
MAJOR_VERSION=0
MINOR_VERSION=40
MICRO_VERSION=0
dnl VERSION is not acceptable; it conflicts
dnl with a variable defined in VC++.
AUTOTRACE_VERSION=${MAJOR_VERSION}.${MINOR_VERSION}.${MICRO_VERSION}
AC_DEFINE_UNQUOTED(PACKAGE, "$PACKAGE", [Package name])
AC_SUBST(PACKAGE)
AC_DEFINE_UNQUOTED(AUTOTRACE_VERSION,"$AUTOTRACE_VERSION", [Package version])
AC_SUBST(AUTOTRACE_VERSION)

#
# version setting up for libtool
#
LT_CURRENT=3
LT_REVISION=0
LT_AGE=0
dnl AC_SUBST(LT_RELEASE)
AC_SUBST(LT_CURRENT)
AC_SUBST(LT_REVISION)
AC_SUBST(LT_AGE)
dnl AC_SUBST(LT_CURRENT_MINUS_AGE)
AC_PROG_CC
AC_PROG_LIBTOOL

PKG_PROG_PKG_CONFIG

AUTOTRACE_WEB=http://autotrace.sourceforge.net
AC_DEFINE_UNQUOTED(AUTOTRACE_WEB, "$AUTOTRACE_WEB", [Project home page])

AM_MAINTAINER_MODE

AM_CONFIG_HEADER(config.h)

AC_PROG_CPP
dnl AC_PROG_RANLIB
dnl AC_CANONICAL_HOST
dnl AC_ARG_PROGRAM
AM_SANITY_CHECK

AC_PROG_INSTALL
AC_CHECK_HEADERS(malloc.h)

IT_PROG_INTLTOOL([0.50.1])
AM_GNU_GETTEXT_VERSION([0.18])
AM_GNU_GETTEXT([external])
AC_SUBST([GETTEXT_PACKAGE], [$PACKAGE_NAME])
AC_DEFINE_UNQUOTED([GETTEXT_PACKAGE], "$GETTEXT_PACKAGE", [Define to the gettext package name.])
ALL_LINGUAS="ja de"
AM_GLIB_GNU_GETTEXT

dnl
dnl ImageMagick
dnl
AC_ARG_WITH(magick,
[  --with-magick  compile with maigck(default)
  --without-magick  compile without maigck],,with_magick=yes)

HAVE_MAGICK=no
if test "x${with_magick}" = xyes; then
	PKG_CHECK_MODULES([MAGICK],
			  [ImageMagick >= 5.2.1],
			  [
			   HAVE_MAGICK=yes
			   AC_DEFINE(HAVE_MAGICK,1,[ImageMagick library is available or not])
			   ],
			  )
else
		AC_MSG_WARN([*** ImageMagick input handler is disabled.           ***])
fi
AM_CONDITIONAL(HAVE_MAGICK, test $HAVE_MAGICK = yes)

dnl
dnl LIBPNG
dnl

HAVE_LIBPNG=no
PKG_CHECK_MODULES([LIBPNG],[libpng],
		  [
		   HAVE_LIBPNG=yes
		   AC_DEFINE(HAVE_LIBPNG,1,[PNG library is available or not])
		   ],
		   [
		    AC_MSG_WARN([*** Cannot find libpng.            ***])
		    AC_MSG_WARN([*** Png input handler is disabled. ***])
		    ]
		  )
AM_CONDITIONAL(HAVE_LIBPNG, test $HAVE_LIBPNG = yes)

dnl
dnl LIBMING(LIBSWF)
dnl

HAVE_LIBSWF=no
AC_CHECK_HEADER(ming.h,
	        swf_header_found=yes,
		swf_header_found=no)
if test "${swf_header_found}" = yes ; then
	PKG_CHECK_MODULES([LIBSWF],
			  [libming],
			  [
			   HAVE_LIBSWF=yes
			   AC_DEFINE(HAVE_LIBSWF,1,[SWF library is available or not])
			   ],
			  )
else
	AC_MSG_WARN([*** Cannot find ming.h.     ***])
	AC_MSG_WARN([*** SWF output is disabled. ***])
fi
AM_CONDITIONAL(HAVE_LIBSWF, test x$HAVE_LIBSWF = xyes)

dnl
dnl pstoedit library
dnl

HAVE_LIBPSTOEDIT=no
AC_ARG_WITH(pstoedit,
[  --with-pstoedit link with pstoedit library(default)
  --without-pstoedit link without pstoedit library],,with_pstoedit=yes)
if test "x${with_pstoedit}" = xyes; then
	PKG_CHECK_MODULES([LIBPSTOEDIT],
			  [pstoedit >= 3.32.0],
			  [
			   HAVE_LIBPSTOEDIT=yes
			   AC_DEFINE(HAVE_LIBPSTOEDIT,1,[pstoedit library is available or not])
			   ],
			  )
else
AC_MSG_WARN([pstoedit output functions are disabled from command line.])
fi
AM_CONDITIONAL(HAVE_LIBPSTOEDIT, test x$HAVE_LIBPSTOEDIT = xyes)

dnl
dnl GLib2
dnl

PKG_CHECK_MODULES(GLIB2, glib-2.0 >= 2.0  gmodule-2.0 >= 2.0 gthread-2.0 >= 2.0 gobject-2.0 >= 2.0, 
	          glib_ok=yes, glib_ok=no)
if test "x${glib_ok}" != "xyes"; then
   AC_MSG_ERROR([cannot find glib-2.0])
else
   :
fi

dnl po/Makefile.in FIXME FIXME FIXME
AC_OUTPUT([po/Makefile.in Makefile autotrace.pc])

echo "

Configuration:
	imagemagick output support:	$HAVE_MAGICK
	libpng output support:		$HAVE_LIBPNG
	swf output support by ming:	$HAVE_LIBSWF
	pstoedit output support:	$HAVE_LIBPSTOEDIT

"
