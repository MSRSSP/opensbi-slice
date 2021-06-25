all: ramdisk
	make -C dtf
	./compile_hello.sh

ramdisk: ramdisk.bin
	dd if=/dev/zero of=ramdisk.bin bs=1M count=16
	mkfs.ext4 -j -F ramdisk.bin

clean:
	rm *.elf
	rm *.bin
