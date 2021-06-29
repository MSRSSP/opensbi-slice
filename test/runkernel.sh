set -x
set -e

BIOS_PATH=${TOP}/opensbi/build/platform/generic/firmware/fw_jump.bin
KERNEL_PATH=${TOP}/linux/arch/riscv/boot/Image
QEMU=${TOP}/qemu/build/qemu-system-riscv64
RAM_DISK_PATH=./ramdisk.ext4
DTB_PATH=dtf/sifive_kernel.dtb
SECOND_IMAGE=${KERNEL_PATH}
DEBUG_OPT=""
DEBUG_OPT+="-monitor telnet:127.0.0.1:4322,server,nowait "
#DEBUG_OPT+="-d riscv_hart_lower_mmu -D logs/riscv_hart_lower_mmu.log "
${QEMU} ${DEBUG_OPT} -M sifive_u -m 512M -display none \
	-serial stdio -serial telnet:localhost:4321,server,nowait  \
	-dtb ${DTB_PATH} -smp 5 \
	-bios ${BIOS_PATH} \
	-kernel ${KERNEL_PATH} \
	-drive file=$RAM_DISK_PATH,format=raw,id=hd0 \
	-device loader,file=${SECOND_IMAGE},addr=0x90200000
	
	 