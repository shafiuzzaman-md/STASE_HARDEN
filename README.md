
## Step-3: Generate ECH
- Process header files for local communication
```
cd eval2_edk2-main
python3 ../process_headers.py $(pwd)
cd ..
```

- Remove macros that are incompatible with symbolic execution.
```
python3 remove_macros.py
```
