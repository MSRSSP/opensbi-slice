#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include "sbi.h"

void d_create(unsigned long cpu_masks, unsigned long mem_start, unsigned long mem_size)
{
	unsigned long err, val;
	sbi_ecall(SBI_EXT_EXPERIMENTAL_D, SBI_D_CREATE, 
			cpu_masks, mem_start, mem_size, 0, 0, 0,
			&err, &val);
	printf("%s return %d %d\n", __func__, err, val);
}

int main(int narg, void **argv)
{
	int rc;

	printf("Start Creating a domain---\n");
    if(narg<4){
        printf("Please add arguments for $proc cpu_mask mem_start mem_size\n");
        exit(0);
    }
	unsigned long cpu_mask = strtoul(argv[1], 0, 16);
    unsigned long mem_start = strtoul(argv[2], 0, 16);
    unsigned long mem_size = strtoul(argv[3], 0, 16);
	d_create(cpu_mask, mem_start, mem_size);
	return 0;
}
