/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2020 Western Digital Corporation or its affiliates.
 *
 * Authors:
 *   Anup Patel <anup.patel@wdc.com>
 */

#include <sbi/sbi_scratch.h>
#include <sbi_utils/fdt/fdt_helper.h>
#include <sbi_utils/reset/fdt_reset.h>
#include <sbi_utils/sys/d_reset.h>
#include <sbi/sbi_console.h>
#include <sbi/d.h>

static int d_reset_reset_init(void *fdt, int nodeoff,
				  const struct fdt_match *match)
{
	int rc;
	unsigned long addr;

	rc = fdt_get_node_addr_size(fdt, nodeoff, &addr, NULL);
	if (rc)
		return rc;
	d_printf("===========%s using domain reset\n", __func__);
	return d_reset_init(addr);
}

static const struct fdt_match d_reset_reset_match[] = {
	{ .compatible = "domain-reset" },
};

struct fdt_reset fdt_reset_domain = {
	.match_table = d_reset_reset_match,
	.init = d_reset_reset_init,
};