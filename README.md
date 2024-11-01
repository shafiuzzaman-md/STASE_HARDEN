
## Generate ECH
- Process header files for local communication
```
cd eval2_edk2-main
python3 ../stase/process_headers.py $(pwd)
cd ..
```

- Remove macros that are incompatible with symbolic execution.
```
python3 stase/remove_macros.py
```
## Compiling and Running

`cd stase`

`chmod +x build_stase.sh`

`./build_stase.sh`

`klee --external-calls=all -libc=uclibc --posix-runtime --smtlib-human-readable --write-test-info --write-paths --write-smt2s --write-cov --write-cvcs --write-kqueries --write-sym-paths --only-output-states-covering-new --use-query-log=solver:smt2 --simplify-sym-indices stase.bc > staseOutput.txt 2>&1` 

`python3 extract_signature.py`
