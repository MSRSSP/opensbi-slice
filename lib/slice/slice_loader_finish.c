#include <libfdt.h>
#include <sbi/riscv_atomic.h>
#include <sbi/riscv_barrier.h>

#include <sbi/sbi_console.h>
#include <sbi/sbi_domain.h>
#include <sbi/sbi_hart.h>
#include <sbi/sbi_init.h>
#include <sbi/sbi_scratch.h>
#include <sbi/sbi_string.h>
#include <slice/slice.h>
#include <slice/slice_fw.h>
#include <slice/slice_mgr.h>
#include <slice/slice_pmp.h>

static void load_next_stage(const void *dom_ptr) {
  const struct sbi_domain *dom = (struct sbi_domain *)dom_ptr;
  void *dst = (void *)dom->next_addr;
  void *src = (void *)dom->next_boot_src;
  if (dst == 0 || src == 0) {
    return;
  }
  if (src != dst) {
    slice_printf("%s: hart %d: %lx-> %lx, %x\n", __func__, current_hartid(),
                 dom->next_boot_src, dom->next_addr, dom->next_boot_size);
    sbi_memcpy(dst, src, dom->next_boot_size);
  }
}

unsigned long private_mem_time[5] ={0};
unsigned long loader_to_untrusted_time[5];

static void slice_copy_to_private_mem(struct sbi_domain *dom) {
  unsigned long startTicks = csr_read(CSR_MCYCLE);
  slice_copy_fdt(dom);
  load_next_stage(dom);
  private_mem_time[current_hartid()] = csr_read(CSR_MCYCLE) - startTicks;
}

// extern struct sbi_domain * root;
extern void __noreturn sbi_slice_init(struct sbi_scratch *scratch, bool coldboot);
extern int slice_config_domain_fdt(const struct sbi_domain *dom);
static int slice_loader_state = 0;
static struct sbi_hartmask root_hmask;
#define ROOT_REGION_MAX	16
//static  struct sbi_domain_memregion private_memregs[ROOT_REGION_MAX + 1] = { 0 };

struct sbi_domain_memregion private_memregs[ROOT_REGION_MAX + 1] = { 0 };
static void __noreturn slice_loader_finish_guest(struct sbi_scratch *private_scratch, struct sbi_domain *shared_dom) {
  struct sbi_domain *dom, private_dom;
  //struct sbi_domain_memregion* root_memregs;
  struct sbi_domain_memregion local_memregs[ROOT_REGION_MAX + 1] = { 0 };
  struct sbi_hartmask local_hmask;
  bool is_boot_hartid = (shared_dom->boot_hartid == current_hartid());
  sbi_console_init(private_scratch);
  if(is_boot_hartid){
    dom = &root;
    //root_memregs = dom->regions;
    sbi_memcpy(dom, shared_dom, sizeof(*dom));
    sbi_memcpy(&root_hmask, shared_dom->possible_harts, sizeof(root_hmask));
    sbi_memcpy(private_memregs, shared_dom->regions, sizeof(struct sbi_domain_memregion)*ROOT_REGION_MAX);
    dom->regions = private_memregs;
    dom->possible_harts = &root_hmask;
    slice_copy_to_private_mem(dom);
    __smp_store_release(&slice_loader_state, 1);
  }else{
    dom = &private_dom;
    sbi_memcpy(dom, shared_dom, sizeof(*dom));
    sbi_memcpy(local_memregs, shared_dom->regions, sizeof(struct sbi_domain_memregion)*ROOT_REGION_MAX);
    sbi_memcpy(&local_hmask, shared_dom->possible_harts, sizeof(local_hmask));
    dom->possible_harts = &local_hmask;
    dom->regions = local_memregs;
  }
  loader_to_untrusted_time[current_hartid()] = csr_read(CSR_MCYCLE);
  slice_setup_pmp(dom);
  //nonslice_setup_pmp();
  if(is_boot_hartid){
    slice_config_domain_fdt(dom);
  }
  while(!__smp_load_acquire(&slice_loader_state)){
  }
  sbi_slice_init(private_scratch, is_boot_hartid);
}

static void slice_loader_finish_nonslice(struct sbi_scratch *private_scratch, struct sbi_domain *shared_dom) {
  root.next_addr =  shared_dom->next_addr;
  root.next_arg1 =  shared_dom->next_arg1;
  nonslice_setup_pmp();
}

void __noreturn slice_loader_finish(struct sbi_scratch *private_scratch,
                         struct sbi_domain *shared_dom) {
  // ulong start_slice_tick = csr_read(CSR_MCYCLE);
  bool is_boot_hartid = (shared_dom->boot_hartid == current_hartid());
  if(is_slice(shared_dom) && slice_is_active(shared_dom)){
    slice_loader_finish_guest(private_scratch, shared_dom);
    sbi_slice_init(private_scratch, is_boot_hartid);
    __builtin_unreachable();
  }else if(is_slice(shared_dom)&& !slice_is_active(shared_dom)){
    emptyslice_setup_pmp();
    wfi();
    __builtin_unreachable();
  }else{
    slice_loader_finish_nonslice(private_scratch, shared_dom);
    sbi_init(private_scratch);
    __builtin_unreachable();
  }
}