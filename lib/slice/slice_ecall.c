#include <sbi/sbi_trap.h>
#include <sbi/sbi_error.h>
#include <sbi/sbi_tlb.h>
#include <sbi/sbi_ipi.h>
#include <sbi/sbi_string.h>
#include <sbi/sbi_console.h>
#include <sbi/sbi_domain.h>
#include <sbi/sbi_hart.h>
#include <sbi/sbi_scratch.h>
#include <sbi/riscv_asm.h>
#include <sbi/sbi_ecall.h>
#include <slice/slice.h>
#include <slice/slice_ecall.h>
#include <slice/slice_err.h>
#include <slice/slice_reset.h>
#include <slice/slice_mgr.h>

int cpu_is_host_cpu()
{
	if(sbi_domain_thishart_ptr()->index == 1){
        return 1;
    }
    return 0;
}

static int slice_sbi_reset(unsigned long *out_val, unsigned long dom_index){
    *out_val = dom_index;
	struct sbi_domain * dom = sbi_index_to_domain(dom_index);
	if(dom == NULL){
		return SBI_ERR_SLICE_ILLEGAL_ARGUMENT;
	}
	slice_printf("%s: reset mask=%lx\n", __func__, *dom->possible_harts->bits);
	d_reset_by_hartmask(*dom->possible_harts->bits);
    return 0;
}

static int slice_sbi_info(unsigned long *out_val, unsigned long index){
	struct sbi_domain * dom;
	slice_printf("%s: dom_index=%ld\n", __func__, index);
	if(index == 0){
		sbi_domain_dump_all("");
	}else{
		dom = sbi_index_to_domain(index);
		sbi_domain_dump(dom, "");
		slice_print_fdt((void*)dom->next_arg1);
	}
	return 0;
}

static int slice_sbi_mem(unsigned long *out_val, unsigned long op_code, unsigned long address, unsigned long data){
	unsigned long * val_ptr = (unsigned long *) address;
	switch(op_code){
		case 'w':
			*val_ptr = data;
			break;
		default:
			break;
	}
	*out_val = *val_ptr;
    return 0;
}

static int sbi_ecall_d_handler(unsigned long extid, unsigned long funcid,
			       const struct sbi_trap_regs *regs,
			       unsigned long *out_val,
			       struct sbi_trap_info *out_trap)
{
	uintptr_t retval;
	slice_printf("%s: funcid=%lx\n", __func__, funcid);
	//if (!cpu_is_host_cpu())
	//	return SBI_ERR_SLICE_SBI_PROHIBITED;
	switch (funcid) {
	case SBI_SLICE_RESET:
		retval = slice_sbi_reset(out_val, regs->a0);
		break;
	case SBI_SLICE_CREATE:{
		struct sbi_hartmask mask;
		sbi_hartmask_clear_all(&mask);
		mask.bits[0] = regs->a0;
		retval = slice_create(mask, regs->a1, regs->a2, 0x84000000, 0x20000000, root.next_arg1, PRV_S);
		break;
	}
	case SBI_SLICE_INFO:
		retval = slice_sbi_info(out_val, regs->a0);
		break;
	case SBI_SLICE_MEM:
		retval = slice_sbi_mem(out_val, regs->a0, regs->a1, regs->a2);
		break;
	default:
		retval = SBI_ERR_SLICE_NOT_IMPLEMENTED;
		break;
	}

	return retval;
}

static int sbi_ecall_iopmp_handler(unsigned long extid, unsigned long funcid,
			       const struct sbi_trap_regs *regs,
			       unsigned long *out_val,
			       struct sbi_trap_info *out_trap)
{
	uintptr_t retval;

	slice_printf("%s: funcid=%lx\n", __func__, funcid);

	switch (funcid) {
	case SBI_IOPMP_UPDATE:
		retval = slice_sbi_reset(out_val, regs->a0);
		break;
	case SBI_IOPMP_REMOVE:
		retval = SBI_ERR_SLICE_NOT_IMPLEMENTED;
		break;
	default:
		retval = SBI_ERR_SLICE_NOT_IMPLEMENTED;
		break;
	}
	return retval;
}

struct sbi_ecall_extension ecall_d_ext = {
	.extid_start = SBI_EXT_EXPERIMENTAL_SLICE,
	.extid_end   = SBI_EXT_EXPERIMENTAL_SLICE,
	.handle	     = sbi_ecall_d_handler,
};

struct sbi_ecall_extension ecall_iopmp = {
	.extid_start = SBI_EXT_EXPERIMENTAL_IOPMP,
	.extid_end   = SBI_EXT_EXPERIMENTAL_IOPMP,
	.handle	     = sbi_ecall_iopmp_handler,
};

int slice_init_host_ecall_handler()
{
    slice_printf("[D] Initializing ... hart [%d]\n", current_hartid());
    return sbi_ecall_register_extension(&ecall_d_ext);
}
