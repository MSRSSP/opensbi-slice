#ifndef __SLICE_DEBUG_H
#define __SLICE_DEBUG_H

#include <sbi/sbi_console.h>

enum slice_log_type {
  SLICE_LOG_DEBUG,
  SLICE_LOG_INFO,
  SLICE_LOG_WARN,
  SLICE_LOG_ERR,
};

#define SLICE_DEBUG_LEVEL SLICE_LOG_INFO

void slice_debug_color(unsigned);

#define slice_debug_printf(logLevel, ...) \
  {                                       \
    if (logLevel > SLICE_DEBUG_LEVEL) {   \
      sbi_printf("%s(): ", __func__);     \
      sbi_printf(__VA_ARGS__);            \
    }                                     \
  }

#define slice_printf(...) slice_debug_printf(SLICE_LOG_DEBUG, __VA_ARGS__)

#endif  // __SLICE_DEBUG_H