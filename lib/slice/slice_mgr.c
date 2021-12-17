#include <sbi/sbi_bitops.h>
#include <sbi/sbi_console.h>
#include <sbi/sbi_domain.h>
#include <sbi/sbi_ecall_interface.h>
#include <sbi/sbi_hart.h>
#include <sbi/sbi_hsm.h>
#include <sbi/sbi_ipi.h>
#include <sbi/sbi_platform.h>
#include <sbi/sbi_scratch.h>
#include <sbi/sbi_string.h>
#include <sbi/sbi_trap.h>
#include <slice/slice.h>
#include <slice/slice_err.h>
#include <slice/slice_mgr.h>
#include <slice/slice_pmp.h>
#include <slice/slice_reset.h>

struct sbi_ipi_data {
  unsigned long ipi_type;
  struct SliceIPIData slice_data[MAX_HART_NUM];
};

struct sbi_ipi_data *slice_ipi_data_ptr(u32 hartid) {
  /*const struct sbi_domain *dom = sbi_hartid_to_domain(hartid);
  // TBD: Use a special memory that is accessible by slice-0 and this slice.
  // This special memory is located in a slice-0 memory and is shared with
  // slice-k by adding whitelist PMP rule when slice-k starts
  if (dom == NULL) {
    sbi_printf("%s: NULL dom\n", __func__);
    sbi_hart_hang();
  }*/
  /*
  unsigned long ptr = (unsigned long)dom->slice_mem_start +
                      SLICE_IPI_DATA_OFFSET +
                      sizeof(struct sbi_ipi_data) * hartid;
  return (struct sbi_ipi_data *)ptr;
  */
  struct slice_bus_data *bus = (struct slice_bus_data *)SLICE0_BUS_MEMORY;
  return (struct sbi_ipi_data *)&bus->slice_buses[hartid];
}

struct SliceIPIData *slice_ipi_slice_data(u32 src, u32 dst) {
  return &slice_ipi_data_ptr(dst)->slice_data[src];
}

static void __attribute__((noreturn))
slice_jump_to_sw_reset(unsigned long next_addr) {
#if __riscv_xlen == 32
  unsigned long val, valH;
#else
  unsigned long val;
#endif
  slice_printf("%s(): next_addr=%#lx\n", __func__, next_addr);
  val = csr_read(CSR_MSTATUS);
  val = INSERT_FIELD(val, MSTATUS_MPP, PRV_M);
  val = INSERT_FIELD(val, MSTATUS_MPIE, 0);
  // Disable all interrupts;
  csr_write(CSR_MIE, 0);
  csr_write(CSR_MSTATUS, val);
  csr_write(CSR_MEPC, next_addr);

  sbi_hsm_hart_stop(sbi_scratch_thishart_ptr(), false);
  register unsigned long a0 asm("a0") = 0;
  register unsigned long a1 asm("a1") = 0;
  __asm__ __volatile__(
     "jr %0\n" 
     :: "r"(next_addr), "r"(a0), "r"(a1)
     );
  __builtin_unreachable();
}

static void sbi_ipi_process_slice_op(struct sbi_scratch *scratch) {
  slice_printf("%s\n", __func__);
  unsigned int dst_hart = current_hartid();
  for (size_t src_hart = 0; src_hart < sbi_scratch_last_hartid(); ++src_hart) {
    switch (slice_ipi_slice_data(src_hart, dst_hart)->func_id) {
      case SLICE_IPI_SW_STOP:
        slice_ipi_slice_data(src_hart, dst_hart)->func_id = SLICE_IPI_NONE;
        slice_printf("%s: hart %d\n", __func__, dst_hart);
        slice_pmp_init();
        slice_pmp_dump();
        slice_jump_to_sw_reset(0x8000000);
        break;
      // DEBUG-purpose
      case SLICE_IPI_PMP_DEBUG:
        slice_ipi_slice_data(src_hart, dst_hart)->func_id = SLICE_IPI_NONE;
        slice_printf("%s: hart %d\n", __func__, dst_hart);
        slice_pmp_dump();
        break;
      case SLICE_IPI_NONE:
      default:
        break;
    }
  }
}

struct sbi_ipi_event_ops ipi_slice_op = {
    .name = "IPI_SLICE",
    .process = sbi_ipi_process_slice_op,
};

static u32 ipi_slice_event = SBI_IPI_EVENT_MAX;

void slice_ipi_register() {
  slice_printf("%s: hart%d\n", __func__, current_hartid());
  int ret = sbi_ipi_event_create(&ipi_slice_op);
  if (ret < 0) {
    sbi_hart_hang();
  }
  ipi_slice_event = ret;
}

static int slice_send_ipi_to_domain(struct sbi_domain *dom,
                                    enum SliceIPIFuncId func_id) {
  unsigned int dst_hart, src_hart = current_hartid();
  unsigned long hart_mask = 0;
  sbi_hartmask_for_each_hart(dst_hart, &dom->assigned_harts) {
    slice_ipi_slice_data(src_hart, dst_hart)->func_id = func_id;
    hart_mask |= (1 << dst_hart);
  }
  sbi_ipi_send_many(hart_mask, 0, ipi_slice_event, NULL);
  slice_printf("%s: hart%d: sbi_ipi_send_many hart_mark=%lx\n", __func__,
               current_hartid(), hart_mask);
  return 0;
}

int slice_create(struct sbi_hartmask cpu_mask, unsigned long mem_start,
                 unsigned long mem_size, unsigned long image_from,
                 unsigned long image_size, unsigned long fdt_from,
                 unsigned long mode) {
  struct slice_options options = {cpu_mask,   mem_start, mem_size, image_from,
                                  image_size, fdt_from,  mode,     ""};
  sbi_memset(options.stdout, 0, SLICE_UART_PATH_LEN);
  sbi_printf("%s:%lx\n", __func__, options.mem_start);
  return slice_create_full(&options);
}

int slice_create_full(struct slice_options *slice_options) {
  struct sbi_domain *dom;
  int err = 0;
  unsigned cpuid = 0, boot_hartid = -1;
  const struct sbi_platform *plat =
      sbi_platform_ptr(sbi_scratch_thishart_ptr());
  sbi_hartmask_for_each_hart(cpuid, &slice_options->hartmask) {
    boot_hartid = cpuid;
    break;
  }
  dom = (struct sbi_domain *)slice_allocate_domain(&slice_options->hartmask);
  if (sbi_strlen(slice_options->stdout) > 0) {
    sbi_memset(dom->stdout_path, 0, array_size(dom->stdout_path));
    sbi_memcpy(dom->stdout_path, slice_options->stdout,
               sbi_strlen(slice_options->stdout));
  }
  dom->boot_hartid = boot_hartid;
  sbi_printf("%s:%lx\n", __func__, slice_options->mem_start);

  dom->slice_mem_start = slice_options->mem_start;
  dom->slice_mem_size = slice_options->mem_size;
  if(dom->slice_mem_size == -1UL){
    sbi_printf("%s:dom->slice_mem_size == -1UL\n", __func__);
    dom->slice_type = SLICE_TYPE_STANDARD_DOMAIN;
  }
  dom->next_addr = (dom->slice_type == SLICE_TYPE_SLICE) ? (slice_options->mem_start + SLICE_OS_OFFSET) : slice_options->image_from;
  dom->next_mode = slice_options->guest_mode;
  sbi_printf("%s: slice_options->stdout=%s\n", __func__, slice_options->stdout);
  dom->next_boot_src = slice_options->image_from;
  dom->next_boot_size = slice_options->image_size;
  dom->slice_dt_src = (void *)slice_options->fdt_from;
  dom->next_arg1 = (dom->slice_type == SLICE_TYPE_SLICE) ? (dom->next_addr + dom->next_boot_size + 0x1000000) : slice_options->fdt_from;
  dom->system_reset_allowed = false;
  if (sbi_platform_ops(plat)->slice_init_mem_region) {
    sbi_platform_ops(plat)->slice_init_mem_region(dom);
  }
 // dump_slice_config(dom);
  err = sanitize_slice(dom);
  if (err) {
    sbi_printf("%s: Cannot create an slice. err = %d\n", __func__, err);
    dump_slice_config(dom);
    return err;
  }
  err = sbi_domain_register(dom, dom->possible_harts);
  if (err) {
    return err;
  }
  if (current_hartid() == slice_host_hartid()) {
    if (sbi_platform_ops(plat)->slice_register_hart &&
        sbi_platform_ops(plat)->slice_register_source) {
      sbi_platform_ops(plat)->slice_register_hart(
          (char*)"", slice_options->hartmask.bits[0], boot_hartid,
          slice_options->guest_mode, dom->slice_mem_start, dom->slice_mem_size);
      sbi_platform_ops(plat)->slice_register_source(
          boot_hartid, slice_options->image_from, slice_options->image_size,
          slice_options->fdt_from, dom->stdout_path);
    }
  }
  return slice_activate(dom);
}

int slice_unregister(struct sbi_domain *dom) {
  const struct sbi_platform *plat =
      sbi_platform_ptr(sbi_scratch_thishart_ptr());
  sbi_printf("%s: plat\n", __func__);
  int ret = 0, hartid;
  if(!sbi_platform_ops(plat)->slice_unregister_hart){
    sbi_printf("%s: unimplemented slice_unregister_hart\n", __func__);
    return SBI_ERR_SLICE_NOT_IMPLEMENTED;
  }
  sbi_hartmask_for_each_hart(hartid, &dom->assigned_harts) {
    sbi_platform_ops(plat)->slice_unregister_hart(hartid);
  }
  return ret;
}

int slice_delete(int dom_index) {
  // Only slice_host_hartid is able to delete a slice config.
  if (current_hartid() != slice_host_hartid()) {
    // should never reach here.
    return SBI_ERR_SLICE_SBI_PROHIBITED;
  }
  sbi_printf("%s: deleting slice %d in progress- get dom.\n",
      __func__, dom_index);
  struct sbi_domain *dom = sbi_index_to_domain(dom_index);
  sbi_printf("%s: deleting slice %d in progress- freeze.\n",
      __func__, dom_index);
  slice_freeze(dom);
  sbi_printf("%s: deleting slice %d in progress- unregister.\n",
      __func__, dom_index);
  slice_unregister(dom);
  sbi_printf(
      "%s: deleting slice %d in progress. Need a reset to completely free its "
      "resource.",
      __func__, dom_index);
  return 0;
}

static void slice_remove_if_frozen(struct sbi_domain *dom) {
  struct sbi_domain_memregion *region;
  unsigned index = dom->index;
  if (!slice_deactivate(dom)) {
    // Start to delete the slice.
    for (unsigned int hart_id = 0; hart_id < MAX_HART_NUM; ++hart_id) {
      if (hartid_to_domain_table[hart_id] == dom) {
        hartid_to_domain_table[hart_id] = &root;
        // sbi_hartmask_set_hart(hart_id, &free_harts);
      }
    }
    sbi_domain_for_each_memregion(dom, region) {
      sbi_memset(region, 0, sizeof(*region));
    }
    sbi_memset(dom, 0, sizeof(*dom));
    sbi_printf("%s: slice %d is deleted.", __func__, index);
    // Move free harts to root domain (slice-host);
    // sbi_hartmask_or(&root.assigned_harts, &root.assigned_harts,
    //		&free_harts);
  }
}
int slice_stop(int dom_index) {
  if (current_hartid() != slice_host_hartid()) {
    // should never reach here.
    return SBI_ERR_SLICE_SBI_PROHIBITED;
  }
  struct sbi_domain *dom = slice_from_index(dom_index);
  if (!dom) {
    return SBI_ERR_SLICE_ILLEGAL_ARGUMENT;
  }
  int err = slice_send_ipi_to_domain(dom, SLICE_IPI_SW_STOP);
  if (err) {
    return err;
  }
  slice_status_stop(dom);
  // Now try to free this slice resource if the dom is frozen by slice_delete.
  slice_remove_if_frozen(dom);
  return 0;
}

int slice_hw_reset(int dom_index) {
  struct sbi_domain *dom = slice_from_index(dom_index);
  if (!dom) {
    return SBI_ERR_SLICE_ILLEGAL_ARGUMENT;
  }
  slice_status_stop(dom);
  d_reset_by_hartmask(*dom->possible_harts->bits);
  slice_remove_if_frozen(dom);
  return 0;
}

void slice_ipi_test(int dom_index){
  slice_send_ipi_to_domain(slice_from_index(dom_index), SLICE_IPI_NONE);
}

void slice_pmp_dump_by_index(int dom_index) {
  if (dom_index >= 0) {
    slice_send_ipi_to_domain(slice_from_index(dom_index), SLICE_IPI_PMP_DEBUG);
  } else {
    slice_pmp_dump();
  }
}

void dump_slices_config(void) {
  struct sbi_domain *dom;
  for (size_t i = 0; i < SBI_DOMAIN_MAX_INDEX; ++i) {
    if ((dom = slice_from_index(i))) {
      dump_slice_config(dom);
      sbi_printf("\n");
    }
  }
}
