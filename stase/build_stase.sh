#!/bin/bash

# Stop the execution if any command fails
set -e

# Enable shell debugging
set -x

# Use the full path to the klee executable
KLEE="/home/shafi/klee_build/bin/klee"

# Define an array of source files
sources=("test_SmmLegacyDispatcher.c" "test_B2SmiHandler.c" "test_B2SmiHandler1.c" "test_RWVariableHandler.c" "test_kbmi_usb.c")

# Loop over each source file
for src in "${sources[@]}"; do
    echo "Processing $src"

    # Define filenames
    base_name="${src%.c}"
    bc_file="${base_name}.bc"
    output_file="${base_name}_output.txt"

    # Step 1: Compile the C file into LLVM bitcode
    clang-14 -emit-llvm -c -g -O0 -Xclang -disable-O0-optnone "$src" -o "$bc_file"

    # Step 2: Run KLEE on the LLVM bitcode
    "$KLEE" --external-calls=all -libc=uclibc --posix-runtime --smtlib-human-readable \
        --write-test-info --write-paths --write-smt2s --write-cov --write-cvcs \
        --write-kqueries --write-sym-paths --only-output-states-covering-new \
        --use-query-log=solver:smt2 --simplify-sym-indices --max-time=60 "$bc_file" > "$output_file" 2>&1

    # Step 3: Run the Python script to extract the signature
    python3 extract_signature.py "$src"

    echo "Finished processing $src"
done
