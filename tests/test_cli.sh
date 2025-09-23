#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."

pass=0
fail=0

assert_contains() {
  local haystack="$1"
  local needle="$2"
  local msg="$3"
  if echo "$haystack" | grep -Fq "$needle"; then
    echo "[PASS] $msg"
    pass=$((pass+1))
  else
    echo "[FAIL] $msg"
    echo "  Expected to find: $needle"
    fail=$((fail+1))
  fi
}

###############################################################################
# Test 1: simple declarations with initialization and update (stdin mode)
###############################################################################
code1='int x = 5; x = x + 3; char[10] name; char * A;'
out1=$(printf "%s\n" "$code1" | ./br)
assert_contains "$out1" "int x = 5;" "t1: shows command int x = 5;"
assert_contains "$out1" "S = {x |-> 5}" "t1: binding shows x -> 5 after first row"
assert_contains "$out1" "x = x + 3;" "t1: shows x = x + 3; row"
assert_contains "$out1" "S = {x |-> 8}" "t1: binding shows x -> 8"
assert_contains "$out1" "char [10] name;" "t1: shows char array declaration"
assert_contains "$out1" "Stack evolution by step:" "t1: has stack evolution section"

###############################################################################
# Test 2: while-loop iteration rows and stack values (stdin mode)
###############################################################################
code2='int i; int x; i = 4; x = 3; while (i < 7) { x = x + i; i = i + 2; }'
out2=$(printf "%s\n" "$code2" | ./br)
assert_contains "$out2" "iter 1: x = x + i;" "t2: first iter body row present"
assert_contains "$out2" "iter 2: i = i + 2;" "t2: second iter body row present"
assert_contains "$out2" "x = 13" "t2: stack diagram shows x = 13 final value"

echo
echo "Passed: $pass, Failed: $fail"
if [ "$fail" -gt 0 ]; then
  exit 1
fi

