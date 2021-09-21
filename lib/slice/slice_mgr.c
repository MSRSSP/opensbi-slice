#include <sbi/sbi_hart.h>
#include <sbi/sbi_ipi.h>
#include <sbi/sbi_scratch.h>
#include <sbi/sbi_trap.h>
#include <sbi/sbi_console.h>
#include <sbi/sbi_domain.h>
#include <sbi/sbi_bitops.h>
#include <sbi/sbi_string.h>
#include <sbi/sbi_hsm.h>
#include <sbi/sbi_ecall_interface.h>
#include <sbi/sbi_platform.h>

#include <slice/slice_mgr.h>
#include <slice/slice_err.h>
#include <slice/slice.h>
#include <slice/slice_pmp.h>

// TODO: Add a lock to avoid dst hart ignors a pending IPI
// when multiple src harts tries to send IPI to it;
struct SliceIPIData slice_ipi_data[MAX_HART_NUM][MAX_HART_NUM];

void __attribute__((noreturn))
slice_reset_regs(unsigned long next_addr, unsigned long next_mode)
{
#if __riscv_xlen == 32
	unsigned long val, valH;
#else
	unsigned long val;
#endif
	sbi_printf("%s(): next_addr=%#lx next_mode=%#lx\n", __func__, next_addr,
		   next_mode);

	switch (next_mode) {
	case PRV_M:
		break;
	case PRV_S:
	case PRV_U:
	default:
		sbi_hart_hang();
	}

	val = csr_read(CSR_MSTATUS);
	val = INSERT_FIELD(val, MSTATUS_MPP, next_mode);
	val = INSERT_FIELD(val, MSTATUS_MPIE, 0);
#if __riscv_xlen == 32
	if (misa_extension('H')) {
		valH = csr_read(CSR_MSTATUSH);
		valH = INSERT_FIELD(valH, MSTATUSH_MPV, 0);
		csr_write(CSR_MSTATUSH, valH);
	}
#else
	if (misa_extension('H')) {
		val = INSERT_FIELD(val, MSTATUS_MPV, 0);
	}
#endif
	// Disable all interrupts;
	csr_write(CSR_MIE, 0);
	csr_write(CSR_MSTATUS, val);
	csr_write(CSR_MEPC, next_addr);

	if (next_mode == PRV_S) {
		csr_write(CSR_STVEC, next_addr);
		csr_write(CSR_SSCRATCH, 0);
		csr_write(CSR_SIE, 0);
		csr_write(CSR_SATP, 0);
	} else if (next_mode == PRV_U) {
		if (misa_extension('N')) {
			csr_write(CSR_UTVEC, next_addr);
			csr_write(CSR_USCRATCH, 0);
			csr_write(CSR_UIE, 0);
		}
	}
	sbi_hsm_hart_stop(sbi_scratch_thishart_ptr(), false);
	register unsigned long a0 asm("a0") = current_hartid();
	__asm__ __volatile__("mret" : : "r"(a0));
	__builtin_unreachable();
}

static void sbi_ipi_process_slice_op(struct sbi_scratch *scratch)
{
	unsigned int dst_hart = current_hartid();
	for (size_t src_index = 0; src_index < sbi_scratch_last_hartid();
	     ++src_index) {
		switch (slice_ipi_data[src_index][dst_hart].func_id) {
		case SLICE_IPI_SW_STOP:
			slice_ipi_data[src_index][dst_hart].func_id =
				SLICE_IPI_NONE;
			slice_printf("%s: hart %d\n", __func__, dst_hart);
			slice_pmp_init();
			slice_reset_regs(sbi_scratch_thishart_ptr()->fw_start,
					 PRV_M);
			break;
		case SLICE_IPI_NONE:
		default:
			break;
		}
	}
}

struct sbi_ipi_event_ops ipi_slice_op = {
	.name	 = "IPI_SLICE",
	.process = sbi_ipi_process_slice_op,
};

static u32 ipi_slice_event = SBI_IPI_EVENT_MAX;

void slice_ipi_register()
{
	slice_printf("%s: hart%d\n", __func__, current_hartid());
	int ret = sbi_ipi_event_create(&ipi_slice_op);
	if (ret < 0) {
		sbi_hart_hang();
	}
	ipi_slice_event = ret;
}

void slice_send_ipi_to_domain(unsigned int dom_index,
			      enum SliceIPIFuncId func_id)
{
	slice_printf("%s: hart%d: dom_index= %d\n", __func__, current_hartid(),
		     dom_index);
	unsigned int dst_hart, src_hart = current_hartid();
	unsigned long hart_mask = 0;
	struct sbi_domain *dom	= sbi_index_to_domain(dom_index);
	sbi_hartmask_for_each_hart(dst_hart, &dom->assigned_harts)
	{
		slice_ipi_data[src_hart][dst_hart].func_id = func_id;
		hart_mask |= (1 << dst_hart);
	}
	sbi_ipi_send_many(hart_mask, 0, ipi_slice_event, NULL);
	slice_printf("%s: hart%d: sbi_ipi_send_many hart_mark=%lx\n", __func__,
		     current_hartid(), hart_mask);
}

#define SLICE_FDT_OFFSET 0x2000000

int slice_create(unsigned long cpu_mask, unsigned long mem_start,
		 unsigned long mem_size, unsigned long image_from,
		 unsigned long image_size, unsigned long fdt_from)
{
	struct sbi_hartmask mask;
	struct sbi_domain *dom;
	unsigned cpuid = 0,  boot_hartid = -1;
	const struct sbi_platform *plat = sbi_platform_ptr(sbi_scratch_thishart_ptr());
	slice_printf("%s: cpu msk = %lx mem=(%lx %lx) image=(%lx %lx) fdt=%lx",
		     __func__, cpu_mask, mem_start, mem_size, image_from, image_size,
		     fdt_from);
	sbi_hartmask_clear_all(&mask);
	while (cpu_mask) {
		if (cpu_mask & 1) {
			sbi_hartmask_set_hart(cpuid, &mask);
			boot_hartid = cpuid;
		}
		cpu_mask >>= 1;
		cpuid++;
	}
	dom		 = (struct sbi_domain *)slice_allocate_domain(&mask);
	dom->boot_hartid = boot_hartid;
	dom->next_addr	    = mem_start;
	dom->next_arg1	    = mem_start + SLICE_FDT_OFFSET;
	dom->next_mode	    = PRV_S;
	dom->next_boot_src  = image_from;
	dom->next_boot_size = image_size;
	dom->slice_dt_src   = (void *)fdt_from;
	dom->dom_mem_size = mem_size;
	if(sbi_platform_ops(plat)->slice_init_mem_region){
		sbi_platform_ops(plat)->slice_init_mem_region(dom);
	}
	return sbi_domain_register(dom, dom->possible_harts);
}

int slice_delete(int dom_index)
{
	// Only slice_host_hartid is able to delete a slice config.
	if (current_hartid() != slice_host_hartid()) {
		return SBI_ERR_SLICE_SBI_PROHIBITED;
	}
	struct sbi_domain *dom = sbi_index_to_domain(dom_index);
	struct sbi_domain_memregion *region;
	//struct sbi_hartmask free_harts;
	//sbi_hartmask_clear_all(&free_harts);
	int hart_state;
	unsigned int hart_id;
	// Check whether we can delete the slice.
	// Cannot delete a slice if a hart in this slice is still running.
	sbi_hartmask_for_each_hart(hart_id, &dom->assigned_harts)
	{
		hart_state = sbi_hsm_hart_get_state(dom, hart_id);
		switch (hart_state) {
		case SBI_HSM_STATE_STOPPED:
		case SBI_HSM_STATE_STOP_PENDING:
			break;
		default:
			sbi_printf("%s: Cannot delete slice %d: "
				   "hart %d is not fully stopped.\n",
				   __func__, dom_index, hart_id);
			return SBI_ERR_SLICE_NOT_DESTROYABLE;
		}
	}
	// Start to delete the slice.
	for (unsigned int hart_id = 0; hart_id < MAX_HART_NUM; ++hart_id) {
		if (hartid_to_domain_table[hart_id] == dom) {
			hartid_to_domain_table[hart_id] = &root;
			//sbi_hartmask_set_hart(hart_id, &free_harts);
		}
	}
	sbi_domain_for_each_memregion(dom, region)
	{
		sbi_memset(region, 0, sizeof(*region));
	}
	sbi_memset(dom, 0, sizeof(*dom));
	// Move free harts to root domain (slice-host);
	// sbi_hartmask_or(&root.assigned_harts, &root.assigned_harts,
	//		&free_harts);
	return 0;
}