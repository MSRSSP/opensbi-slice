#include <sbi/sbi_trap.h>
#include <sbi/sbi_error.h>
#include <sbi/sbi_tlb.h>
#include <sbi/sbi_ipi.h>
#include <sbi/sbi_string.h>
#include <sbi/sbi_console.h>
#include <sbi/sbi_domain.h>
#include <sbi/sbi_scratch.h>
#include <sbi/riscv_asm.h>
#include <sbi/sbi_ecall.h>
#include <sbi/d.h>
#include <sbi/d_ecall.h>
#include <sbi_utils/sys/d_reset.h>


int cpu_is_host_cpu()
{
	if(sbi_domain_thishart_ptr()->index == 1){
        return 1;
    }
    return 0;
}

static int sbi_d_reset(unsigned long *out_val, unsigned long dom_index){
    *out_val = dom_index;
	struct sbi_domain * dom = sbi_index_to_domain(dom_index);
	if(dom == NULL){
		return SBI_ERR_D_ILLEGAL_ARGUMENT;
	}
	d_printf("%s: reset mask=%lx\n", __func__, *dom->possible_harts->bits);
	d_reset_by_hartmask(*dom->possible_harts->bits);
    return 0;
}

static int sbi_d_create(unsigned long *out_val, 
					unsigned long ncpus, 
					unsigned long mem_start, 
					unsigned long mem_size_order){
	d_printf("%s: mem_start=%lx\n", __func__, mem_start);
    return 0;
}

static int sbi_ecall_d_handler(unsigned long extid, unsigned long funcid,
			       const struct sbi_trap_regs *regs,
			       unsigned long *out_val,
			       struct sbi_trap_info *out_trap)
{
	uintptr_t retval;
	d_printf("%s: funcid=%lx\n", __func__, funcid);
	//if (!cpu_is_host_cpu())
	//	return SBI_ERR_D_SBI_PROHIBITED;
	switch (funcid) {
	case SBI_D_RESET:
		retval = sbi_d_reset(out_val, regs->a0);
		break;
	case SBI_D_CREATE:
		retval = sbi_d_create(out_val, regs->a0, regs->a1, regs->a2);
		break;
	default:
		retval = SBI_ERR_SM_NOT_IMPLEMENTED;
		break;
	}

	return retval;
}

struct sbi_ecall_extension ecall_d_ext = {
	.extid_start = SBI_EXT_EXPERIMENTAL_D,
	.extid_end   = SBI_EXT_EXPERIMENTAL_D,
	.handle	     = sbi_ecall_d_handler,
};

int d_init_host_ecall_handler(){
    d_printf("[D] Initializing ... hart [%d]\n", current_hartid());
    return sbi_ecall_register_extension(&ecall_d_ext);
}