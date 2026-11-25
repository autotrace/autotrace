#!/bin/sh
# SPDX-FileCopyrightText: © 2026 AutoTrace contributors
#
# SPDX-License-Identifier: CC0-1.0
#
# Renders the text images of the corpus (issue #387) into DIR, by default
# images/ next to this script.  Their rendering depends on the fonts and on
# FreeType, so the PNGs are committed and this script only records how they
# were made: with Liberation Sans and Serif 2.1.5 (SIL OFL 1.1, which doesn't
# cover documents made with the fonts), ImageMagick 7.1.2 and FreeType 2.13.

set -e
dir=${1:-$(dirname "$0")/images}
font() {
	file=$(fc-match -f '%{file}' "$1")
	case $file in
	*Liberation*) echo "$file" ;;
	*) echo "$0: $1 not installed" >&2; exit 1 ;;
	esac
}
sans=$(font "Liberation Sans")
bold=$(font "Liberation Sans:bold")
serif=$(font "Liberation Serif")
line="Hamburgefonstiv 0123 &@?"

# Glyphs with curves, counters and thin joins, anti-aliased and not.
magick -size 424x48 xc:white -font "$sans" -pointsize 32 -fill black \
	-annotate +8+35 "$line" -depth 8 -type truecolor -strip "$dir/text-sans-aa.png"
magick -size 424x48 xc:white +antialias -font "$sans" -pointsize 32 -fill black \
	-annotate +8+35 "$line" -depth 8 -type truecolor -strip "$dir/text-sans.png"
# Serifs: thin features one or two pixels wide.
magick -size 400x48 xc:white -font "$serif" -pointsize 32 -fill black \
	-annotate +8+35 "$line" -depth 8 -type truecolor -strip "$dir/text-serif-aa.png"
# Small text, where anti-aliasing is most of every glyph.
magick -size 320x44 xc:white -font "$sans" -pointsize 12 -fill black \
	-annotate +6+16 "The quick brown fox jumps over the lazy dog." \
	-annotate +6+34 "Pack my box with five dozen liquor jugs: 0123456789" \
	-depth 8 -type truecolor -strip "$dir/text-small-aa.png"
# Coloured text on a coloured background.
magick -size 304x64 "xc:#fff4c0" -font "$bold" -pointsize 40 -fill "#a00000" \
	-annotate +10+47 "Rag & Bone 42" -depth 8 -type truecolor -strip "$dir/text-colour-aa.png"
