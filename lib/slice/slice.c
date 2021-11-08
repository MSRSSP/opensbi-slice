#include <sbi/riscv_atomic.h>
#include <sbi/riscv_barrier.h>
#include <sbi/sbi_console.h>
#include <sbi/sbi_domain.h>
#include <sbi/sbi_ecall.h>
#include <sbi/sbi_hart.h>
#include <sbi/sbi_hsm.h>
#include <sbi/sbi_platform.h>
#include <sbi/sbi_scratch.h>
#include <sbi/sbi_string.h>
#include <sbi_utils/fdt/fdt_domain.h>
#include <slice/slice.h>
#include <slice/slice_ecall.h>
#include <slice/slice_err.h>
#include <slice/slice_mgr.h>
#include <slice/slice_pmp.h>

bool is_slice(const struct sbi_domain *dom) {
  return dom->slice_type == SLICE_TYPE_SLICE;
}

static unsigned int host_hartid = -1;

unsigned int slice_host_hartid() { return host_hartid; }

int register_host_hartid(unsigned int hartid) {
  if (host_hartid == -1) {
    host_hartid = hartid;
  } else {
    return SBI_ERR_SLICE_UNKNOWN_ERROR;
  }
  return 0;
}

int slice_boot_hart(void) {
  struct sbi_domain *dom = (struct sbi_domain *)sbi_domain_thishart_ptr();
  if (dom->slice_type != SLICE_TYPE_SLICE) {
    return -1;
  }
  return dom->boot_hartid;
}

int slice_activate(struct sbi_domain *dom) {
  enum slice_status oldstate = atomic_cmpxchg(
      &dom->slice_status, SLICE_STATUS_DELETED, SLICE_STATUS_ACTIVE);
  if (oldstate != SLICE_STATUS_DELETED) {
    return SBI_ERR_SLICE_STATUS;
  }
  return 0;
}

int slice_freeze(struct sbi_domain *dom) {
  int oldstate = atomic_cmpxchg(&dom->slice_status, SLICE_STATUS_ACTIVE,
                                SLICE_STATUS_FROZEN);
  if (oldstate != SLICE_STATUS_ACTIVE) {
    return SBI_ERR_SLICE_STATUS;
  }
  return 0;
}

int slice_deactivate(struct sbi_domain *dom) {
  int oldstate = atomic_cmpxchg(&dom->slice_status, SLICE_STATUS_FROZEN,
                                SLICE_STATUS_DELETED);
  if (oldstate != SLICE_STATUS_FROZEN) {
    return SBI_ERR_SLICE_STATUS;
  }
  return 0;
}

int slice_is_active(struct sbi_domain *dom) {
  return atomic_read(&dom->slice_status) == SLICE_STATUS_ACTIVE;
}

int slice_is_existed(struct sbi_domain *dom) {
  return atomic_read(&dom->slice_status) != SLICE_STATUS_DELETED;
}

struct sbi_domain *slice_from_index(unsigned int dom_index) {
  if (dom_index > SBI_DOMAIN_MAX_INDEX) {
    return NULL;
  }
  struct sbi_domain *dom = sbi_index_to_domain(dom_index);
  return slice_is_existed(dom) ? dom : NULL;
}

struct sbi_domain *active_slice_from_index(unsigned int dom_index) {
  struct sbi_domain *dom = slice_from_index(dom_index);
  return slice_is_active(dom) ? dom : NULL;
}

#define SBI_IMAGE_SIZE 0x200000
static void load_next_stage(const void *dom_ptr) {
  unsigned long startTicks = csr_read(CSR_MCYCLE);
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
  sbi_printf("%s: hart %d: #ticks = %lu\n", __func__, current_hartid(),
             csr_read(CSR_MCYCLE) - startTicks);
}

/*
static void zero_slice_memory(void *dom_ptr)
{
        unsigned long startTicks = csr_read(CSR_MCYCLE);
        struct sbi_domain *dom = (struct sbi_domain *)dom_ptr;
        if (dom->next_addr) {
                slice_printf("Zero Slice Mem (%lx, %lx).\n", dom->next_addr,
                                 dom->slice_mem_start - dom->next_addr +
dom->slice_mem_size); sbi_memset((void *) dom->next_addr, 0,
dom->slice_mem_start - dom->next_addr + dom->slice_mem_size);
        }
        sbi_printf("%s: hart %d: #ticks = %lu\n", __func__, current_hartid(),
                   csr_read(CSR_MCYCLE) - startTicks);
}
*/

static void __attribute__((noreturn))
slice_jump_sbi(unsigned long next_addr, unsigned long next_mode) {
#if __riscv_xlen == 32
  unsigned long val, valH;
#else
  unsigned long val;
#endif
  sbi_printf("%s(): next_addr=%#lx next_mode=%#lx\n", __func__, next_addr,
             next_mode);

  switch (next_mode) {
    case PRV_M:
      break;
    case PRV_S:
    case PRV_U:
    default:
      sbi_hart_hang();
  }

  val = csr_read(CSR_MSTATUS);
  val = INSERT_FIELD(val, MSTATUS_MPP, next_mode);
  val = INSERT_FIELD(val, MSTATUS_MPIE, 0);
#if __riscv_xlen == 32
  if (misa_extension('H')) {
    valH = csr_read(CSR_MSTATUSH);
    valH = INSERT_FIELD(valH, MSTATUSH_MPV, 0);
    csr_write(CSR_MSTATUSH, valH);
  }
#else
  if (misa_extension('H')) {
    val = INSERT_FIELD(val, MSTATUS_MPV, 0);
  }
#endif
  // Disable all interrupts;
  csr_write(CSR_MIE, 0);
  csr_write(CSR_MSTATUS, val);
  csr_write(CSR_MEPC, next_addr);

  if (next_mode == PRV_S) {
    csr_write(CSR_STVEC, next_addr);
    csr_write(CSR_SSCRATCH, 0);
    csr_write(CSR_SIE, 0);
    csr_write(CSR_SATP, 0);
  } else if (next_mode == PRV_U) {
    if (misa_extension('N')) {
      csr_write(CSR_UTVEC, next_addr);
      csr_write(CSR_USCRATCH, 0);
      csr_write(CSR_UIE, 0);
    }
  }
  register unsigned long a0 asm("a0") =
      (unsigned long)sbi_scratch_thishart_ptr();
  __asm__ __volatile__("mret" : : "r"(a0));
  __builtin_unreachable();
}

void __attribute__((noreturn))
slice_to_sbi(void *slice_mem_start, void *slice_sbi_start,
             unsigned long slice_sbi_size) {
  slice_printf("%s: hart%d: next sbi is %p (copy from %p)\n", __func__,
               current_hartid(), slice_mem_start, slice_sbi_start);
  if (slice_sbi_size > 0) {
    sbi_memcpy(slice_mem_start, slice_sbi_start, slice_sbi_size);
  }
  slice_jump_sbi((unsigned long)slice_mem_start, PRV_M);
  __builtin_unreachable();
}

void nonslice_sbi_init(void) {
  csr_write(CSR_STVEC, 0);
  emptyslice_setup_pmp();
}

int slice_setup_domain(void *dom_ptr) {
  slice_printf("%s: hart%d\n", __func__, current_hartid());
  ulong start_slice_tick = csr_read(CSR_MCYCLE);
  int ret = 0;
  if (dom_ptr == 0) {
    // This hart does not belong to any domain.
    slice_printf("%s: hart%d dom_ptr == empty\n", __func__, current_hartid());
    return ret;
  }

  struct sbi_domain *dom = (struct sbi_domain *)dom_ptr;

  if (!is_slice(dom) || !slice_is_active(dom)) {
    emptyslice_setup_pmp();
    return 0;
  }
  ret = slice_setup_pmp(dom_ptr);
  if (ret) {
    return ret;
  }
  if (dom->boot_hartid != current_hartid()) {
    return ret;
  }
  // cleared by slice bootloader before sbi_init
  // zero_slice_memory(dom_ptr);
  load_next_stage(dom_ptr);
  ret = slice_create_domain_fdt(dom_ptr);
  if (ret) {
    return ret;
  }
  sbi_scratch_thishart_ptr()->next_arg1 = dom->next_arg1;
  sbi_scratch_thishart_ptr()->next_addr = dom->next_addr;
  unsigned long end_slice_tick = csr_read(CSR_MCYCLE);
  sbi_printf("%s: hart %d: #ticks: %lu\n", __func__, current_hartid(),
             end_slice_tick - start_slice_tick);
  return ret;
}

void *slice_allocate_domain(struct sbi_hartmask *input_mask) {
  if (read_domain_counter() >= FDT_DOMAIN_MAX_COUNT) {
    return 0;
  }
  struct sbi_hartmask *mask = allocate_hartmask();
  struct sbi_domain *dom = allocate_domain();
  struct sbi_domain_memregion *regions = allocate_memregion();
  SBI_HARTMASK_INIT(mask);
  sbi_memcpy(mask, input_mask, sizeof(*mask));
  sbi_memset(regions, 0, sizeof(*regions) * (FDT_DOMAIN_REGION_MAX_COUNT + 1));
  dom->regions = regions;
  dom->possible_harts = mask;
  dom->slice_type = SLICE_TYPE_SLICE;
  inc_domain_counter();
  return dom;
}

void dump_slice_config(const struct sbi_domain *dom) {
  sbi_printf("slicetype = %d\n", dom->slice_type);
  sbi_printf("next_boot_src = %lx\n", dom->next_boot_src);
  sbi_printf("next_boot_size = %lx\n", (unsigned long)dom->next_boot_size);
  sbi_printf("slice_dt_src = %lx\n", (unsigned long)dom->slice_dt_src);
  sbi_printf("slice_mem_size = %lx\n", dom->slice_mem_size);
}
