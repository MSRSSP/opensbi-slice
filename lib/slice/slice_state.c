#include <sbi/sbi_domain.h>
#include <slice/slice_err.h>
#include <slice/slice_state.h>

// In PolarFire, only selected memory regions support atomic operations.
#define atomic_cmpxchg smp_exchange
#define atomic_read smp_read

int slice_activate(struct sbi_domain *dom) {
  enum slice_status oldstate = atomic_cmpxchg(
      &dom->slice_status, SLICE_STATUS_DELETED, SLICE_STATUS_STOPPED);
  if (oldstate != SLICE_STATUS_DELETED) {
    return SBI_ERR_SLICE_STATUS;
  }
  return 0;
}

int slice_freeze(struct sbi_domain *dom) {
  int oldstate = atomic_cmpxchg(&dom->slice_status, SLICE_STATUS_RUNNING,
                                SLICE_STATUS_FROZEN);
  oldstate = atomic_cmpxchg(&dom->slice_status, SLICE_STATUS_STOPPED,
                            SLICE_STATUS_FROZEN);
  if (oldstate != SLICE_STATUS_RUNNING && oldstate != SLICE_STATUS_STOPPED) {
    return SBI_ERR_SLICE_STATUS;
  }
  return 0;
}

int slice_deactivate(struct sbi_domain *dom) {
  int oldstate = atomic_cmpxchg(&dom->slice_status, SLICE_STATUS_FROZEN,
                                SLICE_STATUS_DELETED);
  if (oldstate != SLICE_STATUS_FROZEN) {
    return SBI_ERR_SLICE_STATUS;
  }
  return 0;
}

int slice_status_stop(struct sbi_domain *dom) {
  int oldstate = atomic_cmpxchg(&dom->slice_status, SLICE_STATUS_RUNNING,
                                SLICE_STATUS_STOPPED);
  if (oldstate != SLICE_STATUS_RUNNING) {
    return SBI_ERR_SLICE_STATUS;
  }
  return 0;
}

int slice_status_start(struct sbi_domain *dom) {
  int oldstate = atomic_cmpxchg(&dom->slice_status, SLICE_STATUS_STOPPED,
                                SLICE_STATUS_RUNNING);
  if (oldstate != SLICE_STATUS_STOPPED) {
    return SBI_ERR_SLICE_STATUS;
  }
  return 0;
}

int slice_is_active(struct sbi_domain *dom) {
  int state = atomic_read(&dom->slice_status);
  return (state == SLICE_STATUS_RUNNING) || (state == SLICE_STATUS_STOPPED);
}

int slice_is_running(struct sbi_domain *dom) {
  int state = atomic_read(&dom->slice_status);
  return state == SLICE_STATUS_RUNNING;
}

int slice_is_stopped(struct sbi_domain *dom) {
  int state = atomic_read(&dom->slice_status);
  return state == SLICE_STATUS_STOPPED;
}

int slice_is_existed(struct sbi_domain *dom) {
  return atomic_read(&dom->slice_status) != SLICE_STATUS_DELETED;
}