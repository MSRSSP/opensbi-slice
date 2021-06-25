set -x
set -e

BIOS_PATH=${TOP}/opensbi/build/platform/generic/firmware/fw_jump.bin
KERNEL_PATH=${TOP}/linux/arch/riscv/boot/Image
RAM_DISK_PATH=$TOP/ramdisk.bin
DTB_PATH=dtf/sifive_u.dtb
QEMU=${TOP}/qemu/build/qemu-system-riscv64
${QEMU} -M sifive_u -m 1G -display none \
	-serial stdio -serial telnet:localhost:4321,server  \
	-dtb ${DTB_PATH} -smp 5 \
	-bios ${BIOS_PATH} \
	-kernel ${KERNEL_PATH} \
	-drive file=$RAM_DISK_PATH,format=raw,id=hd0 \
	-device loader,file=${KERNEL_PATH},addr=0x8d200000 \
	 -monitor telnet:127.0.0.1:4322,server,nowait;