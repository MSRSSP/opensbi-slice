#LIBS-sliceloader += thirdparty/opensbi/lib/utils/fdt/fdt_domain.c  
#LIBS-sliceloader += thirdparty/opensbi/lib/utils/libfdt/*.c  
#LIBS-sliceloader += thirdparty/opensbi/lib/utils/*/*.S 
#LIBS-sliceloader += thirdparty/opensbi/lib/slice/slice_fdt.c
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
#LIBS-sliceloader += thirdparty/opensbi/lib/utils/libfdt/*.c
#LIBS-sliceloader += thirdparty/opensbi/lib/sbi/riscv_hardfp.c
LIBS-sliceloader += modules/misc/assert.c
LIBS-sliceloader +=  application/hart0/hss_clock.c
#LIBS-sliceloader += thirdparty/opensbi/lib/sbi/sbi_bitmap.c
LIBS-sliceloader += thirdparty/opensbi/lib/sbi/sbi_bitops.c
LIBS-sliceloader += thirdparty/opensbi/lib/sbi/sbi_ipi.c
#LIBS-sliceloader += thirdparty/opensbi/lib/sbi/sbi_domain.c
#LIBS-sliceloader += sbi_ecall.c
#LIBS-sliceloader += sbi_ecall_base.c
#LIBS-sliceloader += sbi_ecall_hsm.c
#LIBS-sliceloader += sbi_ecall_legacy.c
#LIBS-sliceloader += sbi_ecall_replace.c
#LIBS-sliceloader += sbi_ecall_vendor.c
#LIBS-sliceloader += sbi_emulate_csr.c
LIBS-sliceloader += modules/misc/csr_helper.c
LIBS-sliceloader += thirdparty/opensbi/lib/sbi/sbi_domain.c
LIBS-sliceloader += thirdparty/opensbi/lib/sbi/sbi_hart.c
LIBS-sliceloader += thirdparty/opensbi/lib/sbi/sbi_math.c
#LIBS-sliceloader += thirdparty/opensbi/lib/sbi/sbi_hfence.S
#LIBS-sliceloader += thirdparty/opensbi/lib/sbi/sbi_hsm.c
#LIBS-sliceloader += sbi_illegal_insn.c
#LIBS-sliceloader += sbi_init.c
#LIBS-sliceloader += thirdparty/opensbi/lib/sbi/sbi_ipi.c
#LIBS-sliceloader += thirdparty/opensbi/lib/sbi/sbi_misaligned_ldst.c
#LIBS-sliceloader += thirdparty/opensbi/lib/sbi/sbi_platform.c
#LIBS-sliceloader += thirdparty/opensbi/lib/sbi/sbi_scratch.c
LIBS-sliceloader += thirdparty/opensbi/lib/sbi/sbi_string.c
#LIBS-sliceloader += thirdparty/opensbi/lib/utils/serial/fdt_serial_uart8250.c
#LIBS-sliceloader += thirdparty/opensbi/lib/utils/serial/uart8250.c
#LIBS-sliceloader += thirdparty/opensbi/lib/utils/fdt/fdt_helper.c
#LIBS-sliceloader += thirdparty/opensbi/lib/sbi/sbi_system.c LIBS-sliceloader +=
#sbi_timer.c LIBS-sliceloader += sbi_tlb.c LIBS-sliceloader +=
#thirdparty/opensbi/lib/sbi/sbi_unpriv.c LIBS-sliceloader += sbi_expected_trap.c
#LIBS-sliceloader += modules/debug/hss_debug.c LIBS-sliceloader +=
#application/hart0/hss_clock.c LIBS-sliceloader += modules/misc/csr_helper.c 
LIBS-sliceloader += modules/misc/stack_guard.c 
#LIBS-sliceloader += baremetal/polarfire-soc-bare-metal-library/src/platform/mpfs_hal/common/mss_irq_handler_stubs.c 
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
