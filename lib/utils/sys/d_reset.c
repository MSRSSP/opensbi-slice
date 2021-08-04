/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2020 Western Digital Corporation or its affiliates.
 *
 * Authors:
 *   Anup Patel <anup.patel@wdc.com>
 */

#include <sbi/riscv_io.h>
#include <sbi/riscv_asm.h>
#include <sbi/sbi_ecall_interface.h>
#include <sbi/sbi_system.h>
#include <sbi_utils/sys/d_reset.h>
#include <sbi/sbi_domain.h>
#include <sbi/sbi_hart.h>
#include <sbi/sbi_hsm.h>
#include <sbi/sbi_scratch.h>
#include <sbi/sbi_console.h>
#include <sbi/d.h>

static void *d_reset_base;

static int d_reset_system_reset_check(u32 type, u32 reason)
{
	switch (type) {
	case SBI_SRST_RESET_TYPE_SHUTDOWN:
	case SBI_SRST_RESET_TYPE_COLD_REBOOT:
	case SBI_SRST_RESET_TYPE_WARM_REBOOT:
		return 1;
	}

	return 0;
}

void d_reset_by_hartmask(unsigned hart_mask){
	hart_mask = hart_mask & 0xffff;
	hart_mask <<= D_RESET_CPU_MASK_OFFSET;
	d_printf("%s: write %x to %lx\n", __func__, FINISHER_RESET | hart_mask, (unsigned long)d_reset_base);
	// MO_32. Using writew -> causes STORE/AMO error.
	writel(FINISHER_RESET | hart_mask, d_reset_base);
}

static void d_reset_system_reset(u32 type, u32 reason)
{
	struct sbi_domain * dom = (struct sbi_domain*)sbi_domain_thishart_ptr();
	unsigned int hartid;
	struct sbi_scratch *scratch;

	switch (type) {
	case SBI_SRST_RESET_TYPE_SHUTDOWN:
		if (reason == SBI_SRST_RESET_REASON_NONE)
			writew(FINISHER_PASS, d_reset_base);
		else
			writew(FINISHER_FAIL, d_reset_base);
		break;
	case SBI_SRST_RESET_TYPE_COLD_REBOOT:
	case SBI_SRST_RESET_TYPE_WARM_REBOOT:
	{
		sbi_hartmask_for_each_hart(hartid, dom->possible_harts) {
			d_printf("%s: hart id=%d\n", __func__, hartid);
			scratch = sbi_hartid_to_scratch(hartid);
			sbi_hsm_hart_stop(scratch, true);
			sbi_hsm_hart_start(scratch, dom, hartid, dom->next_addr, dom->next_mode, dom->next_arg1);
			writew(FINISHER_RESET, d_reset_base+(hartid<<2));
		}
	}
	break;
	}
}

static struct sbi_system_reset_device d_reset_reset = {
	.name = "d_reset",
	.system_reset_check = d_reset_system_reset_check,
	.system_reset = d_reset_system_reset
};

int d_reset_init(unsigned long base)
{
	d_reset_base = (void*)base;
	sbi_system_reset_set_device(&d_reset_reset);

	return 0;
}
