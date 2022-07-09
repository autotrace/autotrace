#! /bin/sh

# SPDX-FileCopyrightText: © 2018 Jürgen Weigert
#
# SPDX-License-Identifier: LGPL-2.1-or-later

#
size=64
inkscape -z -e AutoTraceIcon.$size.png -w $size -h $size ../mac/icns/AutoTraceIcon.svg
convert AutoTraceIcon.$size.png autotrace.ico
rm -f AutoTraceIcon.$size.png


