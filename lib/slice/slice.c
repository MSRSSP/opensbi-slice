#include <sbi/sbi_console.h>
#include <sbi/sbi_domain.h>
#include <sbi/sbi_platform.h>
#include <sbi/sbi_hsm.h>
#include <sbi/sbi_string.h>
#include <sbi_utils/fdt/fdt_domain.h>
#include <slice/slice.h>
#include <slice/slice_ecall.h>
#include <slice/slice_err.h>
#include <slice/slice_pmp.h>
#include <sbi/riscv_barrier.h>

bool is_slice(const struct sbi_domain *dom)
{
    return dom->slice_type == SLICE_TYPE_SLICE;
}

static unsigned int host_hartid = -1;

unsigned int slice_host_hartid(){
    return host_hartid;
}

int register_host_hartid(unsigned int hartid){
    if(host_hartid== -1){
        host_hartid = hartid;
    }else{
        return SBI_ERR_SLICE_UNKNOWN_ERROR;
    }
    return 0;
}

int slice_is_domain_boot_hart(int hartid)
{
	struct sbi_domain *dom =
		(struct sbi_domain *)sbi_hartid_to_domain(hartid);
	return (dom->boot_hartid == hartid) ? 1 : 0;
}


static void load_next_stage(const void * dom_ptr){
    unsigned long startTicks = csr_read(CSR_MCYCLE);
    const struct sbi_domain * dom = (struct sbi_domain *) dom_ptr;
    void * dst = (void *) dom->next_addr;
    void * src = (void *) dom->next_boot_src;
    if(dst == 0 || src == 0){
        return;
    }
    if(src != dst){
	    slice_printf("%s: hart %d: %lx-> %lx, %x\n", __func__, current_hartid(),
			 dom->next_boot_src, dom->next_addr, dom->next_boot_size);
	    sbi_memcpy(dst, src, dom->next_boot_size);
    }
    sbi_printf("%s: hart %d: #ticks = %lu\n", __func__, current_hartid(),
		   csr_read(CSR_MCYCLE) - startTicks);
}

static void zero_slice_memory(void * dom_ptr){
	unsigned long startTicks = csr_read(CSR_MCYCLE);
    struct sbi_domain *dom = (struct sbi_domain *)dom_ptr;
	if (dom->next_addr) {
		slice_printf("Zero Slice Mem (%lx, %lx).\n", dom->next_addr,
			     dom->dom_mem_size);
		sbi_memset((void *)dom->next_addr, 0, dom->dom_mem_size);
	}
	sbi_printf("%s: hart %d: #ticks = %lu\n", __func__, current_hartid(),
		   csr_read(CSR_MCYCLE) - startTicks);
}

int slice_setup_domain(void *dom_ptr)
{
    ulong start_slice_tick = csr_read(CSR_MCYCLE);
    int ret = 0;
    if(dom_ptr  == 0){
        // This hart does not belong to any domain. 
        return ret;
    }

    struct sbi_domain *dom = (struct sbi_domain *)dom_ptr;
    
    if(!is_slice(dom)){
        nonslice_setup_pmp(dom_ptr);
        return 0;
    }
    ret = slice_setup_pmp(dom_ptr);
    if(ret){
        return ret;
    }
    if (dom->boot_hartid != current_hartid()) {
        return ret;
    }
    zero_slice_memory(dom_ptr);
    load_next_stage(dom_ptr);
    ret = slice_create_domain_fdt(dom_ptr);
    if(ret){
        return ret;
    }
    sbi_scratch_thishart_ptr()->next_arg1 = dom->next_arg1;
    sbi_scratch_thishart_ptr()->next_addr = dom->next_addr;
    unsigned long end_slice_tick = csr_read(CSR_MCYCLE);
	sbi_printf("%s: hart %d: #ticks: %lu\n", __func__, current_hartid(), end_slice_tick-start_slice_tick);
    return ret;
    // TODO(ziqiao): relocate scratches and hart states to domain memory.
}

/*#ifdef PROTECT_DOMAIN_CONFIG
void allocate_fdt_domain_config()
{
	unsigned long addr = FDT_START;
	fdt_domains	   = (struct sbi_domain *)addr;
	addr += FDT_DOMAIN_MAX_COUNT * sizeof(struct sbi_domain);
	fdt_masks = (struct sbi_hartmask *)addr;
	addr += FDT_DOMAIN_MAX_COUNT * sizeof(struct sbi_hartmask);
	fdt_regions = (struct sbi_domain_memregion **)addr;
	addr += FDT_DOMAIN_MAX_COUNT * sizeof(struct sbi_domain_memregion *);
	for (size_t i = 0; i < FDT_DOMAIN_REGION_MAX_COUNT; ++i) {
		fdt_regions[i] = (struct sbi_domain_memregion *)addr;
		addr += sizeof(struct sbi_domain_memregion);
	}

    slice_printf("%s\n", __func__);
}
#endif
*/

void *slice_allocate_domain(struct sbi_hartmask * input_mask)
{
    if(read_domain_counter() >= FDT_DOMAIN_MAX_COUNT){
        return 0;
    }
    struct sbi_hartmask * mask = allocate_hartmask();
	struct sbi_domain *dom = allocate_domain();
	struct sbi_domain_memregion *regions = allocate_memregion();
    SBI_HARTMASK_INIT(mask);
    sbi_memcpy(mask, input_mask, sizeof(*mask));
	sbi_memset(regions, 0,
	       sizeof(*regions) * (FDT_DOMAIN_REGION_MAX_COUNT + 1));
	dom->regions = regions;
    dom->possible_harts = mask;
    dom->slice_type = SLICE_TYPE_SLICE;
    inc_domain_counter();
    return dom;
}

void dump_slice_config(const struct sbi_domain *dom)
{
	sbi_printf("slicetype = %d\n", dom->slice_type);
    sbi_printf("next_boot_src = %lx\n", dom->next_boot_src);
    sbi_printf("next_boot_size = %lx\n", (unsigned long) dom->next_boot_size);
    sbi_printf("slice_dt_src = %lx\n", (unsigned long)dom->slice_dt_src);
    sbi_printf("dom_mem_size = %lx\n", dom->dom_mem_size);
}