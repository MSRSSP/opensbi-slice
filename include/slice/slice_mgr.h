#ifndef __SLICE_MGR_H
#define __SLICE_MGR_H

#define MAX_HART_NUM 8
enum SliceIPIFuncId {
  SLICE_IPI_NONE,
  SLICE_IPI_SW_STOP,
  SLICE_IPI_PMP_DEBUG,
  SLICE_IPI_FDT_DEBUG,
};

struct SliceIPIData {
  enum SliceIPIFuncId func_id;
  void* data;
};

struct sbi_ipi_data* slice_ipi_data_ptr(u32 hartid);
void slice_ipi_register();
int slice_create(struct sbi_hartmask cpu_mask, unsigned long mem_start,
                 unsigned long mem_size, unsigned long image_from,
                 unsigned long image_size, unsigned long fdt_from,
                 unsigned long mode);
int slice_delete(int dom_index);
int slice_stop(int dom_index);
int slice_hw_reset(int dom_index);
void slice_pmp_dump_by_index(int dom_index);
#endif  // __SLICE_MGR_H