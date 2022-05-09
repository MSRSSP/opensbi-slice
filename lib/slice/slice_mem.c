#include "config.h"
#include <sbi/sbi_string.h>
#include <sbi/sbi_hart.h>
#include <slice/slice.h>

static unsigned long slice0_private_avail = CONFIG_SERVICE_BOOT_DDR_SLICE_PRIVATE_START;
#define SLICE0_PRIVATE_END CONFIG_SERVICE_BOOT_DDR_SLICE_FW_START

void* slice0_alloc_private(unsigned size){
	unsigned long ret = slice0_private_avail;
        if(slice0_private_avail+size > SLICE0_PRIVATE_END){
		slice_error("%s: need more private mem.\n", __func__);
                sbi_hart_hang();
	}
	slice0_private_avail += size;
	return (void*)ret;
}