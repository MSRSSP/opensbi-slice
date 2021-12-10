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

static void __attribute__((noreturn))
slice_jump_to_fw(unsigned long next_addr, unsigned long next_mode,
                 unsigned long arg0, unsigned long arg1, unsigned long arg2) {
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
  register unsigned long a0 asm("a0") = arg0;
  register unsigned long a1 asm("a1") = arg1;
  register unsigned long a2 asm("a2") = arg2;
  __asm__ __volatile__("mret" : : "r"(a0), "r"(a1), "r"(a2));
  __builtin_unreachable();
}

static void zero_slice_memory(void *dom_ptr) {
  unsigned long startTicks = csr_read(CSR_MCYCLE);
  struct sbi_domain *dom = (struct sbi_domain *)dom_ptr;
  if (dom->slice_mem_start) {
    slice_printf("Zero Slice Mem %lx\n", dom->slice_mem_start);
    slice_printf("Zero Slice Mem size %lx.\n", dom->slice_mem_size);
    // sbi_memset((void *)dom->slice_mem_start, 0, dom->slice_mem_size);
  }
  sbi_printf("%s: hart %d: #ticks = %lu\n", __func__, current_hartid(),
             csr_read(CSR_MCYCLE) - startTicks);
}

static struct slice_fw_info fw_info[MAX_HART_NUM];
void slice_loader(struct sbi_domain *dom, unsigned long fw_src,
                  unsigned long fw_size) {
  // Slice-wide initialization: loading firmware into slice MEM.
  unsigned long slice_fw_start = dom->slice_mem_start;
  unsigned hartid = current_hartid();
  if (!is_slice(dom)) {
    // bare metal boot, directlt call sbi_init.
    sbi_init(sbi_scratch_thishart_ptr());
    return;
  }
  if (slice_is_stopped(dom) && dom->boot_hartid == hartid) {
    zero_slice_memory(dom);
    if (!fw_size) {
      sbi_hart_hang();
    }
    slice_printf("relocate_sbi to %lx from %lx.\n", slice_fw_start, fw_src);
    sbi_memcpy((void *)slice_fw_start, (void *)fw_src, fw_size);
    sbi_printf("dom->boothartid=%d", dom->boot_hartid);

    slice_status_start(dom);
  }
  fw_info[hartid].slice_cfg_ptr = (unsigned long)dom;
  fw_info[hartid].sbi_fw_info.magic = SLICE_FW_INFO_MAGIC_VALUE;
  fw_info[hartid].sbi_fw_info.version = SLICE_FW_INFO_VERSION_MAX;
  fw_info[hartid].sbi_fw_info.next_addr = dom->next_addr;
  fw_info[hartid].sbi_fw_info.next_mode = dom->next_mode;
  fw_info[hartid].sbi_fw_info.boot_hart = dom->boot_hartid;
  while (!slice_is_running(dom)) {
  };
  slice_jump_to_fw(slice_fw_start, PRV_M, hartid, (unsigned long)slice_fdt(dom),
                   (unsigned long)&fw_info[hartid]);
}