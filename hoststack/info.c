#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include "sbi.h"

void d_info(unsigned long domain_index)
{
	unsigned long err, val;
	sbi_ecall(SBI_EXT_EXPERIMENTAL_D, SBI_D_INFO, 
			domain_index,0, 0, 0, 0, 0,
			&err, &val);
	printf("%s return %d %d\n", __func__, err, val);
}

int main(int narg, void **argv)
{
	int rc;

	printf("Dump domain info---\n");
    if(narg<2){
        printf("Please enter index argument (0 for all, positive value for domain index)\n");
        exit(0);
    }
	int index = strtoul(argv[1], 0, 10);
	d_info(index);
	return 0;
}
