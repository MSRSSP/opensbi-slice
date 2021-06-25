set -x
set -e

BIOS_PATH=${TOP}/opensbi/build/platform/generic/firmware/fw_jump.bin
KERNEL_PATH=./riscv_kernel_image.bin
QEMU=${TOP}/qemu/build/qemu-system-riscv64
RAM_DISK_PATH=./ramdisk.bin
DTB_PATH=dtf/sifive_kernel.dtb
SECOND_IMAGE=${KERNEL_PATH}
${QEMU} -M sifive_u -m 512M -display none \
	-serial stdio -serial telnet:localhost:4321,server  \
	-dtb ${DTB_PATH} -smp 5 \
	-bios ${BIOS_PATH} \
	-kernel ${KERNEL_PATH} \
	-drive file=$RAM_DISK_PATH,format=raw,id=hd0 \
	-device loader,file=${SECOND_IMAGE},addr=0x90200000 \
	 -monitor telnet:127.0.0.1:4322,server,nowait;