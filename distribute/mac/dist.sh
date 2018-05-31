#! /bin/sh
#
disttop=$(dirname $0)
APP_NAME=$1
APP_VERSION=$2

contents=$disttop/$APP_NAME.app/Contents

rm -rf $contents                # clean any left-overs
mkdir -p $contents/{MacOS,Resources,Frameworks}

# collect binaries and libraries
cp -a $disttop/../../.libs/$APP_NAME $contents/MacOS/
cp -a $disttop/../../.libs/lib*.dylib $contents/Frameworks/

bundlelibs()
{
  bin=$1
  dir=$2
  relink=$3

  for libpath in $(otool -L $bin | grep /usr/local/ | awk '{ print $1 }'); do
    lib=$(basename $libpath)
    test -f $libpath -a ! -e $dir/$lib && cp $libpath $dir/$lib
    chmod 644 $dir/$lib
    install_name_tool -change $libpath "$relink/$lib" $bin
  done
}

# @executable_path/../Frameworks looks like a hack, but works fine. Is @rpath better?
bundlelibs $contents/MacOS/$APP_NAME $contents/Frameworks @executable_path/../Frameworks
for bundledlib in $contents/Frameworks/*; do
  bundlelibs $bundledlib $contents/Frameworks @loader_path
done

# collect all other (arch-independant) files
cp -a $disttop/icns/*.icns $contents/Resources/
cp -a $disttop/../../COPYING* $contents/Resources/

# sed multiline idea seen in
# https://stackoverflow.com/questions/17671392/sed-include-newline-in-pattern
# does not work on osx sed. That silly sed fails to match .* then.
sed -e 's@APP_VERSION@'$APP_VERSION'@' < $APP_NAME-Info.plist > $contents/Info.plist

cd $disttop
mkdir -p ../out
zip=../out/$APP_NAME-$APP_VERSION-$(date +%Y%m%d)-MacOS.zip
rm -f  $zip
zip -r $zip $APP_NAME.app

