#! /bin/sh

(autofig --version) < /dev/null > /dev/null 2>&1 || {
  echo
  echo "**Error**: You must have \`autofig' installed to compile Autotrace."
  echo "See HACKING in autotrace soruce directory."
  echo "Abort..."
  exit 1
}

autofig autotrace-config.af
aclocal $ACLOCAL_FLAGS
autoheader
automake -a
autoconf
