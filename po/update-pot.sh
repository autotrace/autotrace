#!/bin/bash
# SPDX-License-Identifier: CC0-1.0
# SPDX-FileCopyrightText: 2025 Peter Lemenkov
#
# Refresh po/autotrace.pot from the sources and add the header information
# Weblate's libre hosting requires.
#
# The strings are extracted by gettext's own po/Makefile rules (settings in
# po/Makevars, file list in po/POTFILES.in), so the tree has to be configured
# once first: ./autogen.sh && ./configure
# (--without-magick --without-pstoedit is enough for this).

set -e

cd "$(dirname "$0")"

if [ ! -f Makefile ]; then
  echo "Error: po/Makefile not found; run ./autogen.sh && ./configure first" >&2
  exit 1
fi

echo "Generating POT file..."
make autotrace.pot-update

# Verify POT was created
if [ ! -f autotrace.pot ]; then
  echo "Error: Failed to generate autotrace.pot"
  exit 1
fi

# xgettext leaves the placeholder charset when no string is non-ASCII, but
# the SPDX header added below is, and msgmerge then rejects the template.
sed -i 's/charset=CHARSET/charset=UTF-8/' autotrace.pot

# Add license information to the POT file header
# This is important for Weblate's Libre hosting compliance
if ! grep -q '"License:' autotrace.pot; then
  echo "Adding license information to POT file..."
  sed -i '/"Content-Transfer-Encoding: 8bit\\n"/a "License: GPL-2.0-or-later\\n"' autotrace.pot
fi

# Fix POT header for REUSE compliance — only if the generic placeholder is present
if grep -q "SOME DESCRIPTIVE TITLE" autotrace.pot; then
  echo "Replacing generic gettext header with SPDX/REUSE header..."
  SPDX_TAG="SPDX-License-Identifier"
  SPDX_CR="SPDX-FileCopyrightText"
  sed -i '1,5d' autotrace.pot
  sed -i "1i\\
# AutoTrace translation template.\\
# ${SPDX_CR}: © 2000-$(date +%Y) AutoTrace contributors\\
# ${SPDX_TAG}: GPL-2.0-or-later\\
#" autotrace.pot
fi

echo "POT file updated successfully!"
echo "Total translatable strings: $(grep -c "^msgid" autotrace.pot)"
