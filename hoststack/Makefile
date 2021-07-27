CROSS_COMPILE=riscv64-unknown-linux-gnu-
PROGRAM ?= reset
HOST_INITRAM=${PWD}/host_initramfs
GUEST_INITRAM=${PWD}/guest_initramfs
LINUX_DIR=${TOP}/linux
TARGETS=
TARGETS+=${PROGRAM}
TARGETS+=${GUEST_INITRAM}.cdio
TARGETS+=${HOST_INITRAM}.cdio
HOST_PROGRAMS=$(PROGRAM)
$(PROGRAM): $(wildcard *.c) $(wildcard *.h) $(wildcard *.S)
	$(CROSS_COMPILE)gcc $(CFLAGS) $(LDFLAGS) $(filter %.c %.S,$^) $(LOADLIBES) $(LDLIBS) -o $@

guest_initramfs.cpio:
	wget https://github.com/lowRISC/lowrisc-chip/releases/download/v0.6-rc3/initramfs.cpio 
	mv initramfs.cpio guest_initramfs.cpio 

host_initramfs: guest_initramfs.cpio
	rm -rf ${HOST_INITRAM}
	mkdir ${HOST_INITRAM}
	echo "cd host_initramfs;\
	cpio -i < ../guest_initramfs.cpio; "|fakeroot
	mkdir ${HOST_INITRAM}/hoststack/;

host_initramfs.cpio: host_initramfs $(HOST_PROGRAMS)
	echo "ls; cp reset ${HOST_INITRAM}/hoststack/reset" | fakeroot
	echo "cd ${LINUX_DIR};\
		usr/gen_initramfs.sh -o ${HOST_INITRAM}.cpio ${HOST_INITRAM}" | fakeroot

host_kernel_image: host_initramfs.cpio ${LINUX_DIR}
	echo "cd ${LINUX_DIR};\
	make ARCH=riscv CROSS_COMPILE=riscv64-unknown-linux-gnu- \
	CONFIG_INITRAMFS_SOURCE=${HOST_INITRAM}.cpio -j16;" | sh
	cp ${LINUX_DIR}/arch/riscv/boot/Image $@
	

guest_kernel_image: guest_initramfs.cpio
	echo "cd ${LINUX_DIR};\
	make ARCH=riscv CROSS_COMPILE=riscv64-unknown-linux-gnu- \
	CONFIG_INITRAMFS_SOURCE=${GUEST_INITRAM}.cpio -j16;" | sh
	cp ${LINUX_DIR}/arch/riscv/boot/Image $@

clean:
	rm -rf reset guest_initramfs.cpio host_initramfs.cpio