#!/bin/bash

set -e

# Path to KLEE binary
KLEE="/home/shafi/klee_build/bin/klee"

# Step 1: Set KLEE include directory using LLVM 14 headers
export KLEE_INCLUDE_DIR=$(llvm-config-14 --includedir)
echo "[+] Using KLEE include dir: $KLEE_INCLUDE_DIR"

# Step 2: Clean previous build artifacts
rm -f uaf_driver.bc uaf_lib.bc uaf_combined.bc
rm -rf klee-out-0

# Step 3: Compile instrumented code and driver using clang-14
clang-14 -I"$KLEE_INCLUDE_DIR" -emit-llvm -c -g uaf_driver.c -o uaf_driver.bc
clang-14 -I"$KLEE_INCLUDE_DIR" -emit-llvm -c -g instrumented_code/uaf_lib.c -o uaf_lib.bc

# Step 4: Link bitcode files using llvm-link-14
llvm-link-14 uaf_driver.bc uaf_lib.bc -o uaf_combined.bc

# Step 5: Run KLEE
echo "[+] Running KLEE..."
"$KLEE" --external-calls=all -libc=uclibc --posix-runtime --smtlib-human-readable \
        --write-test-info --write-paths --write-smt2s --write-cov --write-cvcs \
        --write-kqueries --write-sym-paths --only-output-states-covering-new \
        --use-query-log=solver:smt2 --simplify-sym-indices --max-time=5 uaf_combined.bc A

# Step 6: Check output for address reuse info
echo "[+] Address observations:"
grep ADDR klee-out-0/*.txt || echo "No ADDR output found."

# Step 7: List generated test cases
echo "[+] Test cases generated:"
ls klee-out-0/test*.ktest
