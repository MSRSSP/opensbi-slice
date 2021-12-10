

#include <sbi/sbi_domain.h>
#include <sbi/sbi_scratch.h>

unsigned long fw_addr, fw_size, slice_cfg_out_of_slice_mem;
unsigned long scratch_in_slice_mem;
extern void slice_loader_finish(struct sbi_scratch *private_scratch,
                                struct sbi_domain *slice_cfg_out_of_slice_mem);
extern void slice_loader(struct sbi_domain *dom, unsigned long fw_src,
                         unsigned long fw_size);

// Stub out sbi_init
void sbi_init(struct sbi_scratch *private_scratch) {}

struct sbi_ipi_data *slice_ipi_data_ptr(u32 hartid) {
  return 0;
}

int slice_loader_main(int narg, void **argv) {
  slice_loader((struct sbi_domain *)slice_cfg_out_of_slice_mem, fw_addr,
               fw_size);
  slice_loader_finish((struct sbi_scratch *)scratch_in_slice_mem,
                      (struct sbi_domain *)slice_cfg_out_of_slice_mem);
}