/*******************************************************************************
 * Copyright 2019-2021 Microchip FPGA Embedded Systems Solutions.
 *
 * SPDX-License-Identifier: MIT
 *
 * MPFS HSS Embedded Software
 *
 */

/*!
 * \file assert implementation
 * \brrief Local assert
 */

#include "sbi/sbi_console.h"
#include "sbi/sbi_hart.h"

/*!
 * \brief Local implemention of assert fail
 */

__attribute__((__noreturn__)) void __assert_func(const char *__file,
                                                 unsigned int __line,
                                                 const char *__function,
                                                 const char *__assertion);
__attribute__((__noreturn__)) void __assert_func(const char *__file,
                                                 unsigned int __line,
                                                 const char *__function,
                                                 const char *__assertion) {
  sbi_printf("%s:%d: %s() Assertion failed: \t%s", __file, __line, __function,
             __assertion);

#ifndef __riscv
  exit(1);
#else
  sbi_hart_hang();
#endif
}

__attribute__((__noreturn__)) void __assert_fail(const char *__file,
                                                 unsigned int __line,
                                                 const char *__function,
                                                 const char *__assertion);
__attribute__((__noreturn__)) void __assert_fail(const char *__file,
                                                 unsigned int __line,
                                                 const char *__function,
                                                 const char *__assertion) {
  __assert_func(__file, __line, __function, __assertion);
}
