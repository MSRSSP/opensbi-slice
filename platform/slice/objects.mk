#
# SPDX-License-Identifier: BSD-2-Clause
#
# Copyright (c) 2020 Western Digital Corporation or its affiliates.
#
# Authors:
#   Anup Patel <anup.patel@wdc.com>
#

platform-objs-y += platform.o  
platform-objs-y += slice_uart_helper.o
platform-objs-y += mss_uart.o
platform-objs-y += ../../../../application/hart0/hss_clock.o
platform-objs-y += ../../../../modules/misc/csr_helper.o
platform-objs-y += ../../../../modules/misc/c_stubs.o
platform-objs-y += ../../../../modules/misc/assert.o
platform-objs-y += ../../../../modules/debug/hss_debug.o
platform-objs-y += ../../../../modules/ssmb/ipi/ssmb_ipi.o