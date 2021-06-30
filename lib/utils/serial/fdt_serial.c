/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2020 Western Digital Corporation or its affiliates.
 *
 * Authors:
 *   Anup Patel <anup.patel@wdc.com>
 */

#include <libfdt.h>
#include <sbi/sbi_console.h>
#include <sbi/sbi_domain.h>
#include <sbi/sbi_error.h>
#include <sbi/sbi_scratch.h>
#include <sbi_utils/fdt/fdt_helper.h>
#include <sbi_utils/serial/fdt_serial.h>

extern struct fdt_serial fdt_serial_uart8250;
extern struct fdt_serial fdt_serial_sifive;
extern struct fdt_serial fdt_serial_htif;
extern struct fdt_serial fdt_serial_shakti;
extern struct fdt_serial fdt_serial_gaisler;

static struct fdt_serial *serial_drivers[] = {
	&fdt_serial_uart8250,
	&fdt_serial_sifive,
	&fdt_serial_htif,
	&fdt_serial_shakti,
	&fdt_serial_gaisler
};

static struct fdt_serial dummy = {
	.match_table = NULL,
	.init = NULL,
};

static struct fdt_serial *current_driver = &dummy;

/* Fix fdt's uart according to dom_ptr.

1. Update stdout-path to domain specific one 
2. Remove uarts outside of the domain.
If not invalidating uarts outside of the domain, 
kernel would panic due to serial_probe (e.g., sifive_serial_probe).
*/
void fdt_serial_fixup(void * fdt, const void *dom_ptr){
	/* Fix up UART*/
	const struct sbi_domain *dom = dom_ptr;
	const struct fdt_match *match;
 	struct fdt_serial *drv;
	int pos, selected_noff=-1, noff = -1, coff;
	coff = fdt_path_offset(fdt, "/chosen");
	if(coff < 0)
		return;
	if (sbi_strlen(dom->stdout_path)){
		// Set the chosen stdout path to the dom's stdout path.
		fdt_setprop_string(fdt, coff, "stdout-path", dom->stdout_path);
		sbi_printf("hart%d: Fix UART: %s\n", current_hartid(), 
					dom->stdout_path);
		const char *sep, *start = dom->stdout_path;
		/* The device path may be followed by ':' */
		sep = strchr(start, ':');
		if (sep)
			selected_noff = fdt_path_offset_namelen(fdt, start, 
											sep-start);
		else
			selected_noff = fdt_path_offset(fdt, start);
	}
	for (pos = 0; pos < array_size(serial_drivers); pos++) {
		drv = serial_drivers[pos];
		noff=-1;
		while (true){
			noff = fdt_find_match(fdt, noff, drv->match_table, &match);
			if (noff < 0)
				break;
			if(selected_noff!=noff)
			{// Invalidate uart not belonging to this domain;
				fdt_nop_node(fdt, noff);
			}
		}
	}
}

int fdt_serial_init(void)
{
	const void *prop;
	struct fdt_serial *drv;
	const struct fdt_match *match;
	int pos, noff = -1, len, coff, rc;
	void *fdt = sbi_scratch_thishart_arg1_ptr();

	/* Find offset of node pointed to by stdout-path */
	coff = fdt_path_offset(fdt, "/chosen");
	if (-1 < coff) {
		prop = fdt_getprop(fdt, coff, "stdout-path", &len);
		if (prop && len) {
			const char *sep, *start = prop;

			/* The device path may be followed by ':' */
			sep = strchr(start, ':');
			if (sep)
				noff = fdt_path_offset_namelen(fdt, prop,
							       sep - start);
			else
				noff = fdt_path_offset(fdt, prop);
		}
	}

	/* First check DT node pointed by stdout-path */
	for (pos = 0; pos < array_size(serial_drivers) && -1 < noff; pos++) {
		drv = serial_drivers[pos];

		match = fdt_match_node(fdt, noff, drv->match_table);
		if (!match)
			continue;

		if (drv->init) {
			rc = drv->init(fdt, noff, match);
			if (rc == SBI_ENODEV)
				continue;
			if (rc)
				return rc;
		}
		current_driver = drv;
		break;
	}

	/* Check if we found desired driver */
	if (current_driver != &dummy)
		goto done;

	/* Lastly check all DT nodes */
	for (pos = 0; pos < array_size(serial_drivers); pos++) {
		drv = serial_drivers[pos];

		noff = fdt_find_match(fdt, -1, drv->match_table, &match);
		if (noff < 0)
			continue;

		if (drv->init) {
			rc = drv->init(fdt, noff, match);
			if (rc == SBI_ENODEV)
				continue;
			if (rc)
				return rc;
		}
		current_driver = drv;
		break;
	}

done:
	return 0;
}
