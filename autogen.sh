#! /bin/sh
autofig autotrace-config.af
aclocal $ACLOCAL_FLAGS
automake -a
autoconf


