#include <sbi/riscv_barrier.h>

long smp_read(long *data) { return __smp_load_acquire(data); }

long smp_exchange(long *data, long current_val, long target_val) {
  long old_val = smp_read(data);
  if (old_val == current_val) {
    __smp_store_release(data, target_val);
  }
  return old_val;
}