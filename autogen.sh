#! /bin/sh

test -n "$srcdir" || srcdir=`dirname "$0"`
test -n "$srcdir" || srcdir=.

cd $srcdir

echo -n "checking for pkg-config... "
which pkg-config || {
	echo "*** No pkg-config found, please install it ***"
	exit 1
}

echo -n "checking for autoreconf... "
which autoreconf || {
	echo "*** No autoreconf found, please install it ***"
	exit 1
}


echo -n "checking for intltoolize... "
which intltoolize || {
	echo "*** No intltoolize found, please install it ***"
	exit 1
}

echo "running autopoint --force"
autopoint --force || exit $?

echo "running autoreconf --force --install --verbose"
AUTOPOINT='intltoolize --automake --copy' autoreconf --force --install --verbose || exit $?
