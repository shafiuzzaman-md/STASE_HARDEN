
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
## Build and Run

`cd stase`

`chmod +x build_stase.sh`

`./build_stase.sh`

## Output

stase_output/file_name]/signature.txt
