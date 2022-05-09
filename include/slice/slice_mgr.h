#ifndef __SLICE_MGR_H
#define __SLICE_MGR_H
#include <sbi/sbi_hartmask.h>

#define MAX_HART_NUM 8
enum SliceIPIFuncId {
	SLICE_IPI_NONE,
	SLICE_IPI_SW_STOP,
	SLICE_IPI_PMP_DEBUG,
	SLICE_IPI_FDT_DEBUG,
};

struct SliceIPIData {
	enum SliceIPIFuncId func_id;
	void *data;
};

// TODO: Add a lock to avoid dst hart ignors a pending IPI
// when multiple src harts tries to send IPI to it;
// struct SliceIPIData slice_ipi_data[MAX_HART_NUM][MAX_HART_NUM];
struct slice_ipi_data {
	unsigned long ipi_type;
	struct SliceIPIData slice_data[MAX_HART_NUM];
};

struct slice_bus_data {
	struct slice_ipi_data slice_buses[MAX_HART_NUM];
};

#define SLICE0_BUS_MEMORY 0xa0000000
#define SLICE_IPI_DATA_OFFSET 0x180000
#define SLICE_OS_OFFSET 0x200000
#define SLICE_FDT_OFFSET 0x2000000

#define SLICE_STDOUT_PATH_LEN 32

struct slice_options {
	struct sbi_hartmask hartmask;
	unsigned long mem_start;
	unsigned long mem_size;
	unsigned long image_from;
	unsigned long image_size;
	unsigned long fdt_from;
	unsigned long guest_mode;
	char stdout[SLICE_STDOUT_PATH_LEN];
};

struct sbi_ipi_data *slice_ipi_data_ptr(u32 hartid);
void slice_ipi_register();
int slice_create_full(struct slice_options *slice_options);

int slice_delete(int dom_index);
int slice_stop(int dom_index);
int slice_hw_reset(int dom_index);
void slice_pmp_dump_by_index(int dom_index);
void dump_slices_config(void);
// Test only; 
// TBD: remove in production.
void slice_ipi_test(int dom_index);

/* Slice-0 setter and getter */
unsigned int slice_host_hartid();
int register_host_hartid(unsigned int hartid);

void slice_register_boot_hart(int boot_hartid, const void * options);

// Returns the slice memory start for this hart;
unsigned long slice_mem_start_this_hart(void);

// Returns the slice memory size for this hart;
unsigned long slice_mem_size_this_hart(void);

// Whether the slice's owner hart already copied the hss-l2 to slice memory;
bool is_slice_sbi_copy_done(void);
void init_slice_sbi_copy_status(void);

// Return True if this hart is the slice's owner hart;
unsigned slice_owner_hart(unsigned hartid);
#endif // __SLICE_MGR_H