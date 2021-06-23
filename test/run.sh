set -x
set -e

BIOS_PATH=../build/platform/generic/firmware/fw_jump.bin
KERNEL_PATH=../../linux/arch/riscv/boot/Image
INITRD_PATH=rootfs_riscv64.img

qemu-system-riscv64 -M sifive_u -m 256M -display none \
	-serial stdio -serial telnet:localhost:4321,server,nowait \
	-dtb sifive_u.dtb -smp 5 \
	-bios ${BIOS_PATH} \
	-kernel ${KERNEL_PATH} \
	-append "root=/dev/ram rw console=ttySIF0 earlycon=sbi"
	#-device loader,file=./hello_world.bin,addr=0x80100000
	#-initrd <initrd_build_path>/rootfs_riscv64.img
