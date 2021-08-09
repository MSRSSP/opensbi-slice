#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include "sbi.h"

/*
 * include/sbi/d_ecall.h
 *
 * Opcode is in enum sbi_iopmp_opcodes.
 */
void d_iopmp(int dom_index, int opcode, int iopmp_index, unsigned long start,
             unsigned long size, unsigned flags)
{
	unsigned long err, val;

	sbi_ecall(SBI_EXT_EXPERIMENTAL_IOPMP,
		  opcode,
		  dom_index,	// arg0
		  iopmp_index,	// arg1
		  start, // arg2
		  size, // arg3
		  flags, // arg4
		  0,
		  &err, &val);

	printf("SBI_EXT_EXPERIMENTAL_D return %d %d\n", err, val);
}
