#ifndef __SLICE_H__
#define __SLICE_H__

#include <sbi/riscv_atomic.h>
#include <slice/slice_log.h>
#include <slice/slice_state.h>
#include <slice/slice_fdt.h>

enum slice_type {
  SLICE_TYPE_STANDARD_DOMAIN,
  SLICE_TYPE_SLICE,
};

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

/*Domain configurations stored in a protected memory region*/
struct sbi_domain* allocate_domain();
struct sbi_hartmask* allocate_hartmask();
struct sbi_domain_memregion* allocate_memregion();
void inc_domain_counter();
unsigned read_domain_counter();

/* Allocate a domain config*/
void* slice_allocate_domain();

/* Return slice boot hart if this hart belongs to a slice.*/
int slice_boot_hart(void);

/* Prepare domain memory region.
  1. Erase memory content;
  2. Load next-stage code (next-stage bootloader, hypervisor, or kernel)
  from src to dst, as a simplified process loading image from disk/network.
*/
int slice_setup_domain(struct sbi_domain* dom_ptr);

// A security-critical function to check overlaps among slices.
int sanitize_slice(struct sbi_domain* new_dom);

/*Dump slice configuration.*/
void dump_slice_config(const struct sbi_domain* dom_ptr);

bool is_slice(const struct sbi_domain* dom);

struct sbi_domain* slice_from_index(unsigned int index);
struct sbi_domain* active_slice_from_index(unsigned int index);

/*Entry of slice loader.*/
void slice_loader(struct sbi_domain *dom, unsigned long fw_src, unsigned long fw_size);

/*Read/write smp data*/
long smp_read(long *data);
long smp_exchange(long *data, long current_val, long target_val);

#define report_time(end_time, label) {\
  sbi_printf("%s: %s: hart %d: #ticks : %lu\n",\
  __func__, label, current_hartid(), end_time - root.slice_start_time[current_hartid()]);}

#define report_duration(dur, label) {\
  sbi_printf("%s: %s: hart %d: #ticks : %lu\n",\
  __func__, label, current_hartid(), dur);}

#endif  // __SLICE_H__
