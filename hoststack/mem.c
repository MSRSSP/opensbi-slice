#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include "sbi.h"

void d_mem(char op, unsigned long addr, unsigned long data)
{
	unsigned long err, val;
	sbi_ecall(SBI_EXT_EXPERIMENTAL_D, SBI_D_MEM, 
			(unsigned long)op, addr, data, 0, 0, 0,
			&err, &val);
	printf("%s return %d %d\n", __func__, err, val);
}

int main(int narg, void **argv)
{
    if(narg<3){
        printf("$prog `w`/`r` addr(hex) data(hex)\n");
        exit(0);
    }
    char * op = (char *)argv[1];
	unsigned long addr = strtoul(argv[2], 0, 16);
    unsigned long data = 0;
    if(narg > 3)
        data = strtoul(argv[3], 0, 16);
	d_mem(*op, addr, data);
	return 0;
}
