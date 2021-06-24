set -x
set -e

BIOS_PATH=${TOP}/opensbi/build/platform/generic/firmware/fw_jump.bin
KERNEL_PATH=${TOP}/linux/arch/riscv/boot/Image
RAM_DISK_PATH=$TOP/ramdisk.bin
DTB_PATH=dtf/sifive_u.dtb

qemu-system-riscv64 -M sifive_u -m 256M -display none \
	-serial stdio -serial telnet:localhost:4321,server,nowait \
	-dtb ${DTB_PATH} -smp 5 \
	-bios ${BIOS_PATH} \
	-kernel ${KERNEL_PATH} \
	-drive file=$RAM_DISK_PATH,format=raw,id=hd0 \
	-device loader,file=./hello_world.bin,addr=0x80100000