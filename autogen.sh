#! /bin/sh

srcdir=`dirname $0`
test -z "$srcdir" && srcdir=.
${srcdir}/tools-version.sh

(autofig --version) < /dev/null > /dev/null 2>&1 || {
  echo
  echo "**Error**: You must have \`autofig' installed to compile Autotrace."
  echo "See HACKING in autotrace soruce directory."
  echo "Abort..."
  exit 1
}

autofig autotrace-config.af

if (aclocal-1.5 --version)  < /dev/null > /dev/null 2>&1; then
    aclocal-1.5 $ACLOCAL_FLAGS
else
    aclocal $ACLOCAL_FLAGS
fi

autoheader

if (automake-1.5 --version) < /dev/null > /dev/null 2>&1; then
    automake-1.5 -a
else
    automake -a
fi

libtoolize --copy --force

if (autoconf-2.53 --version) < /dev/null > /dev/null 2>&1; then
    autoconf-2.53
else
    autoconf
fi

echo "checking for glib-gettextize"
(glib-gettextize --version) < /dev/null > /dev/null 2>&1 || {
    echo
    echo "  You must have glib-gettextize installed to compile autotrace"
    echo "  glib-gettextize is part of glib-2.0, so you should already"
    echo "  have it. Make sure it is in your PATH."
    exit 1
}
glib-gettextize --copy --force