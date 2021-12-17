
/******************************************************************************************
 *
 * MPFS HSS Embedded Software
 *
 * Copyright 2019-2021 Microchip FPGA Embedded Systems Solutions.
 *
 * SPDX-License-Identifier: MIT
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 *
 * Originally based on code from OpenSBI, which is:
 *
 * Copyright (c) 2019 Western Digital Corporation or its affiliates.
 *
 */

#include <assert.h>
#include <sbi/sbi_types.h>

#include "config.h"
#include "hss_types.h"
#define false FALSE
#define true TRUE

#include <libfdt.h>
#include <sbi/riscv_asm.h>
#include <sbi/riscv_encoding.h>
#include <sbi/riscv_io.h>
#include <sbi/sbi_console.h>
#include <sbi/sbi_const.h>
#include <sbi/sbi_domain.h>
#include <sbi/sbi_hart.h>
#include <sbi/sbi_hartmask.h>
#include <sbi/sbi_math.h>
#include <sbi/sbi_platform.h>
#include <sbi_utils/fdt/fdt_fixup.h>
#include <sbi_utils/irqchip/plic.h>
#include <sbi_utils/serial/uart8250.h>
#include <sbi_utils/sys/clint.h>
#include <slice/slice_mgr.h>

//#include "opensbi_service.h"
#include "cache.h"
#define MPFS_HART_COUNT 5
#define MPFS_HART_STACK_SIZE 8192

#define MPFS_SYS_CLK 1000000000

#define MPFS_CLINT_ADDR 0x2000000

#define MPFS_PLIC_ADDR 0xc000000
#define MPFS_PLIC_NUM_SOURCES 186
#define MPFS_PLIC_NUM_PRIORITIES 7

#define MPFS_UART0_ADDR 0x10010000
#define MPFS_UART1_ADDR 0x10011000
#define MPFS_UART_BAUDRATE 115200

#define HART_0_MEM_ADDR 0xa0000000
#define HART_0_MEM_ORDER 28

#define IPI_REG_WIDTH 4

#define ALL_PERM                                                \
  (SBI_DOMAIN_MEMREGION_MMODE | SBI_DOMAIN_MEMREGION_READABLE | \
   SBI_DOMAIN_MEMREGION_WRITEABLE | SBI_DOMAIN_MEMREGION_EXECUTABLE)
#define ALL_PERM_BUT_X                                          \
  (SBI_DOMAIN_MEMREGION_MMODE | SBI_DOMAIN_MEMREGION_READABLE | \
   SBI_DOMAIN_MEMREGION_WRITEABLE)
#define READ_ONLY (SBI_DOMAIN_MEMREGION_MMODE | SBI_DOMAIN_MEMREGION_READABLE)

/**
 * PolarFire SoC has 5 HARTs but HART ID 0 doesn't have S mode. enable only
 * HARTs 1 to 4.
 */
#ifndef MPFS_ENABLED_HART_MASK
#define MPFS_ENABLED_HART_MASK (1 << 1 | 1 << 2 | 1 << 3 | 1 << 4)
#endif

#define DOMAIN_REGION_MAX_COUNT 2

#define MPFS_HARITD_DISABLED ~(MPFS_ENABLED_HART_MASK)

struct plic_data plicInfo = {.addr = MPFS_PLIC_ADDR,
                             .num_src = MPFS_PLIC_NUM_SOURCES};

struct clint_data clintInfo = {.addr = MPFS_CLINT_ADDR,
                               .first_hartid = 0,
                               .hart_count = MPFS_HART_COUNT,
                               .has_64bit_mmio = TRUE};

extern unsigned long STACK_SIZE_PER_HART;

static void mpfs_modify_dt(void *fdt) {
  //fdt_cpu_fixup(fdt, sbi_domain_thishart_ptr());

  //fdt_fixups(fdt, sbi_domain_thishart_ptr());

  // fdt_reserved_memory_nomap_fixup(fdt); // not needed for PolarFire SoC
}

static int mpfs_final_init(bool cold_boot) {
  void *fdt;

  if (!cold_boot) {
    return 0;
  }

  fdt = sbi_scratch_thishart_arg1_ptr();
  mpfs_modify_dt(fdt);

  return 0;
}

static bool console_initialized = false;

static void mpfs_console_putc(char ch) {
  if (console_initialized) {
    u32 hartid = current_hartid();
    int uart_putc(int hartid, const char ch);  // TBD
    uart_putc(hartid, ch);
  }
}

#define NO_BLOCK 0
#define GETC_EOF -1
static int mpfs_console_getc(void) {
  int result = GETC_EOF;
  bool uart_getchar(uint8_t * pbuf, int32_t timeout_sec, bool do_sec_tick);

  uint8_t rcvBuf;
  if (uart_getchar(&rcvBuf, NO_BLOCK, FALSE)) {
    result = rcvBuf;
  }

  return result;
}

static struct sbi_console_device mpfs_console = {
    .name = "mpfs_uart",
    .console_putc = mpfs_console_putc,
    .console_getc = mpfs_console_getc};

extern void HSS_UARTInit(void);

static int mpfs_console_init(void) {
  HSS_UARTInit();
  sbi_console_set_device(&mpfs_console);
  console_initialized = true;
  return 0;
}

static int mpfs_irqchip_init(bool cold_boot) {
  int rc = 0;
  u32 hartid = current_hartid();

  if (hartid == slice_host_hartid()) {
    rc = plic_cold_irqchip_init(&plicInfo);
  }

  if (!rc) {
    rc = plic_warm_irqchip_init(&plicInfo, (hartid) ? (2 * hartid - 1) : 0,
                                (hartid) ? (2 * hartid) : -1);
  }

  return rc;
}

static int mpfs_ipi_init(bool cold_boot) {
  int rc = 0;

  if (cold_boot) {
    rc = clint_cold_ipi_init(&clintInfo);
  }

  if (!rc) {
    rc = clint_warm_ipi_init();
  }

  return rc;
}

static int mpfs_timer_init(bool cold_boot) {
  int rc = 0;

  if (cold_boot) {
    rc = clint_cold_timer_init(&clintInfo, NULL);
  }

  if (!rc) {
    clint_warm_timer_init();
  }

  return rc;
}

static void mpfs_system_down(u32 reset_type, u32 reset_reason) {
  /* For now nothing to do, we'll instead rely on
   * mpfs_final_exit() kicking off the restart... */
  (void)reset_type;
  (void)reset_reason;

  /* re-enable IPIs */
  csr_write(CSR_MSTATUS, MIP_MSIP);
  csr_write(CSR_MIE, MIP_MSIP);

  //void HSS_OpenSBI_Reboot(void);
  //HSS_OpenSBI_Reboot();

  while (1)
    ;
}

static void mpfs_final_exit(void) {
  /* re-enable IPIs */
  csr_write(CSR_MSTATUS, MIP_MSIP);
  csr_write(CSR_MIE, MIP_MSIP);
  mpfs_system_down(0, 0);
}

#define MPFS_TLB_RANGE_FLUSH_LIMIT 0u
static u64 mpfs_get_tlbr_flush_limit(void) {
  return MPFS_TLB_RANGE_FLUSH_LIMIT;
}
/*
// don't allow OpenSBI to play with PMPs
int sbi_hart_pmp_configure(struct sbi_scratch *pScratch)
{
    (void)pScratch;
    return 0;
}*/

struct hart_info {
  char name[64];
  u64 next_addr;
  u64 next_arg1;
  struct sbi_hartmask hartMask;
  u32 next_mode;
  int owner_hartid;
  int boot_pending;
  u64 mem_start;
  u64 mem_size;
  u64 next_boot_src;
  size_t next_boot_size;
  unsigned long slice_fdt_src;
  char uart_path[SLICE_UART_PATH_LEN];
};

static struct hart_info *hart_table =
    (struct hart_info *)CONFIG_SERVICE_BOOT_DDR_SLICE_PRIVATE_START;
void mpfs_domains_register_hart(int hartid, int boot_hartid) {
  hart_table[hartid].owner_hartid = boot_hartid;
  hart_table[hartid].boot_pending = 1;
}

void mpfs_mark_hart_as_booted(enum HSSHartId hartid) {
  assert(hartid < MAX_NUM_HARTS);

  if (hartid < MAX_NUM_HARTS) {
    hart_table[hartid].boot_pending = 0;
  }
}

bool mpfs_is_last_hart_booting(void) {
  int outstanding = 0;
  for (int hartid = 0; hartid < MAX_NUM_HARTS; hartid++) {
    outstanding += hart_table[hartid].boot_pending;
  }

  if (outstanding > 1) {
    return false;
  } else {
    return true;
  }
}

void slice_register_boot_hart(int boot_hartid, unsigned long boot_src,
                              size_t boot_size, unsigned long fdt_src,
                              const char *uart_path) {
  hart_table[boot_hartid].next_boot_src = boot_src;
  hart_table[boot_hartid].next_boot_size = boot_size;
  hart_table[boot_hartid].slice_fdt_src = fdt_src;
  int len = array_size(hart_table[boot_hartid].uart_path) - 1;
  len = sbi_strlen(uart_path) > len ? len : sbi_strlen(uart_path);
  sbi_memset(hart_table[boot_hartid].uart_path, 0, SLICE_UART_PATH_LEN);
  sbi_memcpy(hart_table[boot_hartid].uart_path, uart_path, len);
}

static int slice_unregister_hart(unsigned hartid) {
  sbi_printf("%s: hartid = %d size=%ld\n", __func__, hartid,
             sizeof(hart_table[hartid]));
  /*
  hart_table[hartid].owner_hartid = 0;
  hart_table[hartid].mem_size = 0;
  hart_table[hartid].hartMask.bits[0] = 0;
  */
  sbi_memset(&hart_table[hartid], 0 , sizeof(hart_table[hartid]));
  return 0;
}

unsigned long slice_mem_start_this_hart(void) {
  int hartid = current_hartid();
  int owner = hart_table[hartid].owner_hartid;
  return hart_table[owner].mem_start;
}
unsigned long slice_mem_size_this_hart(void) {
  int hartid = current_hartid();
  int owner = hart_table[hartid].owner_hartid;
  return hart_table[owner].mem_size;
}

unsigned slice_owner_hart(unsigned hartid) {
  return hart_table[hartid].owner_hartid;
}

bool is_slice_sbi_copy_done(void) {
  int hartid = current_hartid();
  int owner = hart_table[hartid].owner_hartid;
  if (hart_table[hartid].boot_pending == 0) {
    // This is a slice reset.
    // Wait for primary hart to reset boot status.
    return false;
  }
  return hart_table[owner].boot_pending == 0;
}

void init_slice_sbi_copy_status(void) {
  int owner = current_hartid();
  hart_table[owner].boot_pending = 1;
  for (u32 hartid = 0; hartid < MAX_NUM_HARTS; ++hartid) {
    if (owner == hart_table[hartid].owner_hartid) {
      hart_table[hartid].boot_pending = 1;
    }
  }
}

static int _mpfs_domains_register_boot_hart(char *pName, u32 hartMask,
                                            int boot_hartid, u32 privMode,
                                            unsigned long mem_start,
                                            unsigned long mem_size) {
  hart_table[boot_hartid].owner_hartid = boot_hartid;
  sbi_memcpy(hart_table[boot_hartid].name, pName,
         ARRAY_SIZE(hart_table[boot_hartid].name) - 1);
  hart_table[boot_hartid].mem_size = mem_size;
  hart_table[boot_hartid].mem_start = mem_start;
  sbi_hartmask_clear_all(&hart_table[boot_hartid].hartMask);
  hart_table[boot_hartid].hartMask.bits[0] = hartMask;
  u32 hartid;
  sbi_hartmask_for_each_hart(hartid, &hart_table[boot_hartid].hartMask) {
    mpfs_domains_register_hart(hartid, boot_hartid);
  }
  hart_table[boot_hartid].next_mode = privMode;
  return 0;
}

void mpfs_domains_register_boot_hart(char *pName, u32 hartMask, int boot_hartid,
                                     u32 privMode, void *entryPoint,
                                     void *pArg1, unsigned long mem_size) {
  assert(hart_table[boot_hartid].owner_hartid == boot_hartid);
  _mpfs_domains_register_boot_hart(pName, hartMask, boot_hartid, privMode,
                                   (unsigned long)entryPoint, mem_size);
}

static int init_slice_shared_mem(struct sbi_domain_memregion *regions,
                                 unsigned int count, unsigned int *out_count) {
  // Dirty region for test; To be removed
  // CLINT map except for IPI
  regions[count].base = MPFS_CLINT_ADDR + MPFS_HART_COUNT * IPI_REG_WIDTH;
  regions[count].order = 30;
  regions[count].flags = ALL_PERM_BUT_X;
  count++;
  if (count > DOMAIN_REGION_MAX_COUNT) {
    return SBI_ERR_FAILED;
  }
  // PLIC
  /*regions[count].base = MPFS_PLIC_ADDR;
  regions[count].order = 30;
  regions[count].flags = ALL_PERM_BUT_X;
  count++;
  if (count > DOMAIN_REGION_MAX_COUNT) {
    return SBI_ERR_FAILED;
  }*/
  // PLIC
  regions[count].base = 0x2008000000;
  regions[count].order = 27;
  regions[count].flags = ALL_PERM_BUT_X;
  count++;
  if (count > DOMAIN_REGION_MAX_COUNT) {
    return SBI_ERR_FAILED;
  }
  // Host memory.
  /*regions[count].base = CONFIG_SERVICE_BOOT_DDR_SLICE_0_MEM_START;
  regions[count].order = CONFIG_SERVICE_BOOT_DDR_SLICE_0_MEM_ORDER;
  regions[count].flags = READ_ONLY;
  count++;
  if (count > DOMAIN_REGION_MAX_COUNT) {
    return SBI_ERR_FAILED;
  }*/
  *out_count = count;
  return 0;
}

int init_slice_mem_regions(struct sbi_domain *pDom) {
  unsigned count = 0;
  struct sbi_scratch *const pScratch = sbi_scratch_thishart_ptr();
  sbi_domain_memregion_init(
      pScratch->fw_start & ~((1UL << log2roundup(pScratch->fw_size)) - 1UL),
      1 << log2roundup(pScratch->fw_size), 0, &pDom->regions[count++]);
  init_slice_shared_mem(pDom->regions, count, &count);
  sbi_domain_memregion_init(pDom->next_addr & (~((1UL << 28) - 1)),
                            pDom->slice_mem_size, ALL_PERM,
                            &pDom->regions[count++]);
  if (count > DOMAIN_REGION_MAX_COUNT) {
    return SBI_EINVAL;
  }
  return 0;
}

//static struct sbi_domain dom_table[MAX_NUM_HARTS] = {0};
//static struct sbi_domain_memregion domain_regions[MAX_NUM_HARTS]
//                                                [DOMAIN_REGION_MAX_COUNT + 1];
static int mpfs_domains_init(void) {
  // register all AMP domains
  int result = 0;
  // Set hart0 as host hart;
  register_host_hartid(0);
  sbi_printf("mpfs_domains_init\n");
  /*
  for (int hartid = 1; hartid < MAX_NUM_HARTS; hartid++) {
    const int boot_hartid = hart_table[hartid].owner_hartid;
    if(boot_hartid >= array_size(dom_table) ){
      continue;
    }
    if (boot_hartid == hartid) {
      struct sbi_domain *const pDom = &dom_table[boot_hartid];
      pDom->regions = domain_regions[boot_hartid];

      if (!pDom->index) {  // { pDom->boot_hartid != boot_hartid) {
        pDom->boot_hartid = boot_hartid;

        // TODO: replace sbi_memcpy with something like strlcpy
        sbi_memcpy(pDom->name, hart_table[boot_hartid].name,
               ARRAY_SIZE(dom_table[0].name) - 1);
        struct slice_options options = {hart_table[boot_hartid].hartMask,
                                        hart_table[boot_hartid].mem_start,
                                        hart_table[boot_hartid].mem_size,
                                        hart_table[boot_hartid].next_boot_src,
                                        hart_table[boot_hartid].next_boot_size,
                                        hart_table[boot_hartid].slice_fdt_src,
                                        hart_table[boot_hartid].next_mode,
                                        {0}};
        hart_table[boot_hartid].uart_path[SLICE_UART_PATH_LEN -1] = 0;
        sbi_memcpy(options.stdout, hart_table[boot_hartid].uart_path,
               sbi_strlen(hart_table[boot_hartid].uart_path));
        result = slice_create_full(&options);
      }
    }
  }*/
  sbi_printf("end mpfs_domains_init\n");
  return result;
}

const struct sbi_platform_operations platform_ops = {
    .early_init = NULL,
    .final_init = mpfs_final_init,
    .early_exit = NULL,
    .final_exit = mpfs_final_exit,

    .misa_check_extension = NULL,
    .misa_get_xlen = NULL,

    .console_init = mpfs_console_init,

    .irqchip_init = mpfs_irqchip_init,
    .irqchip_exit = NULL,

    //.ipi_send = clint_ipi_send,
    //.ipi_clear = clint_ipi_clear,
    .ipi_init = mpfs_ipi_init,
    .ipi_exit = NULL,

    .get_tlbr_flush_limit = mpfs_get_tlbr_flush_limit,

    //.timer_value = clint_timer_value,
    //.timer_event_start = clint_timer_event_start,
    //.timer_event_stop = clint_timer_event_stop,
    .timer_init = mpfs_timer_init,
    .timer_exit = NULL,

    //.system_reset = mpfs_system_down,

    .domains_init = mpfs_domains_init,
    .slice_init_mem_region = init_slice_mem_regions,
    .slice_unregister_hart = slice_unregister_hart,
    .slice_register_hart = _mpfs_domains_register_boot_hart,
    .slice_register_source = slice_register_boot_hart,
    .vendor_ext_check = NULL,
    .vendor_ext_provider = NULL,
};

const u32 mpfs_hart_index2id[MPFS_HART_COUNT] = {
    [0] = -1, [1] = 1, [2] = 2, [3] = 3, [4] = 4,
};

const struct sbi_platform platform = {
    .opensbi_version = OPENSBI_VERSION,
    .platform_version = SBI_PLATFORM_VERSION(0x0, 0x01),
    .name = "Microchip PolarFire(R) SoC",
    .features = SBI_PLATFORM_DEFAULT_FEATURES,  // already have PMPs setup
    .hart_count = MPFS_HART_COUNT,
    .hart_index2id = mpfs_hart_index2id,
    .hart_stack_size = SBI_PLATFORM_DEFAULT_HART_STACK_SIZE,
    .platform_ops_addr = (unsigned long)&platform_ops,
    .firmware_context = 0,
};
