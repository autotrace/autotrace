#! /bin/bash
# Make a windows binary

disttop=$(readlink -e $(dirname $0))	# abspath(dir)
top=$disttop/../..

APP_NAME=$1
APP_VERSION=$2

if [ -z "$APP_VERSION" ]; then
  echo "Usage: $0 APP_NAME APP_VERSION"
  exit 1
fi

win32gcc=x86_64-w64-mingw32-gcc
win32gcc_vers=$($win32gcc -v 2>&1 | grep 'version ')
if [ -z "$win32gcc_vers" ]; then
  echo "ERROR: $win32gcc not found. Cannot crosscompile for windows."
  echo ""
  echo "Try: sudo apt install mingw-w64 binutils-mingw-w64 binutils-mingw-w64-x86-64 mingw-w64-tools"
  exit 1
fi

echo "Cross compiler: $win32gcc_vers"

rm -rf $disttop/glib-2
mkdir -p $disttop/glib-2
cd $disttop/glib-2
unzip -q -o $disttop/3rdparty/glib_2*win64.zip
unzip -q -o $disttop/3rdparty/glib-dev_2*win64.zip

cd $top
make clean
GLIB2_LIBS="-L$disttop/glib-2/lib -lglib-2.0 -lgobject-2.0" GLIB2_CFLAGS="-I$disttop/glib-2/include/glib-2.0 -I$disttop/glib-2/lib/glib-2.0/include" CC="$win32gcc -static" ./configure --host x86_64-w64-mingw32 --without-magick --without-pstoedit
make

cd $disttop
makensis -DVERSION=$APP_VERSION-$(date +%Y%m%d) $APP_NAME.nsi
mkdir -p ../out
zip=../out/$APP_NAME-$APP_VERSION-$(date +%Y%m%d)-win64-setup.zip
rm -f  $zip
zip -r $zip $APP_NAME-*-setup.exe
cp $APP_NAME-*-setup.exe ../out

