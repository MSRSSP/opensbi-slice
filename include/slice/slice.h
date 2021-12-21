#ifndef __SLICE_H__
#define __SLICE_H__

#include <sbi/riscv_atomic.h>
#include <slice/slice_log.h>

/*Domain configurations stored in a protected memory region*/
struct sbi_domain* allocate_domain();
struct sbi_hartmask* allocate_hartmask();
struct sbi_domain_memregion* allocate_memregion();
void inc_domain_counter();
unsigned read_domain_counter();

enum slice_type {
  SLICE_TYPE_STANDARD_DOMAIN,
  SLICE_TYPE_SLICE,
};

enum slice_status {
  SLICE_STATUS_DELETED,
  SLICE_STATUS_RUNNING,
  SLICE_STATUS_STOPPED,
  SLICE_STATUS_FROZEN,
};

/* Allocate a domain config*/
void* slice_allocate_domain();

/* Create domain specific device tree.

Copy the root device tree to dom->next_arg1;
Remove unused devices and disable unused CPUs;
*/
int slice_create_domain_fdt(const struct sbi_domain* dom_ptr);

/* Print domain fdt information.*/
void slice_print_fdt(const void* fdt);

/* Copy fdt from dom->fdt_src to dom->next_addr */
void slice_copy_fdt(const struct sbi_domain *dom);

/* Return slice boot hart if this hart belongs to a slice.*/
int slice_boot_hart(void);

/* Prepare domain memory region.
  1. Erase memory content;
  2. Load next-stage code (next-stage bootloader, hypervisor, or kernel)
  from src to dst, as a simplified process loading image from disk/network.
*/
int slice_setup_domain(struct sbi_domain* dom_ptr);

void dump_slice_config(const struct sbi_domain* dom_ptr);

#define SLICE_UART_PATH_LEN 32
#define MAX_HART_NUM 8
struct slice_config {
  enum slice_type slice_type;
#ifndef CONFIG_QEMU
  long slice_status;
#else
  atomic_t slice_status;
#endif
  // source address of device tree for this slice;
  // in a protected memory region; The region could be
  // 1. Only writable by host and this slice;
  // 2. Writable by host and only readable by sliced guests.
  void* slice_dt_src;
  // source address of the next boot image;
  // in host protected memory;
  // copy context from next_boot_src to next_addr;
  unsigned long next_boot_src;
  // size of next boot image;
  unsigned next_boot_size;
  /** default stdio **/
  char stdout_path[SLICE_UART_PATH_LEN];
  unsigned long slice_mem_start;
  unsigned long slice_mem_size;
  unsigned long slice_sbi_start;
  unsigned long slice_sbi_size;
  unsigned long slice_start_time[MAX_HART_NUM];
};

#define report_time(end_time, label) {\
  sbi_printf("%s: %s: hart %d: #ticks : %lu\n",\
  __func__, label, current_hartid(), end_time - root.slice_start_time[current_hartid()]);}

#define report_duration(dur, label) {\
  sbi_printf("%s: %s: hart %d: #ticks : %lu\n",\
  __func__, label, current_hartid(), dur);}

#define slice_fdt(dom) ((void*)((struct sbi_domain*)dom)->next_arg1)

bool is_slice(const struct sbi_domain* dom);

unsigned int slice_host_hartid();

int register_host_hartid(unsigned int hartid);

/* Helper functions for slice status.*/
int slice_activate(struct sbi_domain* dom);
int slice_freeze(struct sbi_domain* dom);
int slice_deactivate(struct sbi_domain* dom);
int slice_status_stop(struct sbi_domain* dom);
int slice_status_start(struct sbi_domain* dom);
int slice_is_active(struct sbi_domain* dom);
int slice_is_stopped(struct sbi_domain *dom);
int slice_is_existed(struct sbi_domain* dom);
int slice_is_running(struct sbi_domain *dom);

struct sbi_domain* slice_from_index(unsigned int index);
struct sbi_domain* active_slice_from_index(unsigned int index);
void nonslice_sbi_init(void);
void slice_loader(struct sbi_domain *dom, unsigned long fw_src, unsigned long fw_size);
// A security-critical function to check overlaps among slices.
int sanitize_slice(struct sbi_domain* new_dom);

long smp_read(long *data);
long smp_exchange(long *data, long current_val, long target_val);
#endif  // __SLICE_H__
