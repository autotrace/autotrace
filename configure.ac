dnl SPDX-FileCopyrightText: © 2014 Khaled Hosny
dnl SPDX-FileCopyrightText: © 2017-2025 Peter Lemenkov
dnl SPDX-FileCopyrightText: © 2018 Juergen Weigert
dnl SPDX-FileCopyrightText: © 2018 Jürgen Weigert
dnl SPDX-FileCopyrightText: © 2020 Sacha Chua
dnl SPDX-FileCopyrightText: © 2023 Mamoru TASAKA
dnl SPDX-FileCopyrightText: © 2024 Rui Chen
dnl SPDX-FileCopyrightText: © 2024 Ryan Carsten Schmidt
dnl
dnl SPDX-License-Identifier: LGPL-2.1-or-later

# configure.ac for autotrace
#
# Author: Masatake YAMATO (yamato@redhat.com)
#
AC_INIT([autotrace],[0.31.10])
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
LT_INIT

# Try to set C17 standard
AC_MSG_CHECKING([for C17 support])
OLD_CFLAGS="$CFLAGS"
CFLAGS="$CFLAGS -std=c17"
AC_COMPILE_IFELSE(
    [AC_LANG_PROGRAM([[]], [[]])],
    [AC_MSG_RESULT([yes])],
    [AC_MSG_RESULT([no])
     AC_MSG_ERROR([C17 support is required but not available])]
)

# Make C17 standard required
AC_SUBST([AM_CFLAGS], ["-std=c17"])

PKG_PROG_PKG_CONFIG

AUTOTRACE_WEB=https://github.com/autotrace/autotrace
AC_DEFINE_UNQUOTED(AUTOTRACE_WEB, "$AUTOTRACE_WEB", [Project home page])

AM_MAINTAINER_MODE

AC_CONFIG_HEADERS(config.h)

AC_PROG_CPP
dnl AC_PROG_RANLIB
dnl AC_CANONICAL_HOST
dnl AC_ARG_PROGRAM
AM_SANITY_CHECK

AC_PROG_INSTALL
AC_CHECK_HEADERS(malloc.h)

IT_PROG_INTLTOOL([0.50.1])
AM_GNU_GETTEXT_VERSION([0.19.3])
AM_GNU_GETTEXT([external])
AC_SUBST([GETTEXT_PACKAGE], [$PACKAGE_NAME])
AC_DEFINE_UNQUOTED([GETTEXT_PACKAGE], "$GETTEXT_PACKAGE", [Define to the gettext package name.])
ALL_LINGUAS="ja de"
AM_GLIB_GNU_GETTEXT

AC_CHECK_FUNCS([localtime_r])

dnl
dnl GraphicsMagick
dnl
AC_ARG_WITH(magick,
  [AS_HELP_STRING([--with-magick=LIB],
   [select library to use for images (options: GraphicsMagick (default) or ImageMagick (<= 5.5.2))])],
   ,[with_magick=GraphicsMagick])

HAVE_MAGICK=yes
HAVE_MAGICK_READERS=no
MAGICK_LIBRARY=none
if test x"${with_magick}" = xno; then
  AC_MSG_WARN([*** GraphicsMagick or ImageMagick input handler is disabled (NOT RECOMMENDED).           ***])
  HAVE_MAGICK=no
else
  if test "x${with_magick}" = xyes; then
    with_magick=GraphicsMagick
  fi
  if test "x${with_magick}" = xGraphicsMagick; then
    PKG_CHECK_MODULES([GRAPHICSMAGICK],
      [GraphicsMagick >= 1.3.40],
      [HAVE_MAGICK=yes
       HAVE_GRAPHICSMAGICK=yes
       MAGICK_LIBRARY=GraphicsMagick
       AC_DEFINE(HAVE_GRAPHICSMAGICK, 1, [GraphicsMagick library is available.])
	 ],
      AC_MSG_WARN(GraphicsMagick >= 1.3.40 is not available. Falling back to ImageMagick...)
      [with_magick=ImageMagick]
    )
  fi
  if test "x${with_magick}" = xImageMagick; then
      PKG_CHECK_MODULES([IMAGEMAGICK7],
          [ImageMagick >= 7.0.1],
	 [HAVE_IMAGEMAGICK7=yes
	  AC_DEFINE(HAVE_IMAGEMAGICK7, 1, [ImageMagick version 7 or higher is available.])
	 ],[]
	)
      PKG_CHECK_MODULES([IMAGEMAGICK],
         [ImageMagick >= 5.2.1],
	 [HAVE_MAGICK=yes
	  HAVE_IMAGEMAGICK=yes
	  MAGICK_LIBRARY=ImageMagick
	  AC_DEFINE(HAVE_IMAGEMAGICK, 1, [ImageMagick library is available.])
	 ],
	AC_MSG_ERROR([Neither GraphicsMagick nor ImageMagick are available. Try recompiling with --without-magick.])
	)
  fi
  if test "x${HAVE_MAGICK}" = xyes; then
    AC_DEFINE(HAVE_MAGICK, 1, [GraphicsMagick or ImageMagick library is available or not])
	AC_ARG_ENABLE([magick-readers],
		      AS_HELP_STRING([--disable-magick-readers], [Don't use *Magick as a reader for BMP/PGM/PNM/PPM/TGA files])
	)
	AS_IF([test "x$enable_magick_readers" != "xno"], [
		   HAVE_MAGICK_READERS=yes
		   AC_DEFINE(HAVE_MAGICK_READERS, 1, [Use Magick readers for simple formats.])
	])
  fi
fi

AM_CONDITIONAL(HAVE_MAGICK, test "x$HAVE_MAGICK" = xyes)
AM_CONDITIONAL(HAVE_IMAGEMAGICK, test "x$HAVE_IMAGEMAGICK" = xyes)
AM_CONDITIONAL(HAVE_GRAPHICSMAGICK, test "x$HAVE_GRAPHICSMAGICK" = xyes)
AM_CONDITIONAL(HAVE_MAGICK_READERS, test "x$HAVE_MAGICK_READERS" = xyes)

dnl
dnl LIBPNG
dnl

AC_ARG_WITH(png,
[  --with-png  compile with png (default)
  --without-png  compile without png],,with_png=yes)

HAVE_LIBPNG=no
if test "x${with_png}" = xyes; then
	PKG_CHECK_MODULES([LIBPNG],[libpng],
		  [
		   HAVE_LIBPNG=yes
		   AC_DEFINE(HAVE_LIBPNG,1,[Define to 1 if the PNG library is available])
		   ],
		   [
		    AC_MSG_WARN([*** Cannot find libpng.            ***])
		    AC_MSG_WARN([*** PNG input handler is disabled. ***])
		    ]
		  )
else
		AC_MSG_WARN([*** libpng input handler is disabled.           ***])
fi
AM_CONDITIONAL(HAVE_LIBPNG, test $HAVE_LIBPNG = yes)

dnl
dnl pstoedit library
dnl

HAVE_LIBPSTOEDIT=no
AC_ARG_WITH(pstoedit,
[  --with-pstoedit link with pstoedit library (default)
  --without-pstoedit do not link with pstoedit library],,with_pstoedit=yes)
if test "x${with_pstoedit}" = xyes; then
	PKG_CHECK_MODULES([LIBPSTOEDIT],
			  [pstoedit >= 3.32.0],
			  [
			   HAVE_LIBPSTOEDIT=yes
			   AC_DEFINE(HAVE_LIBPSTOEDIT,1,[Define to 1 if the pstoedit library is available])
			   ],
			  )
else
AC_MSG_WARN([pstoedit output functions are disabled from command line.])
fi
AM_CONDITIONAL(HAVE_LIBPSTOEDIT, test x$HAVE_LIBPSTOEDIT = xyes)

dnl
dnl GLib2
dnl

PKG_CHECK_MODULES(GLIB2, glib-2.0 >= 2.44 gmodule-2.0 >= 2.44 gthread-2.0 >= 2.44 gobject-2.0 >= 2.44,
                  glib_ok=yes, glib_ok=no)
if test "x${glib_ok}" != "xyes"; then
   AC_MSG_ERROR([cannot find glib-2.0 >= 2.44 (required for g_autoptr support)])
else
   :
fi

dnl po/Makefile.in FIXME FIXME FIXME
AC_CONFIG_FILES([po/Makefile.in Makefile autotrace.pc])
AC_OUTPUT

echo "

Configuration:
	graphics/imagemagick output support:	$HAVE_MAGICK ($MAGICK_LIBRARY) (magick readers only: $HAVE_MAGICK_READERS)
	libpng output support:			$HAVE_LIBPNG
	pstoedit output support:		$HAVE_LIBPSTOEDIT

"
