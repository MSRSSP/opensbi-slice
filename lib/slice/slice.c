#include <sbi/sbi_console.h>
#include <sbi/sbi_domain.h>
#include <sbi/sbi_platform.h>
#include <sbi/sbi_hsm.h>
#include <sbi/sbi_string.h>
#include <sbi_utils/fdt/fdt_domain.h>
#include <slice/slice.h>
#include <slice/slice_ecall.h>


int slice_is_domain_boot_hart(int hartid)
{
	struct sbi_domain *dom =
		(struct sbi_domain *)sbi_hartid_to_domain(hartid);
	return (dom->boot_hartid == hartid) ? 1 : 0;
}

static void load_next_stage(const void * dom_ptr){
    const struct sbi_domain * dom = (struct sbi_domain *) dom_ptr;
    void * dst = (void *) dom->next_addr;
    void * src = (void *) dom->next_boot_src;
    if(dst == 0 || src == 0){
        return;
    }
    if(src != dst){
        sbi_memcpy(dst, src, dom->next_boot_size);
        slice_printf("%s: %lx-> %lx\n", __func__, dom->next_boot_src, dom->next_addr);
    }
}

/*static void zero_domain_memory(void * dom_ptr){
    struct sbi_domain * dom = (struct sbi_domain *) dom_ptr;
    struct  sbi_domain_memregion* reg = dom->regions;
    
    while(reg->order>0){
        if(reg->base> 0x90000000 && (reg->flags & SBI_DOMAIN_MEMREGION_WRITEABLE) && !(reg->flags & SBI_DOMAIN_MEMREGION_MMIO)) {
            slice_printf("%s: base= %lx, order= %ld\n", __func__, reg->base, reg->order);
            sbi_memset((void*)reg->base, 0, 1UL << reg->order);
        }
        ++reg;
    }
}*/

int slice_setup_domain(void *dom_ptr)
{
    int ret = 0;
    if(dom_ptr  == 0){
        // This hart does not belong to any domain. 
        return ret;
    }

    struct sbi_domain *dom = (struct sbi_domain *)dom_ptr;
    if (dom->boot_hartid != current_hartid()) {
        return ret;
    }
    //zero_domain_memory(dom_ptr);
    load_next_stage(dom_ptr);
    ret = slice_create_domain_fdt(dom_ptr);
    if(ret){
        return ret;
    }
    sbi_scratch_thishart_ptr()->next_arg1 = dom->next_arg1;
    sbi_scratch_thishart_ptr()->next_addr = dom->next_addr;

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
    inc_domain_counter();
    return dom;
}
