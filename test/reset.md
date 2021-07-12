## System-wise reset test
* Checkout zz-dynamic branch @74c6fa1261a0009f2d24c2050d5cd36c9a4316b0
* Checkout qemu zz-reset branch @85d575c5c6d8cd36bb896218c456816c005753dc
The modified hw/riscv/sifive_u.c registers a reset unit at 0x100000. 