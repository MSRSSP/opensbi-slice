LIBS-sliceloader += thirdparty/opensbi/lib/slice/slice_pmp.c
LIBS-sliceloader += thirdparty/opensbi/lib/slice/slice.c
LIBS-sliceloader += thirdparty/opensbi/lib/slice/slice_loader.c
LIBS-sliceloader += thirdparty/opensbi/lib/slice/slice_loader_start.c
LIBS-sliceloader += thirdparty/opensbi/lib/slice/slice_loader_finish.c
LIBS-sliceloader +=  modules/debug/hss_debug.c
LIBS-sliceloader += modules/misc/c_stubs.c
LIBS-sliceloader += thirdparty/opensbi/lib/sbi/riscv_asm.c
LIBS-sliceloader += thirdparty/opensbi/lib/sbi/riscv_atomic.c
LIBS-sliceloader += thirdparty/opensbi/lib/sbi/sbi_console.c
LIBS-sliceloader += modules/misc/assert.c
LIBS-sliceloader +=  application/hart0/hss_clock.c
LIBS-sliceloader += thirdparty/opensbi/lib/sbi/sbi_bitops.c
LIBS-sliceloader += thirdparty/opensbi/lib/sbi/sbi_ipi.c
LIBS-sliceloader += modules/misc/csr_helper.c
LIBS-sliceloader += thirdparty/opensbi/lib/sbi/sbi_domain.c
LIBS-sliceloader += thirdparty/opensbi/lib/sbi/sbi_hart.c
LIBS-sliceloader += thirdparty/opensbi/lib/sbi/sbi_math.c
LIBS-sliceloader += thirdparty/opensbi/lib/sbi/sbi_string.c
LIBS-sliceloader += modules/misc/stack_guard.c 
HEADER-sliceloader += thirdparty/opensbi/include/slice/slice_pmp.h
HEADER-sliceloader += thirdparty/opensbi/include/slice/slice.h
HEADER-sliceloader += thirdparty/opensbi/include/slice/slice_loader.h
HEADER-sliceloader += thirdparty/opensbi/include/slice/slice_loader_start.h
HEADER-sliceloader += thirdparty/opensbi/include/slice/slice_loader_finish.h
HEADER-sliceloader +=  modules/debug/hss_debug.h
HEADER-sliceloader += modules/misc/c_stubs.h
HEADER-sliceloader += thirdparty/opensbi/include/sbi/riscv_asm.h
HEADER-sliceloader += thirdparty/opensbi/include/sbi/riscv_atomic.h
HEADER-sliceloader += thirdparty/opensbi/include/sbi/sbi_console.h
HEADER-sliceloader += modules/misc/assert.h
HEADER-sliceloader +=  application/hart0/hss_clock.h
HEADER-sliceloader += thirdparty/opensbi/include/sbi/sbi_bitops.h
HEADER-sliceloader += thirdparty/opensbi/include/sbi/sbi_ipi.h
HEADER-sliceloader += modules/misc/csr_helper.h
HEADER-sliceloader += thirdparty/opensbi/include/sbi/sbi_domain.h
HEADER-sliceloader += thirdparty/opensbi/include/sbi/sbi_hart.h
HEADER-sliceloader += thirdparty/opensbi/include/sbi/sbi_math.h
HEADER-sliceloader += thirdparty/opensbi/include/sbi/sbi_string.h
HEADER-sliceloader += modules/misc/stack_guard.h
INCLUDE += boards/slice/fpga_design_config/
INCLUDE += /thirdparty/opensbi/include/ 
INCLUDE += thirdparty/opensbi/include/sbi 
INCLUDE += thirdparty/opensbi/include/sbi_utils 
INCLUDE += thirdparty/opensbi/lib/utils/libfdt/
INCLUDE += thirdparty/opensbi/include/slice
INCLUDE += include/
INCLUDE += modules/ssmb/ipi/
INCLUDE += services/boot/
INCLUDE += services/sgdma
INCLUDE += services/usbdmsc/
INCLUDE += baremetal/polarfire-soc-bare-metal-library/src/platform/mpfs_hal/common
INCLUDE += baremetal/polarfire-soc-bare-metal-library/src/platform 
INCLUDE += boards/slice
INCLUDE += ./
DEP_FLAGS = $(foreach obj, $(LIBS-sliceloader), /home/ziqiao/riscv/slice-hss/$(obj))

INCLUDE_FLAGS = $(foreach obj,$(INCLUDE), -I/home/ziqiao/riscv/slice-hss/$(obj))
c:
	mkdir -p sliceloader/c
	cmd = $(foreach obj,$(LIBS-sliceloader),cp /home/ziqiao/riscv/slice-hss/$(obj) sliceloader/c/;)
	echo $(cmd)
	cmd = $(foreach obj,$(HEADER-sliceloader),cp /home/ziqiao/riscv/slice-hss/$(obj) sliceloader/c/;)
	echo $(cmd)

sliceloader/sliceloader:
	mkdir -p $(shell dirname sliceloader)
	@echo "hi"
	riscv64-unknown-linux-gnu-gcc -O0  \
-ftest-coverage  -DCONFIG_PLATFORM_MPFS=1\
 -DCONFIG_SLICE_SW_RESET=1 -DCONFIG_SLICE=1 \
 -DFW_PIC=y -o $@ $(INCLUDE_FLAGS)  \
    $(DEP_FLAGS)  -pipe -grecord-gcc-switches -fno-builtin -fno-builtin-printf -fomit-frame-pointer -Wredundant-decls -Wundef -Wwrite-strings -fno-strict-aliasing -fno-common -Wendif-labels -Wmissing-include-dirs -Wempty-body -Wformat=2 -Wformat-security -Wformat-y2k -Winit-self -Wold-style-declaration -Wtype-limits -mno-fdiv -fms-extensions -nostdlib -ffunction-sections -fdata-sections -fstack-protector-strong -DCONFIG_PLATFORM_MPFS=1 -DCONFIG_SLICE_SW_RESET=1 -DCONFIG_SLICE=1 -DFW_PIC=y  -DROLLOVER_TEST=0 -Wl,--gc-sections  -Wl,-eslice_loader_main

sliceloader/sliceloaders:
	mkdir -p $(shell dirname sliceloader)
	@echo "hi"
	riscv64-unknown-linux-gnu-gcc -Os -s  \
 -DCONFIG_PLATFORM_MPFS=1\
 -DCONFIG_SLICE_SW_RESET=1 -DCONFIG_SLICE=1 \
 -DFW_PIC=y -o $@ $(INCLUDE_FLAGS)  \
    $(DEP_FLAGS)  -pipe -grecord-gcc-switches -fno-builtin -fno-builtin-printf -fomit-frame-pointer -Wredundant-decls -Wundef -Wwrite-strings -fno-strict-aliasing -fno-common -Wendif-labels -Wmissing-include-dirs -Wempty-body -Wformat=2 -Wformat-security -Wformat-y2k -Winit-self -Wold-style-declaration -Wtype-limits -mno-fdiv -fms-extensions -nostdlib -ffunction-sections -fdata-sections -fstack-protector-strong -DCONFIG_PLATFORM_MPFS=1 -DCONFIG_SLICE_SW_RESET=1 -DCONFIG_SLICE=1 -DFW_PIC=y  -DROLLOVER_TEST=0 -Wl,--gc-sections  -Wl,-eslice_loader_main
