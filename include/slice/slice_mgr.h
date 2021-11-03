#ifndef __SLICE_MGR_H
#define __SLICE_MGR_H

#define MAX_HART_NUM 8
enum SliceIPIFuncId{
    SLICE_IPI_NONE,
    SLICE_IPI_SW_STOP,
    SLICE_IPI_PMP_DEBUG,
    SLICE_IPI_FDT_DEBUG,
};

struct SliceIPIData{
    enum SliceIPIFuncId func_id;
    void* data;
};

struct sbi_ipi_data * slice_ipi_data_ptr(u32 hartid);
void slice_send_ipi_to_domain(unsigned int dom_index, enum SliceIPIFuncId func_id);
void slice_ipi_register();
int slice_create(unsigned long cpu_mask, 
					unsigned long mem_start, 
					unsigned long mem_size, 
					unsigned long image_from,
					unsigned long image_size,
					unsigned long fdt_from);
int slice_delete(int dom_index);
#endif // __SLICE_MGR_H