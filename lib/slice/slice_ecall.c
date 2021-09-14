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
		return SBI_ERR_SLICE_ILLEGAL_ARGUMENT;
	}
	slice_printf("%s: reset mask=%lx\n", __func__, *dom->possible_harts->bits);
	d_reset_by_hartmask(*dom->possible_harts->bits);
    return 0;
}

static int sbi_d_create(unsigned long *out_val, 
					unsigned long cpu_mask, 
					unsigned long mem_start, 
					unsigned long mem_size){
	struct sbi_hartmask mask;
	struct sbi_domain* dom;
	struct sbi_domain_memregion *reg, * regions;
	unsigned  cpuid=0, count=0, boot_hartid = -1;
	sbi_hartmask_clear_all(&mask);
	while(cpu_mask){
		if(cpu_mask&1){
			sbi_hartmask_set_hart(cpuid, &mask);
			boot_hartid = cpuid;
		}
		cpu_mask >>= 1;
		cpuid++;
	}
	dom = (struct sbi_domain*) slice_allocate_domain(&mask);
	dom->boot_hartid = boot_hartid;
	regions = dom->regions;
	unsigned long all_perm = SBI_DOMAIN_MEMREGION_MMODE | 
					SBI_DOMAIN_MEMREGION_READABLE | 
					SBI_DOMAIN_MEMREGION_WRITEABLE | 
					SBI_DOMAIN_MEMREGION_EXECUTABLE;
	sbi_domain_memregion_init(0, 1UL<<31,
					all_perm, &regions[count++]);
	sbi_domain_memregion_init(mem_start, mem_size,
					all_perm, &regions[count++]);
	//sbi_domain_memregion_init(1UL<<31, 1UL<<31,
	//				SBI_DOMAIN_MEMREGION_MMODE, &regions[count++]);
	sbi_domain_for_each_memregion(&root, reg) {
		if ((reg->flags & SBI_DOMAIN_MEMREGION_READABLE) ||
		    (reg->flags & SBI_DOMAIN_MEMREGION_WRITEABLE) ||
		    (reg->flags & SBI_DOMAIN_MEMREGION_EXECUTABLE))
			continue;
		if (sbi_hart_pmp_count(sbi_scratch_thishart_ptr()) <= count)
			return SBI_ERR_SLICE_NO_FREE_RESOURCE;
		sbi_memcpy(&regions[count++], reg, sizeof(*reg));
	}
	dom->next_addr = mem_start;
	dom->next_arg1 = mem_start + 0x2000000;
	dom->next_mode = 1;
	dom->next_boot_src  = 0x84000000;
	dom->next_boot_size = 0x02000000;
	sbi_memcpy(dom->stdout_path, "/soc/serial@10010000", 21);
	dom->stdout_path[21] = 0;
	return sbi_domain_register(dom, dom->possible_harts);
}

static int sbi_d_info(unsigned long *out_val, unsigned long index){
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

static int sbi_d_mem(unsigned long *out_val, unsigned long op_code, unsigned long address, unsigned long data){
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
		retval = sbi_d_reset(out_val, regs->a0);
		break;
	case SBI_SLICE_CREATE:
		retval = sbi_d_create(out_val, regs->a0, regs->a1, regs->a2);
		break;
	case SBI_SLICE_INFO:
		retval = sbi_d_info(out_val, regs->a0);
		break;
	case SBI_SLICE_MEM:
		retval = sbi_d_mem(out_val, regs->a0, regs->a1, regs->a2);
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
		retval = sbi_d_reset(out_val, regs->a0);
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
