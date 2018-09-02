#! /bin/bash
# Make a debian/ubuntu distribution

name=$1
vers=$2
url=http://github.com/autotrace/autotrace
requires="libpstoedit0c2a,libbz2-1.0,libgd3,libexif12,libtiff5,imagemagick"

# libtool must not call ranlib when installing in fakeroot
export PATH=$(pwd):$PATH

if [ -n "$(../../libtool --config | grep host_os | grep mingw)" ]; then
  echo "ERROR: configured for mingw cross compiler."
  echo "Please rerun: ./configure; make clean; make"
  exit 1
fi


mkdir -p doc-pak
cp ../../ChangeLog 	doc-pak/
cp ../../COPYING	doc-pak/
cp ../../COPYING.LIB	doc-pak/
cp ../../README.md	doc-pak/README
cp ../../THANKS		doc-pak/

tmp=../out

[ -d $tmp ] && rm -rf $tmp/*.deb
mkdir -p $tmp
fakeroot checkinstall --fstrans --reset-uid --type debian \
  --install=no -y --pkgname $name --pkgversion $vers --pkgrelease $(date +%Y%m%d) --arch all \
  --pkglicense GPL --pkggroup other --pakdir $tmp --pkgsource $url \
  --maintainer "'Juergen Weigert (juewei@fabmail.org)'" \
  --requires "'$requires'" make -C ../.. install \
  -e PREFIX=/usr || { echo "error"; exit 1; }

rm -rf doc-pak
for pkg in $tmp/*.deb; do dpkg-deb --info $pkg; done
