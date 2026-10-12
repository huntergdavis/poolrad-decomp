#ifndef POOLRAD_RECOVERED_RESIDENT_8AE_DISPATCH_H
#define POOLRAD_RECOVERED_RESIDENT_8AE_DISPATCH_H

#include <stdint.h>

/* The target is a far procedure with one WORD argument and RETF 2.
 * These are distinct effects reachable from RES_08AE:00DC. The +181A
 * callee is modeled directly because its complete byte store is known. */
typedef enum {
    PR_REC_8AE_CALL_006C,
    PR_REC_8AE_CALL_137C,
    PR_REC_8AE_CALL_13B1,
    PR_REC_8AE_CALL_1742,
    PR_REC_8AE_CALL_17AE
} pr_rec_8ae_target;

typedef struct {
    uint16_t site;       /* Offset in resident segment 0x8AE. */
    pr_rec_8ae_target target;
    uint8_t has_word;
    uint16_t word;       /* Source push value if has_word is true. */
} pr_rec_8ae_call;

typedef struct pr_rec_8ae_state pr_rec_8ae_state;

typedef struct {
    void *context;
    /* Zero means the call completed. Effects on state are visible to
     * subsequent reads of 0x5372/0x5374 at their original instruction
     * sites. A missing or failing hook stops when the call is reached. */
    int (*call)(void *context, pr_rec_8ae_state *state,
                const pr_rec_8ae_call *call);
} pr_rec_8ae_hooks;

struct pr_rec_8ae_state {
    uint8_t byte_5372;
    uint16_t word_5374;
    uint8_t driver_byte_020d; /* DS=0x8AE store by exact +181A callee. */
    const pr_rec_8ae_hooks *hooks;
};

typedef enum {
    PR_REC_8AE_OK = 0,
    PR_REC_8AE_ARGUMENT,
    PR_REC_8AE_BOUNDARY
} pr_rec_8ae_status;

typedef struct {
    uint16_t stop_offset; /* +017B after RETF, or failing call site. */
    uint8_t completed_calls;
} pr_rec_8ae_result;

/* Bounded model: external near-call effects belong to the host. It
 * neither classifies those callees as game-specific nor models DOS
 * interrupt/PIT behavior. result may be NULL. */
pr_rec_8ae_status pr_rec_res_08ae_00dc(pr_rec_8ae_state *state,
                                        uint16_t word_argument,
                                        pr_rec_8ae_result *result);

#endif
