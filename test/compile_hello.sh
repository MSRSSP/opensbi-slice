set -x
set -e

CROSS_COMPILE=riscv64-linux-gnu-
${CROSS_COMPILE}gcc -g -nostdlib -nostdinc -e _start -Wl,--build-id=none -Wl,-Ttext=0x80100000 -DUART_SIFIVE -DUART_SIFIVE_BASE=0x10011000 hello_world.S -o hello_world.elf
${CROSS_COMPILE}objcopy -O binary hello_world.elf hello_world.bin
