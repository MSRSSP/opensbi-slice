#include <sbi/sbi_domain.h>
#include <sbi/sbi_scratch.h>
#include <sbi/sbi_string.h>
#include <sbi_utils/fdt/fdt_domain.h>
#include <slice/slice.h>
#include <slice/slice_err.h>

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

static bool slice_mem_overlap(struct sbi_domain *dom1,
                              struct sbi_domain *dom2) {
  if (dom1->slice_mem_start > dom2->slice_mem_start) {
    return slice_mem_overlap(dom2, dom1);
  }
  return (dom1->slice_mem_start + dom1->slice_mem_size) > dom2->slice_mem_start;
}

static bool slice_cpu_overlap(struct sbi_domain *dom1,
                              struct sbi_domain *dom2) {
  unsigned hartid;
  sbi_hartmask_for_each_hart(hartid, dom1->possible_harts) {
    if (sbi_hartmask_test_hart(hartid, dom2->possible_harts)) {
      return true;
    }
  }
  return false;
}

int sanitize_slice(struct sbi_domain *new_dom) {
  struct sbi_domain *dom;
  for (size_t i = 1; i < SBI_DOMAIN_MAX_INDEX; ++i) {
    if ((dom = slice_from_index(i))) {
      if (slice_mem_overlap(new_dom, dom)) {
        dump_slice_config(dom);
        return SBI_ERR_SLICE_REGION_OVERLAPS;
      }
      if (slice_cpu_overlap(new_dom, dom)) {
        dump_slice_config(dom);
        return SBI_ERR_SLICE_REGION_OVERLAPS;
      }
    }
  }
  if (new_dom->boot_hartid <= 0 ||
      new_dom->boot_hartid > last_hartid_having_scratch) {
    return SBI_ERR_SLICE_ILLEGAL_ARGUMENT;
  }
  if (new_dom->slice_mem_size == -1UL) {
    new_dom->slice_type = SLICE_TYPE_STANDARD_DOMAIN;
  }
  return 0;
}

void dump_slice_config(const struct sbi_domain *dom) {
  const char *slice_status_str[3] = {"INACTIVE/DELETED", "ACTIVE", "FROZEN"};
  struct sbi_domain *d = (struct sbi_domain *)dom;
  int status_code = smp_read(&d->slice_status);
  slice_log_printf(SLICE_LOG_INFO, "slice %d: slice_type        = %d\n",
                   dom->index, dom->slice_type);
  slice_log_printf(SLICE_LOG_INFO, "slice %d: slice_status      = %s\n",
                   dom->index, slice_status_str[status_code]);
  slice_log_printf(SLICE_LOG_INFO, "slice %d: Boot HART         = %d\n",
                   dom->index, dom->boot_hartid);
  unsigned k = 0, i;
  slice_log_printf(SLICE_LOG_INFO,
                   "slice %d: HARTs             = ", dom->index);
  sbi_hartmask_for_each_hart(i, dom->possible_harts)
      slice_log_printf(SLICE_LOG_INFO, "%s%d%s", (k++) ? "," : "", i,
                       sbi_domain_is_assigned_hart(dom, i) ? "*" : "");

  slice_log_printf(SLICE_LOG_INFO, "\n");
  slice_log_printf(SLICE_LOG_INFO, "slice %d: slice_mem_start   = 0x%lx\n",
                   dom->index, dom->slice_mem_start);
  slice_log_printf(SLICE_LOG_INFO, "slice %d: slice_mem_size    = %ld MiB\n",
                   dom->index, (u64)dom->slice_mem_size >> 20);
  slice_log_printf(SLICE_LOG_INFO, "slice %d: slice_fw_start    = 0x%lx\n",
                   dom->index, dom->slice_sbi_start);
  slice_log_printf(SLICE_LOG_INFO, "slice %d: slice_fw_size     = 0x%lx\n",
                   dom->index, dom->slice_sbi_size);
  slice_log_printf(SLICE_LOG_INFO,
                   "slice %d: guest_kernel_src  = 0x%lx (loaded by slice-0)\n",
                   dom->index, dom->next_boot_src);
  slice_log_printf(SLICE_LOG_INFO, "slice %d: guest_kernel_size = 0x%lx \n",
                   dom->index, (unsigned long)dom->next_boot_size);
  slice_log_printf(
      SLICE_LOG_INFO,
      "slice %d: guest_kernel_start= 0x%lx (copy from guest_kernel_src)\n",
      dom->index, dom->next_addr);
  slice_log_printf(SLICE_LOG_INFO,
                   "slice %d: guest_fdt_src     = 0x%lx (loaded by slice-0)\n",
                   dom->index, (unsigned long)dom->slice_dt_src);
  slice_log_printf(
      SLICE_LOG_INFO,
      "slice %d: slice_fdt_start   = 0x%lx (copy from guest_fdt_src)\n",
      dom->index, dom->next_arg1);
  slice_log_printf(SLICE_LOG_INFO, "slice %d: slice_uart        = %s\n",
                   dom->index, dom->stdout_path);
  sbi_hartmask_for_each_hart(i, dom->possible_harts)
      slice_log_printf(SLICE_LOG_INFO, "slice %d: slice_start_time =%lu\n",
                       dom->index, dom->slice_start_time[i]);
}