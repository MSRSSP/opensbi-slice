#ifndef __SBI_H
#define __SBI_H
#include <stdint.h>
#include <stddef.h>
#include <sbi/sbi_ecall_interface.h>
void sbi_ecall(int ext, int fid, unsigned long arg0, unsigned long arg1,
	       unsigned long arg2, unsigned long arg3, unsigned long arg4,
	       unsigned long arg5, unsigned long *err, unsigned long *value);

#define sbi_ecall_console_putc(c) sbi_ecall(SBI_EXT_0_1_CONSOLE_PUTCHAR, 0, c, 0, 0, 0, 0, 0, 0, 0)

static inline void sbi_ecall_console_puts(const char *str)
{
	while (str && *str)
		sbi_ecall_console_putc(*str++);
}

#define wfi()                                             \
	do {                                              \
		__asm__ __volatile__("wfi" ::: "memory"); \
	} while (0)


/*Extension ID*/
#define SBI_EXT_EXPERIMENTAL_D 0x00888888

/*Reset function ID*/
#define SBI_D_RESET 0x2001
#define SBI_D_CREATE 0x1001
#endif // __SBI_H