#
# SPDX-License-Identifier: BSD-2-Clause
#
# Copyright (c) 2020 Western Digital Corporation or its affiliates.
#
# Authors:
#   Anup Patel <anup.patel@wdc.com>
#

# Compiler flags
FW_TEXT_START=0x80000000
platform-cppflags-y =
platform-cflags-y += -DCONFIG_PLATFORM_MPFS=1
platform-cflags-y += -I$(SLICE_HSS_DIR) 
platform-cflags-y += -I$(SLICE_HSS_DIR)/include/ 
platform-cflags-y += -I$(SLICE_HSS_DIR)/services/opensbi/ 
platform-cflags-y += -I$(SLICE_HSS_DIR)/baremetal/polarfire-soc-bare-metal-library/src/platform/mpfs_hal/common/ 
platform-cflags-y += -I$(SLICE_HSS_DIR)/baremetal/polarfire-soc-bare-metal-library/src/platform/
platform-cflags-y += -I$(SLICE_HSS_DIR)/boards/slice/fpga_design_config/
platform-cflags-y += -I$(SLICE_HSS_DIR)/boards/slice/
platform-cflags-y += -I$(SLICE_HSS_DIR)/modules/ssmb/ipi/
platform-cflags-y += -Iinclude/sbi/
platform-cflags-y += -Os -Wl,--gc-sections -fomit-frame-pointer -fno-strict-aliasing 
platform-asflags-y =-fomit-frame-pointer -fno-strict-aliasing 
platform-ldflags-y = -fomit-frame-pointer -fno-strict-aliasing 


# Command for platform specific "make run"
platform-runcmd = qemu-system-riscv$(PLATFORM_RISCV_XLEN) -M virt -m 256M \
  -nographic -bios $(build_dir)/platform/generic/firmware/fw_payload.elf

# Blobs to build
FW_SLICE=y

ifeq ($(PLATFORM_RISCV_XLEN), 32)
  # This needs to be 4MB aligned for 32-bit system
  FW_JUMP_ADDR=$(shell printf "0x%X" $$(($(FW_TEXT_START) + 0x400000)))
else
  # This needs to be 2MB aligned for 64-bit system
  FW_JUMP_ADDR=$(shell printf "0x%X" $$(($(FW_TEXT_START) + 0x200000)))
endif
FW_JUMP_FDT_ADDR=$(shell printf "0x%X" $$(($(FW_TEXT_START) + 0x2200000)))
