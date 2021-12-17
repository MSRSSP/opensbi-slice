#include <sbi/riscv_asm.h>
#include <stddef.h>
#include <common/mss_hart_ints.h>
/*------------------------------------------------------------------------------
 * Enable particular local interrupt
 */
void __enable_local_irq(uint8_t local_interrupt)
{
    if((local_interrupt > (int8_t)0) && (local_interrupt <= LOCAL_INT_MAX))
    {
        csr_write_num(MSTATUS_MIE, (0x1LLU << (int8_t)(local_interrupt + 16U)));  /* mie Register- Machine Interrupt Enable Register */
    }
}