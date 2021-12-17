#include <sbi/sbi_domain.h>
#include <assert.h>
#include <sbi/sbi_types.h>
#include <sbi/sbi_console.h>

#include "config.h"
#include "hss_types.h"
#include "cache.h"

#define SET_CACHE_MASK_BY_HART(hartid, mask) {case hartid: {\
        CACHE_CTRL->WAY_MASK_U54_##hartid##_ICACHE = mask; \
        CACHE_CTRL->WAY_MASK_U54_##hartid##_DCACHE = mask; \
        sbi_printf("set cache mask.\n"); \
        break; \
        }}

#define PRINT_CACHE_MASK_BY_HART(hartid) {\
        sbi_printf("hartid= "#hartid": %lx %lx\n",CACHE_CTRL->WAY_MASK_U54_##hartid##_ICACHE, CACHE_CTRL->WAY_MASK_U54_##hartid##_DCACHE); \
        }

void slice_process_cache_mask(int dom_index, unsigned long value){
	struct sbi_domain* dom = slice_from_index(dom_index);
  sbi_printf("%s(%d, %lx)\n", __func__, dom_index, value);
  if(dom){
    unsigned int hartid;
    sbi_printf("setting cache mask %lx... \n", value);
    sbi_hartmask_for_each_hart(hartid, dom->possible_harts){
      switch(hartid){ 
        SET_CACHE_MASK_BY_HART(1, value)
        SET_CACHE_MASK_BY_HART(2, value)
        SET_CACHE_MASK_BY_HART(3, value)
        SET_CACHE_MASK_BY_HART(4, value)
        default:
          break;
      }
    }
  }else{
    if(value > CACHE_CTRL->WAY_ENABLE){
      CACHE_CTRL->WAY_ENABLE = value;
    }
  }
  sbi_printf("CACHE_CTRL->WAY_ENABLE=%d\nCache mask:\n",     
              CACHE_CTRL->WAY_ENABLE);
  sbi_printf("hart 0: %lx %lx\n", CACHE_CTRL->WAY_MASK_E51_ICACHE, CACHE_CTRL->WAY_MASK_E51_DCACHE);
  PRINT_CACHE_MASK_BY_HART(1)
  PRINT_CACHE_MASK_BY_HART(2)
  PRINT_CACHE_MASK_BY_HART(3)
  PRINT_CACHE_MASK_BY_HART(4)
}
