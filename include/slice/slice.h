#ifndef __SLICE_H__
#define __SLICE_H__

/*Domain configurations stored in a protected memory region*/
struct sbi_domain * allocate_domain();
struct sbi_hartmask * allocate_hartmask();
struct sbi_domain_memregion * allocate_memregion();
void inc_domain_counter();
unsigned read_domain_counter();

enum slice_type {
	SLICE_TYPE_STANDARD_DOMAIN,
	SLICE_TYPE_SLICE,
};

/* Allocate a domain config*/
void *slice_allocate_domain();

/* Create domain specific device tree.

Copy the root device tree to dom->next_arg1;
Remove unused devices and disable unused CPUs;
*/
int slice_create_domain_fdt(const void * dom_ptr);

/* Print domain fdt information.*/
void slice_print_fdt(const void * fdt);

/* Return slice boot hart if this hart belongs to a slice.*/
int slice_boot_hart(void);

/* Prepare domain memory region.
  1. Erase memory content;
  2. Load next-stage code (next-stage bootloader, hypervisor, or kernel)
  from src to dst, as a simplified process loading image from disk/network.
*/
int slice_setup_domain(void * dom_ptr);

// Jump to slice's sbi stage.
void __attribute__((noreturn)) 
slice_to_sbi(void* slice_mem_start, void* slice_sbi_start, unsigned long slice_sbi_size);

void dump_slice_config(const struct sbi_domain * dom_ptr);

struct slice_config{
  enum slice_type slice_type;
  // source address of device tree for this slice;
  // in a protected memory region; The region could be
  // 1. Only writable by host and this slice;  
  // 2. Writable by host and only readable by sliced guests.
  void * slice_dt_src; 
  // source address of the next boot image;
  // in host protected memory;
  // copy context from next_boot_src to next_addr;
  unsigned long next_boot_src;
  // size of next boot image;
  unsigned next_boot_size;
  /** default stdio **/
	char stdout_path[64];
  unsigned long slice_mem_start;
  unsigned long slice_mem_size;
  unsigned long slice_sbi_start;
  unsigned long slice_sbi_size;
};

bool is_slice(const struct sbi_domain *dom);

unsigned int slice_host_hartid();

int register_host_hartid(unsigned int hartid);

#define slice_printf sbi_printf
//#define slice_printf(x, ...) {}
#endif // __D_H__
