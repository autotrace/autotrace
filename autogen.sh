#! /bin/sh

# SPDX-FileCopyrightText: © 2000 Martin Weber
# SPDX-FileCopyrightText: © 2000-2003 Masatake YAMATO
# SPDX-FileCopyrightText: © 2014 Khaled Hosny
# SPDX-FileCopyrightText: © 2021 Ryan Schmidt
#
# SPDX-License-Identifier: LGPL-2.1-or-later

test -n "$srcdir" || srcdir=`dirname "$0"`
test -n "$srcdir" || srcdir=.

cd $srcdir

printf "%s" "checking for pkg-config... "
which pkg-config || {
	echo "*** No pkg-config found, please install it ***"
	exit 1
}

printf "%s" "checking for autoreconf... "
which autoreconf || {
	echo "*** No autoreconf found, please install it ***"
	exit 1
}

printf "%s" "checking for intltoolize... "
which intltoolize || {
	echo "*** No intltoolize found, please install it ***"
	exit 1
}

echo "running autopoint --force"
autopoint --force || exit $?

echo "running autoreconf --force --install --verbose"
AUTOPOINT='intltoolize --automake --copy' autoreconf --force --install --verbose || exit $?
