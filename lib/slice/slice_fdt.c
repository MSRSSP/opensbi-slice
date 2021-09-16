#include <libfdt.h>
#include <sbi/sbi_console.h>
#include <sbi/sbi_domain.h>
#include <sbi/sbi_math.h>
#include <sbi/sbi_hart.h>
#include <sbi/sbi_scratch.h>
#include <sbi/sbi_string.h>
#include <sbi_utils/fdt/fdt_fixup.h>
#include <sbi_utils/fdt/fdt_domain.h>
#include <sbi_utils/fdt/fdt_helper.h>
#include <sbi_utils/serial/fdt_serial.h>
#include <sbi_utils/reset/fdt_reset.h>
#include <sbi/sbi_platform.h>
#include <slice/slice.h>

void _print_fdt(const void * fdt, int node, char* prefix){
    int property, child, fixup_len, plen, len;
    const fdt32_t * fixup_val;
    const char *name, * str_val;
    fdt_for_each_property_offset(property, fdt, node) {
		fixup_val = fdt_getprop_by_offset(fdt, property,
                    &name, &fixup_len);
        str_val = (const char *) fixup_val;
        sbi_printf("%s%s = ", prefix, name);
        if(sbi_isprintable(str_val[0])){
            sbi_printf("%s\n", str_val);
        }else{
            for(size_t i = 0; i< fixup_len/4; ++i){
                sbi_printf("%x ",fdt32_to_cpu(fixup_val[i]));
            }
            sbi_printf("\n");
        }
    }
    plen =  strlen(prefix);
    fdt_for_each_subnode(child, fdt, node) {
        sbi_printf("%s%s{\n",prefix, fdt_get_name(fdt, child, &len));
        prefix[plen] = '\t';
        _print_fdt(fdt, child, prefix);
        prefix[plen] = 0;
        sbi_printf("%s}\n", prefix);
    }
}

void slice_print_fdt(const void * fdt){
   int root_fdt = fdt_path_offset(fdt, "/");
   char prefix[64]="";
   _print_fdt(fdt, root_fdt, prefix);
}

void copy_fdt(const void * src_fdt, void * dst_fdt){
    if(dst_fdt==0){
        return;
    }
    if(fdt_totalsize(dst_fdt)==0 && (long)src_fdt != (long)dst_fdt){
        slice_printf("duplicate %lx -> %lx", (unsigned long)src_fdt, (unsigned long)dst_fdt);
        sbi_memcpy(dst_fdt, src_fdt, fdt_totalsize(src_fdt) );
    }
}

static const struct fdt_match riscv_cpu_match[] = {
	{ .compatible = "riscv" },
};

int slice_remove_useless_cpus(void * fdt, const void * dom_ptr){
    int noff=0, prevnoff=0, err;
    u32 hartid;
    const struct fdt_match *match;
    while (true){
        prevnoff = noff;
        noff = fdt_find_match(fdt, noff, riscv_cpu_match, &match);
        if (noff < 0)
            break;
        err = fdt_parse_hart_id(fdt, noff, &hartid);
        if(err){
            slice_printf("err hart %d\n", hartid);
        }
        slice_printf("Check hart %d in dom %lx\n", hartid, (unsigned long)dom_ptr);

        if (hartid!=0 && sbi_hartid_to_domain(hartid) != dom_ptr){
            slice_printf("remove hart %d\n", hartid);
            fdt_nop_node(fdt, noff);
            noff = prevnoff;
        }
	}
    return 0;
}

bool slice_is_accessible(unsigned long addr, unsigned long size, const void* dom_ptr){
    struct sbi_domain_memregion *reg;
	const struct sbi_domain *dom = dom_ptr;
    sbi_domain_for_each_memregion(dom, reg) {
		/* Ignore MMIO or READABLE or WRITABLE or EXECUTABLE regions */

        // remove the unit as it is not accessible
        if(addr >= reg->base && (addr+size) <= (reg->base + (1<<reg->order))  ){
            if (reg->flags & SBI_DOMAIN_MEMREGION_READABLE)
			    return true;
		    if (reg->flags & SBI_DOMAIN_MEMREGION_WRITEABLE)
			    return true;
		    if (reg->flags & SBI_DOMAIN_MEMREGION_EXECUTABLE)
			    return true;
        }
    }
    return false;
}

static const struct fdt_match fixup_device_match_table[] = {
    { .compatible = "syscon-reboot"},
    { },
};

int fdt_reset_device_fixup(void * fdt, const void * dom_ptr)
{
    int len=-1;
    int node = 0, hartid;
    const struct fdt_match *match;
    const u32 *val;
    int to_remove[64];
    int n_remove = 0;
    for(int i=0; i< sbi_scratch_last_hartid(); ++i ){
        slice_printf("fdt=%lx node=%d fixup_device_match_table=%lx\n", (unsigned long)fdt, node, (unsigned long)fixup_device_match_table);
        node = fdt_find_match(fdt, node, fixup_device_match_table, &match);
        slice_printf("fdt_find_match=%d\n", node);
        if(node<0){
            break;
        }
        val = fdt_getprop(fdt, node, "cpu", &len);
        if(val == NULL){
            continue;
        }
        hartid = fdt32_to_cpu(*val);
        if (sbi_hartid_to_domain(hartid) == dom_ptr){
            continue;
        }
        to_remove[n_remove] = node;
        n_remove++;
    }
    for(; n_remove>0; n_remove--){
        fdt_nop_node(fdt, to_remove[n_remove-1]);
    }
    slice_printf("end node=%d\n", node);
    return 0;
}

int slice_create_domain_fdt(const void * dom_ptr){
    const struct sbi_domain * domain = (const struct sbi_domain *) dom_ptr;
    void * fdt_src, *fdt;
    if(domain->boot_hartid != current_hartid()){
        return 0;
    }
    fdt = (void *) domain->next_arg1;
    if(fdt == NULL){
        return 0;
    }
    slice_printf("Hart-%d: %s: fdt=%lx\n", current_hartid(), __func__, domain->next_arg1);
    // TODO: reset domain memory;
    if(domain->slice_dt_src){
        fdt_src = domain->slice_dt_src;
    }else{
        fdt_src = (void*)root.next_arg1;
    }
    copy_fdt(fdt_src, fdt);
    fdt_cpu_fixup(fdt, dom_ptr);
    // Cannot remove cpu0;
    // If exposing only cpu0, cpu3, cpu4, kernel would panic
    fdt_reset_device_fixup(fdt, dom_ptr);
    slice_printf("%s: --fdt_device_fixup \n", __func__);
    if(domain->slice_dt_src == NULL){
        fdt_serial_fixup(fdt, dom_ptr);
        slice_printf("%s: --fdt_serial_fixup \n", __func__);
    }
    fdt_fixups(fdt, dom_ptr);
    slice_printf("%s: --fdt_fixups \n", __func__);
    fdt_domain_fixup(fdt, dom_ptr);
    slice_printf("%s: --fdt_domain_fixup \n", __func__);
    return 0;
}