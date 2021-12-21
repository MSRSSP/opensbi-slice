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
platform-objs-y += mss_utils.o
platform-objs-y += assert.o
#platform-objs-y += ../../../../modules/misc/hss_memcpy_via_pdma.o
#platform-objs-y += ../../../../baremetal/polarfire-soc-bare-metal-library/src/platform/drivers/mss_pdma/mss_pdma.o