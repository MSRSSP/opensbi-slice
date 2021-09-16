#ifndef __SLICE_PMP_H__
#define __SLICE_PMP_H__

// Set PMP to enforce memory protection for the given region.

// Uses 2 PMP entries with TOR flag if the region is not naturally aligned;
// Otherwise, uses 1 pmp entry and call OPENSBI's pmp_set to setup that entry.
// Returns negative error code if unable to setup PMP for the memory;
// Returns pmp_index if this memory region is already covered by existing PMP entries;
// Returns the next available pmp index if succeeds.
int slice_set_pmp_for_mem(unsigned pmp_index, unsigned long prot,
			  unsigned long addr, unsigned long size);

// Set up pmp protection for a domain.
int slice_setup_pmp(void *dom_ptr);
int nonslice_setup_pmp(void *dom_ptr);
#endif