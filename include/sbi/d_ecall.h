/*Extension ID*/
#define SBI_EXT_EXPERIMENTAL_D 0x00888888 


/** function ID **/
#define SBI_D_INFO      0x3001
#define SBI_D_CREATE    0x1001
#define SBI_D_RESET     0x2001

/* error codes */
#define SBI_ERR_D_SUCCESS                     0
#define SBI_ERR_D_UNKNOWN_ERROR               100000
#define SBI_ERR_D_INVALID_ID                  100001
#define SBI_ERR_D_INTERRUPTED                 100002
#define SBI_ERR_D_PMP_FAILURE                 100003
#define SBI_ERR_D_NOT_RUNNABLE                100004
#define SBI_ERR_D_NOT_DESTROYABLE             100005
#define SBI_ERR_D_REGION_OVERLAPS             100006
#define SBI_ERR_D_NOT_ACCESSIBLE              100007
#define SBI_ERR_D_ILLEGAL_ARGUMENT            100008
#define SBI_ERR_D_NOT_RUNNING                 100009
#define SBI_ERR_D_NOT_RESUMABLE               100010
#define SBI_ERR_D_EDGE_CALL_HOST              100011
#define SBI_ERR_D_NOT_INITIALIZED             100012
#define SBI_ERR_D_NO_FREE_RESOURCE            100013
#define SBI_ERR_D_SBI_PROHIBITED              100014
#define SBI_ERR_D_ILLEGAL_PTE                 100015
#define SBI_ERR_D_NOT_FRESH                   100016
#define SBI_ERR_SM_DEPRECATED                          100099
#define SBI_ERR_SM_NOT_IMPLEMENTED                     100100

int d_init_host_ecall_handler();