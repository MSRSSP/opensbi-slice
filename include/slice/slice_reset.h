/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2020 Western Digital Corporation or its affiliates.
 *
 * Authors:
 *   Anup Patel <anup.patel@wdc.com>
 */

#ifndef __SYS_SLICE_RESEY_H
#define __SYS_SLICE_RESEY_H

#include <sbi/sbi_types.h>

#define FINISHER_FAIL		0x3333
#define FINISHER_PASS		0x5555
#define FINISHER_RESET		0x7777
#define D_RESET_DOMAIN		0x1111
#define D_RESET_CPU_MASK_OFFSET 16

int d_reset_init(unsigned long base);

void d_reset_by_hartmask(unsigned hart_mask);

#endif //__SYS_SLICE_RESEY_H
