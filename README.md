This implements symbolic exution using STASE (Static Analysis guided Symbolic Execution) to discover vulnerabilities (with vulnerability signature).


## Input
- Source code directory under analysis 
- Entry point and vulnerable instruction location: From static analysis
- Assertion template: Derived for vulnerability type (e.g., OOB_WRITE)

## Output

- Vulnerability Signature: stase_output
- Formatted Output for Mitigations: formatted_output 


## Environment Configuration

Run **once** to set up the environment.

### EDK2
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

### Linux
cd eval2_linux-main
python3 ../stase/process_headers.py $(pwd)


## Build and Run

```
cd stase
chmod +x build_stase.sh
./build_stase.sh
```

## Formated Output
``` 
python3 parse_output.py
```


