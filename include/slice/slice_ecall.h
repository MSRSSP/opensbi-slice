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

/* error codes */
#define SBI_ERR_SLICE_SUCCESS                     0
#define SBI_ERR_SLICE_UNKNOWN_ERROR               100000
#define SBI_ERR_SLICE_INVALID_ID                  100001
#define SBI_ERR_SLICE_INTERRUPTED                 100002
#define SBI_ERR_SLICE_PMP_FAILURE                 100003
#define SBI_ERR_SLICE_NOT_RUNNABLE                100004
#define SBI_ERR_SLICE_NOT_DESTROYABLE             100005
#define SBI_ERR_SLICE_REGION_OVERLAPS             100006
#define SBI_ERR_SLICE_NOT_ACCESSIBLE              100007
#define SBI_ERR_SLICE_ILLEGAL_ARGUMENT            100008
#define SBI_ERR_SLICE_NOT_RUNNING                 100009
#define SBI_ERR_SLICE_NOT_RESUMABLE               100010
#define SBI_ERR_SLICE_EDGE_CALL_HOST              100011
#define SBI_ERR_SLICE_NOT_INITIALIZED             100012
#define SBI_ERR_SLICE_NO_FREE_RESOURCE            100013
#define SBI_ERR_SLICE_SBI_PROHIBITED              100014
#define SBI_ERR_SLICE_ILLEGAL_PTE                 100015
#define SBI_ERR_SLICE_NOT_FRESH                   100016
#define SBI_ERR_SLICE_DEPRECATED                          100099
#define SBI_ERR_SLICE_NOT_IMPLEMENTED                     100100

int slice_init_host_ecall_handler();

