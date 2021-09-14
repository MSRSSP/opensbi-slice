#ifndef __SLICE_ECALL_H__
#define __SLICE_ECALL_H__

/*Extension ID*/
#define SBI_EXT_EXPERIMENTAL_SLICE	0x00888888 
#define SBI_EXT_EXPERIMENTAL_IOPMP	0x00888889


/** function ID **/
#define SBI_SLICE_INFO      0x3001
#define SBI_SLICE_CREATE    0x1001
#define SBI_SLICE_RESET     0x2001
#define SBI_SLICE_MEM     	0x2	// Demo purpose

enum sbi_iopmp_opcodes {
	SBI_IOPMP_UPDATE,
	SBI_IOPMP_REMOVE
};

int slice_init_host_ecall_handler();

#endif // __SLICE_ERR_H__