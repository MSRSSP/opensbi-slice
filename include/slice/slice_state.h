#ifndef __SLICE_STATE_H__
#define __SLICE_STATE_H__

enum slice_status {
  SLICE_STATUS_DELETED,
  SLICE_STATUS_RUNNING,
  SLICE_STATUS_STOPPED,
  SLICE_STATUS_FROZEN,
};

struct sbi_domain;
/* Helper functions for slice status.*/
int slice_activate(struct sbi_domain* dom);
int slice_freeze(struct sbi_domain* dom);
int slice_deactivate(struct sbi_domain* dom);
int slice_status_stop(struct sbi_domain* dom);
int slice_status_start(struct sbi_domain* dom);
int slice_is_active(struct sbi_domain* dom);
int slice_is_stopped(struct sbi_domain* dom);
int slice_is_existed(struct sbi_domain* dom);
int slice_is_running(struct sbi_domain* dom);

#endif