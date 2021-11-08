#include <sbi/riscv_asm.h>
#include <sbi/sbi_console.h>
#include <sbi/sbi_domain.h>
#include <sbi/sbi_hart.h>
#include <sbi/sbi_ipi.h>
#include <sbi/sbi_math.h>
#include <sbi/sbi_scratch.h>
#include <sbi/sbi_string.h>
#include <slice/slice.h>
#include <slice/slice_err.h>

#define CONFIG_SLICE_SW_RESET 0
#if CONFIG_SLICE_SW_RESET
#define SLICE_PMP_L 0x0
#else
#define SLICE_PMP_L PMP_L
#endif

// don't allow OpenSBI to play with PMPs
int sbi_hart_pmp_configure(struct sbi_scratch *pScratch) { return 0; }

static int detect_region_covered_by_pmp(uintptr_t addr, uintptr_t size) {
  unsigned long pmp_base, pmp_end, pmp_end2, input_end;
  unsigned long pmp_log2size, pmp_log2size2;
  unsigned long prot_out;
  int region_overlap = 0, i;
  unsigned n_pmp = sbi_hart_pmp_count(sbi_scratch_thishart_ptr());
  // Safety check the addr+size
  unsigned long long input_end_tocheck;
  input_end_tocheck = addr + size;
  input_end = input_end_tocheck;
  if ((input_end_tocheck != (unsigned long long)input_end)) {
    return 1;
  }

  for (i = 0; i < n_pmp; i++) {
    pmp_base = 0;
    pmp_log2size = 0;
    pmp_end2 = 0;
    prot_out = 0;
    pmp_get(i, &prot_out, &pmp_base, &pmp_log2size);
    pmp_end = pmp_base + (1 << pmp_log2size);
    if (i < n_pmp - 1) {
      pmp_get(i + 1, &prot_out, &pmp_end2, &pmp_log2size2);
      if ((prot_out & PMP_A) == PMP_A_TOR) {
        i++;
        pmp_end = pmp_end2;
      }
    }
    if ((pmp_base <= addr) && (pmp_end >= input_end)) {
      slice_printf(
          "Mem region (%lx, %lx) is already covered by pmp_%d (%lx, %lx): "
          "%lx\n",
          addr, input_end, i, pmp_base, pmp_end, prot_out);
      region_overlap = 1;
      break;
    } else if (addr < pmp_end && addr > pmp_base) {
      // Continue check right remaining region
      addr = pmp_base + (1 << pmp_log2size) + 1;
    } else if (input_end < pmp_end && input_end > pmp_base) {
      // Continue check left remaining region
      size = pmp_base - addr;
    } else if (input_end > pmp_end && addr < pmp_base) {
      if (!detect_region_covered_by_pmp(input_end + 1, input_end - pmp_end)) {
        return 0;
      }
      size = pmp_base - addr;
    }
  }
  return region_overlap;
}

static int _pmp_regions() {
  return sbi_hart_pmp_count(sbi_scratch_thishart_ptr());
}

int pmp_set_non_natural_aligned(unsigned int pmp_index, unsigned long prot,
                                unsigned long addr) {
  int pmpcfg_csr, pmpcfg_shift, pmpaddr_csr;
  unsigned long cfgmask, pmpcfg;
  unsigned long pmpaddr;

  /* check parameters */
  if (pmp_index >= _pmp_regions()) return SBI_ERR_SLICE_PMP_FAILURE;

    /* calculate PMP register and offset */
#if __riscv_xlen == 32
  pmpcfg_csr = CSR_PMPCFG0 + (n >> 2);
  pmpcfg_shift = (pmp_index & 3) << 3;
#elif __riscv_xlen == 64
  pmpcfg_csr = (CSR_PMPCFG0 + (pmp_index >> 2)) & ~1;
  pmpcfg_shift = (pmp_index & 7) << 3;
#else
  pmpcfg_csr = -1;
  pmpcfg_shift = -1;
#endif
  pmpaddr_csr = CSR_PMPADDR0 + pmp_index;
  if (pmpcfg_csr < 0 || pmpcfg_shift < 0) return SBI_ERR_SLICE_PMP_FAILURE;

  /* encode PMP config */
  cfgmask = ~(0xffUL << pmpcfg_shift);
  pmpcfg = (csr_read_num(pmpcfg_csr) & cfgmask);
  pmpcfg |= ((prot << pmpcfg_shift) & ~cfgmask);

  /* encode PMP address */
  pmpaddr = (addr >> PMP_SHIFT);

  /* write csrs */
  csr_write_num(pmpaddr_csr, pmpaddr);
  csr_write_num(pmpcfg_csr, pmpcfg);

  return 0;
}

int slice_calculate_pmp_for_mem(unsigned long addr, unsigned long size,
                                bool force_override) {
  int naturally_aligned =
      (size == (1UL << log2roundup(size))) && ((addr & ~(size - 1UL)) == addr);
  if (size < (1 << PMP_SHIFT)) {
    return -1;
  }
  if (detect_region_covered_by_pmp(addr, size) && !force_override) {
    return 0;
  }
  if (size == -1UL) {
    return 1;
  }
  return naturally_aligned ? 1 : 2;
}

int slice_set_pmp_for_mem(unsigned pmp_index, unsigned long prot,
                          unsigned long addr, unsigned long size,
                          bool force_override) {
  unsigned order = log2roundup(size);
  unsigned requested_pmp =
      slice_calculate_pmp_for_mem(addr, size, force_override);
  int err;
  unsigned long exist_prot, exist_addr, exist_order;
  if (pmp_index + requested_pmp > _pmp_regions()) {
    return SBI_ERR_SLICE_PMP_FAILURE;
  }
  if (requested_pmp < 0) {
    slice_printf(
        "failed pmp alloc: %s:hart %d, set %d pmp[%d:%d]: mem(%lx, %lx), "
        "prot=%lx\n",
        __func__, current_hartid(), requested_pmp, pmp_index,
        pmp_index + requested_pmp, addr, size, prot);
    return requested_pmp;
  }
  if (requested_pmp == 0) {
    slice_printf("hart %d: Region (%lx, %lx) is already covered by pmp\n",
                 current_hartid(), addr, size);
  } else if (requested_pmp == 1) {
    slice_printf(
        "pmp alloc: %s:hart %d, set %d pmp[%d:%d]: mem(%lx, %lx), prot=%lx\n",
        __func__, current_hartid(), requested_pmp, pmp_index,
        pmp_index + requested_pmp, addr, size, prot);
    exist_prot = 0;
    pmp_get(pmp_index, &exist_prot, &exist_addr, &exist_order);
    if (SLICE_PMP_L && ((exist_prot & SLICE_PMP_L) == SLICE_PMP_L)) {
      slice_printf(
          "pmp exists: %s:hart %d, set %d pmp[%d:%d]: mem(%lx, 1<<%lx), "
          "prot=%lx\n",
          __func__, current_hartid(), requested_pmp, pmp_index,
          pmp_index + requested_pmp, exist_addr, exist_order, exist_prot);
      return SBI_ERR_SLICE_PMP_FAILURE;
    }
    err = pmp_set(pmp_index, prot, addr, order);
    if (err) {
      return err;
    }
  } else if (requested_pmp == 2) {
    slice_printf(
        "pmp alloc: %s:hart %d, set %d pmp[%d:%d]: mem(%lx, %lx), prot=%lx\n",
        __func__, current_hartid(), requested_pmp, pmp_index,
        pmp_index + requested_pmp, addr, size, prot);

    err = pmp_set_non_natural_aligned(pmp_index, prot, addr);
    if (err) {
      return err;
    }
    prot |= PMP_A_TOR;
    err = pmp_set_non_natural_aligned(pmp_index + 1, prot, addr + size);
    if (err) {
      return err;
    }
  }
  return pmp_index + requested_pmp;
}

void slice_pmp_init() {
  unsigned long init_prot = 0;

  for (unsigned int i = 0; i < _pmp_regions(); i++) {
    pmp_set_non_natural_aligned(i, init_prot, 0);
  }
}

static int slice_setup_pmp_for_ipi(unsigned pmp_index, void *dom_ptr) {
  slice_printf("hart %d: %s\n", current_hartid(), __func__);
  const struct sbi_ipi_device *ipi_dev = sbi_ipi_get_device();
  const struct sbi_domain *dom = (struct sbi_domain *)dom_ptr;
  unsigned hartid = 0, prev_assigned_hartid = 0;
  unsigned long addr = 0, size = 0;
  sbi_hartmask_for_each_hart(hartid, dom->possible_harts) {
    slice_printf("hart %d is assigned to dom %s\n", hartid, dom->name);
    if (!addr) {
      addr = ipi_dev->ipi_addr(hartid);
      size += 4;
    } else if ((hartid - prev_assigned_hartid) == 1) {
      size += 4;
    } else if (size) {
      pmp_index = slice_set_pmp_for_mem(pmp_index, SLICE_PMP_L | PMP_W | PMP_R,
                                        addr, size, false);
      if (pmp_index < 0) {
        return pmp_index;
      }
      size = 0;
      addr = 0;
    }
    prev_assigned_hartid = hartid;
  }
  if (size) {
    pmp_index = slice_set_pmp_for_mem(pmp_index, SLICE_PMP_L | PMP_W | PMP_R,
                                      addr, size, false);
  }
  return pmp_index;
}

#define PMP_PERM_FLAG_NUM 4

void slice_pmp_dump() {
  unsigned pmp_index;
  unsigned long prot, addr, order;
  unsigned long perm_flags[] = {SLICE_PMP_L, PMP_R, PMP_W, PMP_X};
  char perm_str[] = "LRWX";
  char perm[PMP_PERM_FLAG_NUM + 2];
  int npmp = _pmp_regions();
  if (current_hartid() == 0) {
    npmp = 16;
  }
  sbi_printf("%s: hart %d: _pmp_regions() = %d\n", __func__, current_hartid(),
             npmp);
  for (pmp_index = 0; pmp_index < npmp; ++pmp_index) {
    pmp_get(pmp_index, &prot, &addr, &order);
    sbi_memset(perm, '-', 1 + PMP_PERM_FLAG_NUM);
    perm[PMP_PERM_FLAG_NUM] = 0;
    for (size_t i = 0; i < PMP_PERM_FLAG_NUM; ++i) {
      if ((prot & perm_flags[i]) == perm_flags[i]) {
        perm[i] = perm_str[i];
      }
    }
    if ((prot & PMP_A) == PMP_A_TOR) {
      perm[PMP_PERM_FLAG_NUM] = 'T';
    }

    sbi_printf("hart %d: pmp[%d]: %lx-%lx (prot = %lx %s) \n", current_hartid(),
               pmp_index, addr, addr + (1UL << order), prot, perm);
  }
}

int slice_setup_pmp(void *dom_ptr) {
  struct sbi_domain_memregion *reg;
  struct sbi_domain *dom = (struct sbi_domain *)dom_ptr;
  unsigned int pmp_index = 0;
  unsigned long pmp_flags = 0;
  // TODO(ziqiao): Remove unlocked PMP for firmware access.
  // copy scratch->fw_start to domain memory,
  // set up sbi trap handler using the copied handler
  // and lock this before entering S mode;
  slice_pmp_init();

  // Manually added rule to disallow access to original sbi.
  // Each slice only uses its own sbi copy.
  pmp_index = slice_set_pmp_for_mem(pmp_index, SLICE_PMP_L, 0x8000000,
                                    1UL << 24, false);
  if (pmp_index < 0) {
    sbi_hart_hang();
    return pmp_index;
  }

  pmp_index = slice_setup_pmp_for_ipi(pmp_index, dom_ptr);
  // Set up allowed domain memory regions.
  if (pmp_index < 0) {
    sbi_hart_hang();
    return pmp_index;
  }

  slice_printf("dom ptr = %lx\n", (unsigned long)dom);
  sbi_domain_for_each_memregion(dom, reg) {
    slice_printf("%s: hart %d: mem region (%lx, %lx)\n", __func__,
                 current_hartid(), reg->base, reg->order);
    pmp_flags = SLICE_PMP_L;
    if (reg->flags & SBI_DOMAIN_MEMREGION_READABLE) pmp_flags |= PMP_R;
    if (reg->flags & SBI_DOMAIN_MEMREGION_WRITEABLE) pmp_flags |= PMP_W;
    if (reg->flags & SBI_DOMAIN_MEMREGION_EXECUTABLE) pmp_flags |= PMP_X;
    if (pmp_flags == SLICE_PMP_L) {
      continue;
    }
    pmp_index = slice_set_pmp_for_mem(pmp_index, pmp_flags, reg->base,
                                      1UL << reg->order, false);
    if (pmp_index < 0) {
      sbi_hart_hang();
      return pmp_index;
    }
  }
  // Do not allow access to other regions;
  pmp_index = slice_set_pmp_for_mem(pmp_index, SLICE_PMP_L, 0, -1UL, false);
  if (pmp_index < 0) {
    return pmp_index;
  }
  slice_pmp_dump();
  return 0;
}

int nonslice_setup_pmp(void *dom_ptr) {
  slice_printf("__func__=%s\n", __func__);
  int pmp_index = 0;
  pmp_index =
      slice_set_pmp_for_mem(pmp_index, PMP_W | PMP_R | PMP_X, 0, -1UL, false);
  slice_pmp_dump();
  return 0;
}

int emptyslice_setup_pmp(void) {
  slice_printf("%s: hart%d\n", __func__, current_hartid());
  pmp_set(0, PMP_L, 0, __riscv_xlen);
  slice_pmp_dump();
  return 0;
}
