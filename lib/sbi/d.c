#include <sbi/sbi_console.h>
#include <sbi/sbi_domain.h>
#include <sbi/sbi_platform.h>
#include <sbi/sbi_hsm.h>
#include <sbi/sbi_string.h>
#include <sbi/d.h>
#include <sbi/d_ecall.h>

int sbi_is_domain_boot_hart(int hartid){
	struct sbi_domain * dom = (struct sbi_domain *)sbi_hartid_to_domain(hartid);
	return (dom->boot_hartid == hartid)?1:0;
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
        d_printf("%s: %lx-> %lx\n", __func__, dom->next_boot_src, dom->next_addr);
    }
}

/*static void zero_domain_memory(void * dom_ptr){
    struct sbi_domain * dom = (struct sbi_domain *) dom_ptr;
    struct  sbi_domain_memregion* reg = dom->regions;
    
    while(reg->order>0){
        if(reg->base> 0x90000000 && (reg->flags & SBI_DOMAIN_MEMREGION_WRITEABLE) && !(reg->flags & SBI_DOMAIN_MEMREGION_MMIO)) {
            d_printf("%s: base= %lx, order= %ld\n", __func__, reg->base, reg->order);
            sbi_memset((void*)reg->base, 0, 1UL << reg->order);
        }
        ++reg;
    }
}*/

int prepare_domain_memory(void * dom_ptr){
    struct sbi_domain * dom = (struct sbi_domain *) dom_ptr;
    if(dom->boot_hartid!= current_hartid()){
        return 0;
    }
    //zero_domain_memory(dom_ptr);
    load_next_stage(dom_ptr);
    return d_create_domain_fdt(dom_ptr);
    // TODO(ziqiao): relocate scratches and hart states to domain memory.
}

