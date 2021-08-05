CROSS_COMPILE=riscv64-unknown-linux-gnu-
CC=$(CROSS_COMPILE)gcc
AR=$(CROSS_COMPILE)ar
compile_cc = mkdir -p `dirname $(1)`; \
	     echo "$(CC)        $(subst $(build_dir)/,,$(1))"; \
	     $(CC) $(CFLAGS)  $(call dynamic_flags,$(1),$(2)) -c $(2) -o $(1)
compile_ar = $(CMD_PREFIX)mkdir -p `dirname $(1)`; \
	     echo "$(AR)        $(subst $(build_dir)/,,$(1))"; \
	     $(AR) $(ARFLAGS) $(1) $(2)
CFLAGS+=-Iinclude
CFLAGS+=-I../include
LINUX_DIR=${TOP}/linux

build_dir=build
PROGRAM ?= $(build_dir)/reset
HOST_PROGRAMS+=$(PROGRAM)
HOST_PROGRAMS+=$(build_dir)/create
HOST_INITRAM=$(build_dir)/host_initramfs
GUEST_INITRAM=$(build_dir)/guest_initramfs
TARGETS=
TARGETS+=${PROGRAM}
TARGETS+=${GUEST_INITRAM}.cpio
TARGETS+=${HOST_INITRAM}.cpio
TARGETS+=$(build_dir)/host_kernel_image
TARGETS+=$(build_dir)/guest_kernel_image
CURRENT_DIR=$(PWD)
default: $(TARGETS)
include lib/Makefile
$(build_dir)/reset: $(build_dir)/reset.o $(build_dir)/lib/libsbi.a
	$(CROSS_COMPILE)gcc $< $(build_dir)/lib/libsbi.a  -o $@
$(build_dir)/create: $(build_dir)/create.o $(build_dir)/lib/libsbi.a
	$(CROSS_COMPILE)gcc $< $(build_dir)/lib/libsbi.a  -o $@
$(GUEST_INITRAM).cpio:
	wget https://github.com/lowRISC/lowrisc-chip/releases/download/v0.6-rc3/initramfs.cpio 
	mv initramfs.cpio $(GUEST_INITRAM).cpio 

$(HOST_INITRAM).cpio: $(GUEST_INITRAM).cpio  $(HOST_PROGRAMS)
	echo "rm -r ${HOST_INITRAM}; mkdir ${HOST_INITRAM}; cd ${HOST_INITRAM};\
	cpio -i < ../guest_initramfs.cpio;\
	chown root init;\
	mkdir hoststack; cp ../reset hoststack/reset;\
	mkdir hoststack; cp ../create hoststack/create;\
	cd ${LINUX_DIR};\
	echo $(CURRENT_DIR);\
	usr/gen_initramfs.sh -o $(CURRENT_DIR)/${HOST_INITRAM}.cpio $(CURRENT_DIR)/${HOST_INITRAM};" | fakeroot

$(build_dir)/host_kernel_image: $(HOST_INITRAM).cpio ${LINUX_DIR} $(build_dir)/guest_kernel_image
	echo "cd ${LINUX_DIR};\
	make ARCH=riscv CROSS_COMPILE=riscv64-unknown-linux-gnu- \
	CONFIG_INITRAMFS_SOURCE=$(CURRENT_DIR)/${HOST_INITRAM}.cpio -j16;" | sh
	cp ${LINUX_DIR}/arch/riscv/boot/Image $@
	

$(build_dir)/guest_kernel_image: $(GUEST_INITRAM).cpio
	echo "cd ${LINUX_DIR};\
	make ARCH=riscv CROSS_COMPILE=riscv64-unknown-linux-gnu- \
	CONFIG_INITRAMFS_SOURCE=$(CURRENT_DIR)/${GUEST_INITRAM}.cpio -j16;" | sh
	cp ${LINUX_DIR}/arch/riscv/boot/Image $@

clean:
	rm -rf build