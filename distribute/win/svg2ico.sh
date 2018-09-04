#! /bin/sh
#
size=64
inkscape -z -e AutoTraceIcon.$size.png -w $size -h $size ../mac/icns/AutoTraceIcon.svg
convert AutoTraceIcon.$size.png autotrace.ico
rm -f AutoTraceIcon.$size.png


