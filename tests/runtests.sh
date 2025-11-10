#!/bin/sh
#
# SPDX-FileCopyrightText: © 2014-2016 Walter Doekes
# SPDX-FileCopyrightText: © 2024 Ryan Carsten Schmidt
#
# SPDX-License-Identifier: LGPL-2.1-or-later
#
# This regression test is a part of Autotrace.
# Author: Walter Doekes, OSSO B.V., 2014
# Taken from https://github.com/SIPp/sipp

# Don't run from anywhere else than here!
cd `dirname "$0"`

# Cleanup test remnants.
find . -name '*.log' -print -delete
echo

# Export custom location Autotrace binary if supplied.
export AUTOTRACE
# Set flag that we want verbose exit codes.
export VERBOSE_EXITSTATUS=1

echo "== RUNNING TESTS =="
ok=0
fail=0
unexpected_ok=0
expected_fail=0
skip=0
for path in github-* other-*; do
    if test -x $path/run; then
        $path/run $path
        ret=$?
        case $ret in
          0) ok=$((ok+1)) ;;
          1) fail=$((fail+1)) ;;
          2) unexpected_ok=$((unexpected_ok+1)) ;;
          3) expected_fail=$((expected_fail+1)) ;;
          4) skip=$((skip+1)) ;;
          *) echo "Unexpected return value: $ret" >&2 && exit 1
        esac
    fi
done

echo "=="
printf "%d success" "$ok"
test $skip -ne 0 && printf ", %d skipped" "$skip"
test $fail -ne 0 && printf ", %d FAILURE" "$fail"
test $unexpected_ok -ne 0 && printf ", %d UNEXPECTED SUCCESS" "$unexpected_ok"
test $expected_fail -ne 0 && printf ", %d expected failure" "$expected_fail"
echo

# Return our status.
test $fail -ne 0 && exit 1
test $unexpected_ok -ne 0 && exit 2
exit 0

# vim: set ts=8 sw=4 sts=4 et ai:
