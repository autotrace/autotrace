#!/bin/sh
# This regression test is a part of Autotrace.
# Author: Walter Doekes, OSSO B.V., 2014
# Taken from https://github.com/SIPp/sipp

# Don't run from anywhere else than here!
cd `dirname "$0"`

# Cleanup test remnants.
find . -name '*.log' | xargs --no-run-if-empty -d\\n rm -v
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
echo -n "$ok success"
test $skip -ne 0 && echo -n ", $skip skipped"
test $fail -ne 0 && echo -n ", $fail FAILURE"
test $unexpected_ok -ne 0 && echo -n ", $unexpected_ok UNEXPECTED SUCCESS"
test $expected_fail -ne 0 && echo -n ", $expected_fail expected failure"
echo

# Return our status.
test $fail -ne 0 && exit 1
test $unexpected_ok -ne 0 && exit 2
exit 0

# vim: set ts=8 sw=4 sts=4 et ai:
