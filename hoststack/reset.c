/* Copyright 2019 SiFive, Inc */
/* SPDX-License-Identifier: Apache-2.0 */

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include "sbi.h"

void d_reset(int dom_index)
{
	char c = 'a';
	unsigned long err, val;
	sbi_ecall(SBI_EXT_EXPERIMENTAL_D, SBI_D_RESET, 
			dom_index, 0, 0, 0, 0, 0,
			&err, &val);
	printf("SBI_EXT_EXPERIMENTAL_D return %d %d\n", err, val);
}

int main(int narg, void **argv)
{
	int rc;

	printf("Start Reset---\n");
	if (narg != 2) {
        printf("Nothing is resetted.\n");
		return 1;
	}
	int dom_index = atoi(argv[1]);
	d_reset(dom_index);
	return 0;
}
