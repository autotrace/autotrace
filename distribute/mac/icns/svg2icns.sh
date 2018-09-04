#!/bin/bash
#
test -n "$BASH_VERSION" || echo "ERROR: Must use bash"
set -o braceexpand
set -x

# 64x64 is not supported by icns.
for i in 16 32 128 256 512; do
  inkscape -z -e AutoTraceIcon.$i.png -w $i -h $i AutoTraceIcon.svg
done

type png2icns 2>/dev/null || echo "ERROR: png2icns not found. Try: sudo apt-get install icnsutils";
png2icns AutoTraceIcon.icns AutoTraceIcon.{16,32,128,256,512}.png
