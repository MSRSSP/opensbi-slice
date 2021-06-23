
#define __ASM_STR(x)	x

#if __riscv_xlen == 64
#define __REG_SEL(a, b)	__ASM_STR(a)
#define RISCV_PTR		.dword
#define RISCV_SZPTR		8
#define RISCV_LGPTR		3
#elif __riscv_xlen == 32
#define __REG_SEL(a, b)	__ASM_STR(b)
#define RISCV_PTR		.word
#define RISCV_SZPTR		4
#define RISCV_LGPTR		2
#else
#error "Unexpected __riscv_xlen"
#endif

#define REG_L		__REG_SEL(ld, lw)
#define REG_S		__REG_SEL(sd, sw)

	.section .text, "ax", %progbits
	.align 3
	.globl _start
_start:
	call _uart_init

	la	a0, _start
	la	a1, _uart_init

1:
	la	t5, _test_misalign
	lh	t4, (t5)
	lhu	t6, (t5)
	bne	t4, t6, 3f
	li	a0, 'A'
	call	_uart_putc
3:

	la	a0, _hello_str
	call	_uart_puts


	la	a0, _delay_loop_count
	REG_L	a0, (a0)
2:	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	add	a0, a0, -1
	bne	a0, zero, 2b

	j	1b

	.align 1
	.byte	0x00
_test_misalign:
	.byte	0x1e
	.byte	0xff

	.align 3
_delay_loop_count:
#ifdef DELAY_LOOP_COUNT
	RISCV_PTR	DELAY_LOOP_COUNT
#else
	RISCV_PTR	0x10000000
#endif
_hello_str:
	.ascii	"Hello World !\r\n"

#ifdef UART_SBI

	.align 3
_uart_init:
	ret

	.align 3
_uart_putc:
	li	a7, 0x1
	add	a6, zero, zero
	ecall
	ret


#elif UART_SIFIVE

#define UART_SIFIVE_TXFIFO_OFFSET	(0 * 0x4)
#define UART_SIFIVE_RXFIFO_OFFSET	(1 * 0x4)
#define UART_SIFIVE_TXCTRL_OFFSET	(2 * 0x4)
#define UART_SIFIVE_RXCTRL_OFFSET	(3 * 0x4)
#define UART_SIFIVE_IE_OFFSET		(4 * 0x4)
#define UART_SIFIVE_IP_OFFSET		(5 * 0x4)
#define UART_SIFIVE_DIV_OFFSET		(6 * 0x4)

#define UART_SIFIVE_TXFIFO_FULL	0x80000000
#define UART_SIFIVE_RXFIFO_EMPTY	0x80000000
#define UART_SIFIVE_RXFIFO_DATA	0x000000ff
#define UART_SIFIVE_TXCTRL_TXEN	0x1
#define UART_SIFIVE_RXCTRL_RXEN	0x1

	.align 3
_uart_sifive_base:
	RISCV_PTR	UART_SIFIVE_BASE
_uart_sifive_div:
#ifdef UART_SIFIVE_DIV
	RISCV_PTR	UART_SIFIVE_DIV
#else
	RISCV_PTR	0x10
#endif
_uart_sifive_txfull:
	RISCV_PTR	UART_SIFIVE_TXFIFO_FULL

	.align 3
_uart_init:
	la	t0, _uart_sifive_base
	REG_L	t0, (t0)
	la	t1, _uart_sifive_div
	REG_L	t1, (t1)
	sw	t1, UART_SIFIVE_DIV_OFFSET(t0)
	sw	zero, UART_SIFIVE_IE_OFFSET(t0)
	li	t1, UART_SIFIVE_TXCTRL_TXEN
	sw	t1, UART_SIFIVE_TXCTRL_OFFSET(t0)
	li	t1, UART_SIFIVE_RXCTRL_RXEN
	sw	t1, UART_SIFIVE_RXCTRL_OFFSET(t0)
	ret

	.align 3
_uart_putc:
	la	t4, _uart_sifive_base
	REG_L	t4, (t4)
	la	t5, _uart_sifive_txfull
	REG_L	t5, (t5)
1:	lwu	t6, UART_SIFIVE_TXFIFO_OFFSET(t4)
	and	t6, t6, t5
	bne	t6, zero, 1b
	sw	a0, UART_SIFIVE_TXFIFO_OFFSET(t4)
	ret

#else

	.align 3
_uart_init:
	ret

	.align 3
_uart_putc:
	ret

#endif

	.align 3
_uart_puts:
	add	t0, ra, zero
	add	t1, a0, zero
_uart_puts_again:
	lbu	a0, 0(t1)
	beq	a0, zero, _uart_puts_done
	call	_uart_putc
	add	t1, t1, 0x1
	j	_uart_puts_again
_uart_puts_done:
	add	ra, t0, zero
	ret
