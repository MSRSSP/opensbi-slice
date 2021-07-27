CROSS_COMPILE=riscv64-unknown-linux-gnu-
UART_ADDR=0x10011000
all: ramdisk.ext4 hello_world.bin
	make -C dtf

%.elf: %.S
	$(CROSS_COMPILE)gcc -g -nostdlib -nostdinc -e _start -Wl,--build-id=none -Wl,-Ttext=0x80100000 -DUART_SIFIVE -DUART_SIFIVE_BASE=${UART_ADDR} $< -o $@


%.bin: %.elf
	$(CROSS_COMPILE)objcopy -O binary $< $@

clean:
	make -C dtf clean;
	rm -rf *.elf *.bin;
	

