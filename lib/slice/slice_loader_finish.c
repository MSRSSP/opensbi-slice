#include <libfdt.h>
#include <sbi/riscv_atomic.h>
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

static void copy_fdt(const void *src_fdt, void *dst_fdt) {
  unsigned long startTicks = csr_read(CSR_MCYCLE);
  if (dst_fdt && (fdt_totalsize(src_fdt) != 0)) {
    slice_printf("duplicate %lx -> %lx\n", (unsigned long)src_fdt,
                 (unsigned long)dst_fdt);
    sbi_memcpy(dst_fdt, src_fdt, fdt_totalsize(src_fdt));
  }
  sbi_printf("%s: hart %d: #ticks = %lu\n", __func__, current_hartid(),
             csr_read(CSR_MCYCLE) - startTicks);
}

void slice_copy_fdt(const struct sbi_domain *dom_ptr) {
  const struct sbi_domain *domain = (const struct sbi_domain *)dom_ptr;
  void *fdt_src, *fdt;
  if (domain->boot_hartid != current_hartid()) {
    return;
  }
  fdt = slice_fdt(domain);
  if (fdt == NULL) {
    return;
  }
  if (domain->slice_dt_src) {
    fdt_src = domain->slice_dt_src;
  } else {
    fdt_src = (void *)root.next_arg1;
  }
  copy_fdt(fdt_src, fdt);
}

static void slice_copy_to_private_mem(struct sbi_domain *dom) {
  slice_copy_fdt(dom);
  load_next_stage(dom);
}

// extern struct sbi_domain * root;
int slice_loader_state;
void slice_loader_finish(struct sbi_scratch *private_scratch,
                         struct sbi_domain *shared_dom) {
  slice_loader_state = 0;
  // ulong start_slice_tick = csr_read(CSR_MCYCLE);
  struct sbi_domain *dom = &root;
  sbi_printf("copy dom from shared to private");
  slice_loader_state = 1;
  sbi_memcpy(dom, shared_dom, sizeof(*dom));
  slice_loader_state = 2;
  slice_copy_to_private_mem(dom);
  slice_loader_state = 3;
  // copy parameter into private MEM;
  if (is_slice(dom) && !slice_is_active(dom)) {
    emptyslice_setup_pmp();
    return;
  } else if (!is_slice(dom)) {
    // Non-slice: standard OpenSBI domain;
    nonslice_setup_pmp(dom);
  }
  if (is_slice(dom)) {
    if (slice_setup_pmp(dom)) {
      sbi_hart_hang();
    }
  }
  private_scratch->next_arg1 = dom->next_arg1;
  private_scratch->next_addr = dom->next_addr;
  slice_loader_state = 4;
  // sbi_printf("%s: hart %d: #ticks: %lu\n", __func__, current_hartid(),
  //           csr_read(CSR_MCYCLE) - start_slice_tick);
  sbi_init(private_scratch);
}
