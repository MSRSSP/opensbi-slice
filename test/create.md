# Create a domain dynamically

1. Run `./runkernel.sh` to launch a host domain using CPU1 and memory (0xa0000000-0xb0000000) and UART1 and a guest domain using CPU(3,4) and memory (0xb0000000-0xb7ffffff) and UART2.
2. In another terminal, run `telnet localhost 4321` to enter the host domain. Then run `./hoststack/create 0x4 0xb8000000 0x08000000` to create a new domain. 
3. Run `./hoststack/info 0` to dump all domain information. The new domain should be "domain 4" using CPU2 and domain memory (0xb8000000 - 0xbfffffff), UART0. 
4. Run `./hoststack/reset 4` to start domain 4. 