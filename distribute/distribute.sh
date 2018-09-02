#!/bin/bash

cd $(dirname $0)

echo "Determining Version:"
PKG_VER=$( (egrep '_VERSION=' ../configure.ac ;echo 'echo $AUTOTRACE_VERSION') | sh)
echo "Version is: \"$PKG_VER\""
PKG_NAME=autotrace

test -f /etc/os-release && . /etc/os-release
if [ "$ID_LIKE" = ubuntu -o "$ID_LIKE" = debian -o "$_system_name" = Ubuntu ]; then
  echo "****************************************************************"
  echo "Ubuntu Version: using checkinstall and dpkg"
  (cd deb && sh ./dist.sh $PKG_NAME $PKG_VER) || exit 1
  echo "****************************************************************"
  echo "Win64 Version: using x86_64-w64-mingw32 cross toolchain"
  (cd win && sh ./dist.sh $PKG_NAME $PKG_VER)
elif [ "$(uname)" = Darwin ]; then
  echo "++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++"
  echo "MacOS Version: using zip"
  (cd mac && sh ./dist.sh $PKG_NAME $PKG_VER)
else
  echo "................................................................"
  echo "ERROR: unknown system, neither Ubunut nor MacOS."
  exit 1
fi

echo "Built packages are in distribute/out :"
ls -la out
echo "done."
