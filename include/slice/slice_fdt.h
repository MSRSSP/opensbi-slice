#ifndef __SLICE_FDT_H
#define __SLICE_FDT_H

#define slice_fdt(dom) ((void*)((struct sbi_domain*)dom)->next_arg1)

/* Create domain specific device tree.

Copy the root device tree to dom->next_arg1;
Remove unused devices and disable unused CPUs;
*/
int slice_create_domain_fdt(const struct sbi_domain* dom_ptr);

/* Print domain fdt information.*/
void slice_print_fdt(const void* fdt);

/* Copy fdt from dom->fdt_src to dom->next_addr */
void slice_copy_fdt(const struct sbi_domain* dom);

#endif  //__SLICE_FDT_H