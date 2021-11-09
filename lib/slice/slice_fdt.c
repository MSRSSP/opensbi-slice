#include <libfdt.h>
#include <sbi/sbi_console.h>
#include <sbi/sbi_domain.h>
#include <sbi/sbi_hart.h>
#include <sbi/sbi_math.h>
#include <sbi/sbi_platform.h>
#include <sbi/sbi_scratch.h>
#include <sbi/sbi_string.h>
#include <sbi_utils/fdt/fdt_domain.h>
#include <sbi_utils/fdt/fdt_fixup.h>
#include <sbi_utils/fdt/fdt_helper.h>
#include <sbi_utils/reset/fdt_reset.h>
#include <sbi_utils/serial/fdt_serial.h>
#include <slice/slice.h>
#include <slice/slice_err.h>

void _print_fdt(const void *fdt, int node, char *prefix) {
  int property, child, fixup_len, plen, len;
  const fdt32_t *fixup_val;
  const char *name, *str_val;
  fdt_for_each_property_offset(property, fdt, node) {
    fixup_val = fdt_getprop_by_offset(fdt, property, &name, &fixup_len);
    str_val = (const char *)fixup_val;
    sbi_printf("%s%s = ", prefix, name);
    if (sbi_isprintable(str_val[0])) {
      sbi_printf("%s\n", str_val);
    } else {
      for (size_t i = 0; i < fixup_len / 4; ++i) {
        sbi_printf("%x ", fdt32_to_cpu(fixup_val[i]));
      }
      sbi_printf("\n");
    }
  }
  plen = strlen(prefix);
  fdt_for_each_subnode(child, fdt, node) {
    sbi_printf("%s%s{\n", prefix, fdt_get_name(fdt, child, &len));
    prefix[plen] = '\t';
    _print_fdt(fdt, child, prefix);
    prefix[plen] = 0;
    sbi_printf("%s}\n", prefix);
  }
}

void slice_print_fdt(const void *fdt) {
  int root_fdt = fdt_path_offset(fdt, "/");
  char prefix[64] = "";
  _print_fdt(fdt, root_fdt, prefix);
}

void copy_fdt(const void *src_fdt, void *dst_fdt) {
  unsigned long startTicks = csr_read(CSR_MCYCLE);
  if (dst_fdt && (fdt_totalsize(src_fdt) != 0)) {
    slice_printf("duplicate %lx -> %lx\n", (unsigned long)src_fdt,
                 (unsigned long)dst_fdt);
    sbi_memcpy(dst_fdt, src_fdt, fdt_totalsize(src_fdt));
  }
  sbi_printf("%s: hart %d: #ticks = %lu\n", __func__, current_hartid(),
             csr_read(CSR_MCYCLE) - startTicks);
}

static const struct fdt_match riscv_cpu_match[] = {
    {.compatible = "riscv"},
};

int slice_remove_useless_cpus(void *fdt, const void *dom_ptr) {
  int noff = 0, prevnoff = 0, err;
  u32 hartid;
  const struct fdt_match *match;
  while (true) {
    prevnoff = noff;
    noff = fdt_find_match(fdt, noff, riscv_cpu_match, &match);
    if (noff < 0) break;
    err = fdt_parse_hart_id(fdt, noff, &hartid);
    if (err) {
      slice_printf("err hart %d\n", hartid);
    }
    slice_printf("Check hart %d in dom %lx\n", hartid, (unsigned long)dom_ptr);

    if (hartid != 0 && sbi_hartid_to_domain(hartid) != dom_ptr) {
      slice_printf("remove hart %d\n", hartid);
      fdt_nop_node(fdt, noff);
      noff = prevnoff;
    }
  }
  return 0;
}

bool slice_is_accessible(unsigned long addr, unsigned long size,
                         const void *dom_ptr) {
  struct sbi_domain_memregion *reg;
  const struct sbi_domain *dom = dom_ptr;
  sbi_domain_for_each_memregion(dom, reg) {
    /* Ignore MMIO or READABLE or WRITABLE or EXECUTABLE regions */

    // remove the unit as it is not accessible
    if (addr >= reg->base && (addr + size) <= (reg->base + (1 << reg->order))) {
      if (reg->flags & SBI_DOMAIN_MEMREGION_READABLE) return true;
      if (reg->flags & SBI_DOMAIN_MEMREGION_WRITEABLE) return true;
      if (reg->flags & SBI_DOMAIN_MEMREGION_EXECUTABLE) return true;
    }
  }
  return false;
}

extern struct fdt_serial fdt_serial_uart8250;
extern struct fdt_serial fdt_serial_sifive;
extern struct fdt_serial fdt_serial_htif;
extern struct fdt_serial fdt_serial_shakti;
extern struct fdt_serial fdt_serial_gaisler;

static struct fdt_serial *serial_drivers[] = {
    &fdt_serial_uart8250, &fdt_serial_sifive, &fdt_serial_htif,
    &fdt_serial_shakti, &fdt_serial_gaisler};

static void slice_serial_fixup(void *fdt, const struct sbi_domain *dom) {
  /* Fix up UART*/
  const struct fdt_match *match;
  struct fdt_serial *drv;
  int pos, selected_noff = -1, noff = -1;
  if (sbi_strlen(dom->stdout_path)) {
    const char *sep, *start = dom->stdout_path;
    /* The device path may be followed by ':' */
    sep = strchr(start, ':');
    if (sep)
      selected_noff = fdt_path_offset_namelen(fdt, start, sep - start);
    else
      selected_noff = fdt_path_offset(fdt, start);
  } else {
    // Do not update serial entries in this device tree if no domain.
    return;
  }
  sbi_printf("%s: selected_noff=%d\n", __func__, selected_noff);
  for (pos = 0; pos < array_size(serial_drivers); pos++) {
    drv = serial_drivers[pos];
    noff = 0;
    while (true) {
      noff = fdt_find_match(fdt, noff, drv->match_table, &match);
      if (noff < 0) break;
      sbi_printf("%s: uart node=%d\n", __func__, noff);
      int diff = selected_noff - noff;
      if (diff < 10 && diff > -10) {
        fdt_setprop_string(fdt, noff, "status", "okay");
      } else {
        fdt_setprop_string(fdt, noff, "status", "disabled");
      }
    }
  }
}

/* Fix fdt's uart according to dom_ptr.

1. Update stdout-path to domain specific one
2. Remove uarts outside of the domain.
If not invalidating uarts outside of the domain,
kernel would panic due to serial_probe (e.g., sifive_serial_probe).
*/
void fdt_serial_fixup(void *fdt, const void *dom_ptr) {
  /* Fix up UART*/
  const struct sbi_domain *dom = dom_ptr;
  int coff;
  coff = fdt_path_offset(fdt, "/chosen");
  if (coff < 0) return;
  if (sbi_strlen(dom->stdout_path)) {
    // Set the chosen stdout path to the dom's stdout path.
    fdt_setprop_string(fdt, coff, "stdout-path", dom->stdout_path);
    sbi_printf("hart%d: Fix UART: %s\n", current_hartid(), dom->stdout_path);
    slice_serial_fixup(fdt, dom);
  }
}

static const struct fdt_match fixup_device_match_table[] = {
    {.compatible = "syscon-reboot"},
    {},
};

int fdt_reset_device_fixup(void *fdt, const void *dom_ptr) {
  int len = -1;
  int node = 0, hartid;
  const struct fdt_match *match;
  const u32 *val;
  int to_remove[64];
  int n_remove = 0;
  for (int i = 0; i < sbi_scratch_last_hartid(); ++i) {
    slice_printf("fdt=%lx node=%d fixup_device_match_table=%lx\n",
                 (unsigned long)fdt, node,
                 (unsigned long)fixup_device_match_table);
    node = fdt_find_match(fdt, node, fixup_device_match_table, &match);
    slice_printf("fdt_find_match=%d\n", node);
    if (node < 0) {
      break;
    }
    val = fdt_getprop(fdt, node, "cpu", &len);
    if (val == NULL) {
      continue;
    }
    hartid = fdt32_to_cpu(*val);
    if (sbi_hartid_to_domain(hartid) == dom_ptr) {
      continue;
    }
    to_remove[n_remove] = node;
    n_remove++;
  }
  for (; n_remove > 0; n_remove--) {
    fdt_nop_node(fdt, to_remove[n_remove - 1]);
  }
  slice_printf("end node=%d\n", node);
  return 0;
}

int slice_fixup_memory(void *fdt, const struct sbi_domain *dom) {
  int soc_offset = fdt_path_offset(fdt, "/soc");
  if (soc_offset < 0) {
    sbi_printf("%s: soc_offset<0\n", __func__);
    return SBI_ERR_SLICE_ILLEGAL_ARGUMENT;
  }
  int len, err;
  const char *device_type;
  fdt32_t reg[4];
  fdt32_t *val;
  val = reg;
  *val++ = cpu_to_fdt32((u64)dom->slice_mem_start >> 32);
  *val++ = cpu_to_fdt32(dom->slice_mem_start);
  *val++ = cpu_to_fdt32((u64)dom->slice_mem_size >> 32);
  *val++ = cpu_to_fdt32(dom->slice_mem_size);
  fdt_for_each_subnode(soc_offset, fdt, soc_offset) {
    device_type =
        (const char *)fdt_getprop(fdt, soc_offset, "device_type", &len);
    if (!device_type || !len) {
      continue;
    }
    if (strcmp(device_type, "memory") == 0) {
      err = fdt_setprop(fdt, soc_offset, "reg", reg, 4 * sizeof(fdt32_t));
      slice_printf("%s: Fix up memory\n", __func__);
      if (err) {
        return SBI_ERR_SLICE_ILLEGAL_ARGUMENT;
      }
    }
  }
  return 0;
}

int slice_create_domain_fdt(const void *dom_ptr) {
  const struct sbi_domain *domain = (const struct sbi_domain *)dom_ptr;
  void *fdt_src, *fdt;
  if (domain->boot_hartid != current_hartid()) {
    return 0;
  }
  fdt = (void *)domain->next_arg1;
  if (fdt == NULL) {
    return 0;
  }
  slice_printf("Hart-%d: %s: fdt=%lx\n", current_hartid(), __func__,
               domain->next_arg1);
  // TODO: reset domain memory;
  if (domain->slice_dt_src) {
    fdt_src = domain->slice_dt_src;
  } else {
    fdt_src = (void *)root.next_arg1;
  }
  copy_fdt(fdt_src, fdt);
  fdt_cpu_fixup(fdt, dom_ptr);
  slice_fixup_memory(fdt, domain);
  // Cannot remove cpu0;
  // If exposing only cpu0, cpu3, cpu4, kernel would panic
  fdt_reset_device_fixup(fdt, dom_ptr);
  fdt_serial_fixup(fdt, dom_ptr);
  // slice_serial_fixup(fdt, domain);
  fdt_fixups(fdt, dom_ptr);
  fdt_domain_fixup(fdt, dom_ptr);
  slice_print_fdt(fdt);
  return 0;
}
