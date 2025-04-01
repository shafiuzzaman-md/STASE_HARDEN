# ip253
**Linux Kernel Modules**

This is a kernel module simulates a temperature sensor

# WMIs
- **OOB Write** The last message is stored in "last_message", and it can be overflowed
- **Control Flow Hijacking** "handler" is overflowed then is able to be called

# Intended Solution Path
1. Get shellcode in ktemp_dev.net_creds
2. Overflow the LSB of the 'connect_net' pointer to do_shell
3. Call connect_net
