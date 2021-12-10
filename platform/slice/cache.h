#ifndef SLICE_CACHE_H
#define SLICE_CACHE_H

#ifndef CACHE_CTRL
#define CACHE_CTRL_BASE     0x02010000ULL
#endif

#define RO  volatile const
#define RW  volatile
#define WO  volatile

typedef struct {
  RO uint8_t  BANKS;
  RO uint8_t  WAYS;
  RO uint8_t  SETS;
  RO uint8_t  BYTES;
} CACHE_CONFIG_typedef;

typedef struct {
  CACHE_CONFIG_typedef CONFIG;
  RO uint32_t RESERVED;
  RW uint8_t  WAY_ENABLE;
  RO uint8_t  RESERVED0[55];

  RW uint32_t ECC_INJECT_ERROR;
  RO uint32_t RESERVED1[47];

  RO uint64_t ECC_DIR_FIX_ADDR;
  RO uint32_t ECC_DIR_FIX_COUNT;
  RO uint32_t RESERVED2[13];

  RO uint64_t ECC_DATA_FIX_ADDR;
  RO uint32_t ECC_DATA_FIX_COUNT;
  RO uint32_t RESERVED3[5];

  RO uint64_t ECC_DATA_FAIL_ADDR;
  RO uint32_t ECC_DATA_FAIL_COUNT;
  RO uint32_t RESERVED4[37];

  WO uint64_t FLUSH64;
  RO uint64_t RESERVED5[7];

  WO uint32_t FLUSH32;
  RO uint32_t RESERVED6[367];

  RW uint64_t WAY_MASK_DMA;

  RW uint64_t WAY_MASK_AXI4_SLAVE_PORT_0;
  RW uint64_t WAY_MASK_AXI4_SLAVE_PORT_1;
  RW uint64_t WAY_MASK_AXI4_SLAVE_PORT_2;
  RW uint64_t WAY_MASK_AXI4_SLAVE_PORT_3;

  RW uint64_t WAY_MASK_E51_DCACHE;
  RW uint64_t WAY_MASK_E51_ICACHE;

  RW uint64_t WAY_MASK_U54_1_DCACHE;
  RW uint64_t WAY_MASK_U54_1_ICACHE;

  RW uint64_t WAY_MASK_U54_2_DCACHE;
  RW uint64_t WAY_MASK_U54_2_ICACHE;

  RW uint64_t WAY_MASK_U54_3_DCACHE;
  RW uint64_t WAY_MASK_U54_3_ICACHE;

  RW uint64_t WAY_MASK_U54_4_DCACHE;
  RW uint64_t WAY_MASK_U54_4_ICACHE;
} CACHE_CTRL_typedef;

#ifndef CACHE_CTRL
#define CACHE_CTRL  ((volatile CACHE_CTRL_typedef *) CACHE_CTRL_BASE)
#endif

#endif
