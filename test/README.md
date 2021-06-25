# Boot two domains

## Build device trees and create ramdisk
```bash
make
```

## Tutorial 1: Boot one kernel + one hello world
1. Run
```bash
make
./runhello.sh
```

2. lauch a second terminal;
```bash
telnet localhost 4321
```
After that cmd, you should see kernel booted in first terminal and helloworld in second terminal;

In second term:
```
Trying 127.0.0.1...                                                                                                     
Connected to localhost.                                                                                                 
Escape character is '^]'.                                                                                               
Hello World !               
Hello World !  
Hello World !  
...
```

## Tutorial 2: Boot two kernel in two partitions
**You must build latest `opensbi` in `zz` branch (after commit: 128ab43e55d1e4e11bcd8c29261e3131226b637e).**

0. build latest opensbi
```
cd ../
./make.sh
```

1. Run: change qemu(`QEMU_PATH`), opensbi(`BIOS_PATH`), and kernel(`KERNEL_PATH`) path based on your environment
My kernel is built based on [boot-customized-riscv-linux-kernel-using-qemu](https://github.com/MSRSSP/hardware-partition-docs/blob/main/boot-tutorial.md#option-2-boot-customized-riscv-linux-kernel-using-qemu)

```bash
cd test
make
./runkernel.sh
```

2. lauch a second terminal;
```bash
telnet localhost 4321
```
After that cmd, you should see kernel booted in both terminal (a tiny delay in second term);