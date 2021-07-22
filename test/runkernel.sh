set -x
set -e

BIOS_PATH=${TOP}/opensbi/build/platform/generic/firmware/fw_jump.bin
KERNEL_PATH=${TOP}/linux/arch/riscv/boot/Image
QEMU=${TOP}/qemu/build/qemu-system-riscv64
RAM_DISK_PATH=./ramdisk.ext4
DTB_PATH=dtf/sifive_kernel.dtb
SECOND_IMAGE=${KERNEL_PATH}
HELLO_IMAGE=./hello_world.bin
DEBUG_OPT=""
DEBUG_OPT+="-monitor telnet:127.0.0.1:4322,server,nowait "
#DEBUG_OPT+="--trace memory_region_ops_*"
#DEBUG_OPT+="-d opensbi -D log.txt -s" # localhost:4321 for gdb debug
${QEMU} ${DEBUG_OPT} -M sifive_u -m 1G -display none \
	-serial stdio -serial telnet:localhost:4321,server,nowait  \
	-serial telnet:localhost:4320,server,nowait \
	-dtb ${DTB_PATH} -smp 5 \
	-bios ${BIOS_PATH} \
	-kernel ${KERNEL_PATH} \
	-drive file=$RAM_DISK_PATH,format=raw,id=hd0 \
	-device loader,file=${KERNEL_PATH},addr=0x90200000 
	 