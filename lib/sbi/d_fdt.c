#include <libfdt.h>
#include <sbi/d.h>
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

void _print_fdt(void * fdt, int node, char* prefix){
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

void print_fdt(void * fdt){
   int root = fdt_path_offset(fdt, "/");
   char prefix[64]="";
   _print_fdt(fdt, root, prefix);
}

void relocate_fdt(const void * src_fdt, void * dst_fdt){
    if(dst_fdt==0){
        return;
    }
    if(fdt_totalsize(dst_fdt)==0 && (long)src_fdt != (long)dst_fdt){
        d_printf("duplicate %lx -> %lx", (unsigned long)src_fdt, (unsigned long)dst_fdt);
        sbi_memcpy(dst_fdt, src_fdt, fdt_totalsize(src_fdt) );
    }
}

int d_create_domain_fdt(const void * dom_ptr){
    const struct sbi_domain * domain = (const struct sbi_domain *) dom_ptr;
    if(domain->boot_hartid != current_hartid()){
        return 0;
    }
    void * fdt = (void *) domain->next_arg1;
    if(fdt == NULL){
        return 0;
    }
    d_printf("Hart-%d: %s: fdt=%lx\n", current_hartid(), __func__, domain->next_arg1);

    relocate_fdt(sbi_scratch_thishart_arg1_ptr(), fdt);
    fdt_cpu_fixup(fdt, dom_ptr);
    fdt_serial_fixup(fdt, dom_ptr);
    fdt_fixups(fdt, dom_ptr);
    fdt_domain_fixup(fdt, dom_ptr);
    //print_fdt(fdt);
    return 0;
}