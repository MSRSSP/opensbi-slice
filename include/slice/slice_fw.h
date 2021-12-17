#ifndef SLICE_FW_H
#define SLICE_FW_H
/** Offset of magic member in fw_dynamic_info */
#define SLICE_FW_INFO_MAGIC_OFFSET		(0 * __SIZEOF_POINTER__)
/** Offset of version member in fw_dynamic_info */
#define SLICE_FW_INFO_VERSION_OFFSET		(1 * __SIZEOF_POINTER__)
/** Offset of next_addr member in fw_dynamic_info (version >= 1) */
#define SLICE_FW_INFO_NEXT_ADDR_OFFSET	(2 * __SIZEOF_POINTER__)
/** Offset of next_mode member in fw_dynamic_info  (version >= 1) */
#define SLICE_FW_INFO_NEXT_MODE_OFFSET	(3 * __SIZEOF_POINTER__)
/** Offset of options member in fw_dynamic_info  (version >= 1) */
#define SLICE_FW_INFO_OPTIONS_OFFSET		(4 * __SIZEOF_POINTER__)
/** Offset of boot_hart member in fw_dynamic_info  (version >= 2) */
#define SLICE_FW_INFO_BOOT_HART_OFFSET	(5 * __SIZEOF_POINTER__)

#define SLICE_FW_INFO_SLICE_CFG_OFFSET	(6 * __SIZEOF_POINTER__)
#define SLICE_FW_INFO_SLICE_SCRATCH_OFFSET	(7 * __SIZEOF_POINTER__)

/** Expected value of info magic ('OSBI' ascii string in hex) */
#define SLICE_FW_INFO_MAGIC_VALUE		0x4942534f

/** Maximum supported info version */
#define SLICE_FW_INFO_VERSION_MAX		0x2

/** Possible next mode values */
#define SLICE_FW_INFO_NEXT_MODE_U		0x0
#define SLICE_FW_INFO_NEXT_MODE_S		0x1
#define SLICE_FW_INFO_NEXT_MODE_M		0x3

#ifndef __ASSEMBLER__

/** Representation dynamic info passed by previous booting stage */
struct fw_dynamic_info {
	/** Info magic */
	unsigned long magic;
	/** Info version */
	unsigned long version;
	/** Next booting stage address */
	unsigned long next_addr;
	/** Next booting stage mode */
	unsigned long next_mode;
	/** Options for OpenSBI library */
	unsigned long options;
	/**
	 * Preferred boot HART id
	 *
	 * It is possible that the previous booting stage uses same link
	 * address as the FW_DYNAMIC firmware. In this case, the relocation
	 * lottery mechanism can potentially overwrite the previous booting
	 * stage while other HARTs are still running in the previous booting
	 * stage leading to boot-time crash. To avoid this boot-time crash,
	 * the previous booting stage can specify last HART that will jump
	 * to the FW_DYNAMIC firmware as the preferred boot HART.
	 *
	 * To avoid specifying a preferred boot HART, the previous booting
	 * stage can set it to -1UL which will force the FW_DYNAMIC firmware
	 * to use the relocation lottery mechanism.
	 */
	unsigned long boot_hart;
} __packed;

struct slice_fw_info{
    struct fw_dynamic_info sbi_fw_info;
    unsigned long slice_cfg_ptr;
	unsigned long slice_scratch_ptr;
};

#endif 

#endif //SLICE_FW_H