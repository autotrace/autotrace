#!/bin/sh
# Report the version of distro and tools building autotrace 
# (derived from sodipodi)
#
# You can get the latest distro command from 
# distro web page: http://distro.pipfield.ca/ 

# Please add a tool you want to check
TOOLS="m4 autoconf autoheader automake automake-1.6 aclocal aclocal-1.6 gettextize libtoolize glib-gettextize "
ENVPATTERN='PATH\|FLAGS\|LANG'

srcdir=`dirname $0`
test -z "$srcdir" && srcdir=.

echo '============================================================================='
echo 'When you report a trouble about building CVS version of autotrace            '
echo 'Please report following information about distro and tools version, too.     '
echo 
(echo '--1. distribution------------------------------------------------------------'
$srcdir/distro -a
echo )

(echo '--2. tools-------------------------------------------------------------------'
for x in $TOOLS; do 
    echo "which $x: `which $x`"
    y=`echo $x | cut -f1 -d-`
    $x --version | grep $y
done 
echo )

(echo '--3. environment variables---------------------------------------------------'
env | grep -e $ENVPATTERN
echo )
echo '============================================================================='
