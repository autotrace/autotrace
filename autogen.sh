#! /bin/sh

(autofig --version) < /dev/null > /dev/null 2>&1 || {
  echo
  echo "**Error**: You must have \`autofig' installed to compile Autotrace."
  echo "See HACKING in autotrace soruce directory."
  echo "Abort..."
  exit 1
}

autofig autotrace-config.af

if (aclocal-1.4 --version)  < /dev/null > /dev/null 2>&1; then
    aclocal-1.4 $ACLOCAL_FLAGS
else
    aclocal $ACLOCAL_FLAGS
fi

autoheader

if (automake-1.5 --version) < /dev/null > /dev/null 2>&1; then
    automake-1.5 -a
else
    automake -a
fi

if (autoconf-2.53 --version) < /dev/null > /dev/null 2>&1; then
    autoconf-2.53
else
    autoconf
fi

