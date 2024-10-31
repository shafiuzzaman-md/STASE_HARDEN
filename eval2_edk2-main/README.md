# eval1_edk2

The EDK2 challenge makes use of the EDK2 source,  based on `https://github.com/tianocore/edk2` commit `af8859bc`.

```
sudo apt-get install git gcc g++ make uuid-dev python-is-python3 build-essential nasm iasl libx11-dev libxv-dev gdb gcc-aarch64-linux-gnu libncurses-dev autoconf libssl-dev qemu-system-x86 libbrotli-dev
```

### Build EDK2
1. `source edksetup.sh`
2. `git submodule update --init`
3. `make -C BaseTools`
4. `build -p OvmfPkg/OvmfPkgX64.dsc -t GCC5 -a X64 -D NETWORK_HTTP_BOOT_ENABLE -D NETWORK_TLS_ENABLE -D E1000_ENABLE -D DEBUG_ON_SERIAL_PORT -D IPSEC_ENABLE -D NETWORK_IP6_ENABLE`
