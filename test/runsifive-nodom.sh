set -x
set -e

BIOS_PATH=${TOP}/opensbi/build/platform/generic/firmware/fw_dynamic.bin
KERNEL_PATH=${TOP}/linux/arch/riscv/boot/Image
QEMU=${TOP}/qemu/build/qemu-system-riscv64
RAM_DISK_PATH=./ramdisk.ext4
DTB_PATH=dtf/sifive_u.dtb
SECOND_IMAGE=${KERNEL_PATH}
DEBUG_OPT=""
DEBUG_OPT+="-monitor telnet:127.0.0.1:4322,server,nowait "
#DEBUG_OPT+="--trace memory_region_ops_*"
#DEBUG_OPT+="-d exec -D log.txt"
${QEMU} ${DEBUG_OPT} -M sifive_u -nographic  \
	-smp 5 \
	-dtb ${DTB_PATH} -smp 5 \
	-kernel ${KERNEL_PATH} \
	-drive file=$RAM_DISK_PATH,format=raw,id=hd0
	 