#ifndef __HARTS_H
#define __HARTS_H
enum HSSHartId {
    HSS_HART_E51 = 0,
    HSS_HART_U54_1,
    HSS_HART_U54_2,
    HSS_HART_U54_3,
    HSS_HART_U54_4,

    HSS_HART_NUM_PEERS,
    HSS_HART_ALL = HSS_HART_NUM_PEERS
};
#define MAX_NUM_HARTS ((unsigned)HSS_HART_NUM_PEERS)

#endif // __HARTS_H