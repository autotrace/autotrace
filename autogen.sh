#! /bin/sh
aclocal $ACLOCAL_FLAGS
automake -a
autoconf
autofig autotrace-config.af

