#include <libfdt.h>
#include <libfdt_env.h>
#include <sbi/sbi_domain.h>
#include <sbi/riscv_encoding.h>
#include <sbi/sbi_error.h>
#include <sbi/sbi_hartmask.h>
#include <sbi/sbi_scratch.h>
#include <sbi_utils/fdt/fdt_domain.h>
#include <sbi_utils/fdt/fdt_helper.h>

u32 fdt_domains_count;
struct sbi_domain fdt_domains[FDT_DOMAIN_MAX_COUNT];
struct sbi_hartmask fdt_masks[FDT_DOMAIN_MAX_COUNT];
struct sbi_domain_memregion
	fdt_regions[FDT_DOMAIN_MAX_COUNT][FDT_DOMAIN_REGION_MAX_COUNT + 1];

struct sbi_domain * allocate_domain(void){
	return &fdt_domains[fdt_domains_count];
}

struct sbi_hartmask * allocate_hartmask(void){
	return &fdt_masks[fdt_domains_count];
}

struct sbi_domain_memregion * allocate_memregion(void){
	return fdt_regions[fdt_domains_count];
}

void inc_domain_counter(void){
	fdt_domains_count++;
}

unsigned read_domain_counter(void){
	return fdt_domains_count;
}
