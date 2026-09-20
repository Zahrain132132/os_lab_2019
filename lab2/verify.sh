#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")"
export LD_LIBRARY_PATH="$PWD/build${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
test "$(build/swap)" = 'b a'
for app in revert_direct revert_static revert_dynamic; do
    for input in '' 'a' 'abc' 'abcd' 'Hello' 'String with spaces'; do
        expected=$(printf '%s' "$input" | rev)
        test "$("build/$app" "$input")" = "Reverted: $expected"
    done
    if "build/$app" > /dev/null 2>&1; then exit 1; fi
    if "build/$app" one two > /dev/null 2>&1; then exit 1; fi
done
build/tests
app_lib=$(ldd build/revert_dynamic | awk '/librevert.so/ {print $3}')
test_lib=$(ldd build/tests | awk '/librevert.so/ {print $3}')
test "$(readlink -f "$app_lib")" = "$PWD/build/librevert.so"
test "$(readlink -f "$test_lib")" = "$PWD/build/librevert.so"
if readelf -d build/revert_static | grep -q librevert; then exit 1; fi
printf 'PASS: swap; 18 string cases; 6 argument errors; CUnit; shared library identity\n'
