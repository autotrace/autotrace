#!/bin/bash
# SPDX-License-Identifier: CC0-1.0
# SPDX-FileCopyrightText: 2025 Peter Lemenkov
#
# Script to update the POT file with proper license information

set -e

cd "$(dirname "$0")"

echo "Generating POT file using intltool-update..."

# Use intltool-update - it handles paths correctly
intltool-update --pot --gettext-package=autotrace

# Verify POT was created
if [ ! -f autotrace.pot ]; then
  echo "Error: Failed to generate autotrace.pot"
  exit 1
fi

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
