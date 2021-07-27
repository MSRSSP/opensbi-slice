/* Copyright 2019 SiFive, Inc */
/* SPDX-License-Identifier: Apache-2.0 */

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>

void sbi_ecall(int ext, int fid, unsigned long arg0, unsigned long arg1,
	       unsigned long arg2, unsigned long arg3, unsigned long arg4,
	       unsigned long arg5, unsigned long *err, unsigned long *value)
{
	register uintptr_t a0 asm("a0") = (uintptr_t)(arg0);
	register uintptr_t a1 asm("a1") = (uintptr_t)(arg1);
	register uintptr_t a2 asm("a2") = (uintptr_t)(arg2);
	register uintptr_t a3 asm("a3") = (uintptr_t)(arg3);
	register uintptr_t a4 asm("a4") = (uintptr_t)(arg4);
	register uintptr_t a5 asm("a5") = (uintptr_t)(arg5);
	register uintptr_t a6 asm("a6") = (uintptr_t)(fid);
	register uintptr_t a7 asm("a7") = (uintptr_t)(ext);

	asm volatile("ecall"
		     : "+r"(a0), "+r"(a1)
		     : "r"(a2), "r"(a3), "r"(a4), "r"(a5), "r"(a6), "r"(a7)
		     : "memory");
	*err   = a0;
	*value = a1;
}

/*Extension ID*/
#define SBI_EXT_EXPERIMENTAL_D 0x00888888

/*Reset function ID*/
#define SBI_D_RESET 0x2001

void d_reset(int dom_index)
{
	char c = 'a';
	unsigned long err, val;
	sbi_ecall(SBI_EXT_EXPERIMENTAL_D, SBI_D_RESET, dom_index, 0, 0, 0, 0, 0,
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
	char *opt;
	opt	      = argv[1];
	int dom_index = atoi(opt);
	d_reset(dom_index);
	return 0;
}
