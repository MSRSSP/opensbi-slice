#include <libfdt.h>
#include <libfdt_env.h>
#include <sbi/sbi_domain.h>
#include <sbi/riscv_encoding.h>
#include <sbi/sbi_error.h>
#include <sbi/sbi_hartmask.h>
#include <sbi/sbi_scratch.h>
#include <sbi_utils/fdt/fdt_domain.h>
#include <sbi_utils/fdt/fdt_helper.h>

u32 current_dom_id;
struct sbi_domain fdt_domains[FDT_DOMAIN_MAX_COUNT];
struct sbi_hartmask fdt_masks[FDT_DOMAIN_MAX_COUNT];
struct sbi_domain_memregion
	fdt_regions[FDT_DOMAIN_MAX_COUNT][FDT_DOMAIN_REGION_MAX_COUNT + 1];

u32 hart_count(struct sbi_hartmask* mask) {
	unsigned hartid;
	u32 count = 0;
	sbi_hartmask_for_each_hart(hartid, mask) {
		count++;
	}
	return count;
}

u32 fdt_domains_count(void){
	u32 i = 0;
	u32 fdt_domains_count = 0;
	for (i=0; i < FDT_DOMAIN_MAX_COUNT; ++i ) {
		if (hart_count(&fdt_masks[i]) != 0) {
			fdt_domains_count++;
		}
	}
	return fdt_domains_count;
}

u32 allocate_domain_id(void) {
	u32 i = 0;
	for (i=0; i < FDT_DOMAIN_MAX_COUNT; ++i ) {
		if (hart_count(&fdt_masks[i]) == 0) {
			current_dom_id = i;
			 slice_log_printf(SLICE_LOG_INFO, "%s: domid = %d\n", __func__, i);
			return i;
		}
	}
	return -1;
}

struct sbi_domain * allocate_domain(u32 id){
	return &fdt_domains[id];
}

struct sbi_hartmask * allocate_hartmask(u32 id){
	return &fdt_masks[id];
}

struct sbi_domain_memregion * allocate_memregion(u32 id){
	return fdt_regions[id];
}
