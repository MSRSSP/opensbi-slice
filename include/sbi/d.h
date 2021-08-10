#ifndef __D_H__
#define __D_H__

/*Domain configurations stored in a protected memory region*/
struct sbi_domain * allocate_domain();
struct sbi_hartmask * allocate_hartmask();
struct sbi_domain_memregion * allocate_memregion();
void inc_domain_counter();
unsigned read_domain_counter();

/* Allocate a domain config*/
void *d_allocate_domain();

/* Create domain specific device tree.

Copy the root device tree to dom->next_arg1;
Remove unused devices and disable unused CPUs;
*/
int d_create_domain_fdt(const void * dom_ptr);

/* Print domain fdt information.*/
void d_print_fdt(const void * fdt);

/* Check whether the hart is a domain boot hart;*/
int sbi_is_domain_boot_hart(int hartid);

/* Prepare domain memory region.
  1. Erase memory content;
  2. Load next-stage code (next-stage bootloader, hypervisor, or kernel)
  from src to dst, as a simplified process loading image from disk/network.
*/
int prepare_domain_memory(void * dom_ptr);

struct d_extended_config{
    unsigned long next_boot_src; // Use 0x80000000 - 0x90000000 as permanent storage;
    unsigned next_boot_size;
};

#define d_printf sbi_printf

#endif // __D_H__