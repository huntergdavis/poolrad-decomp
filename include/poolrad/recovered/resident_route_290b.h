#ifndef POOLRAD_RECOVERED_RESIDENT_ROUTE_290B_H
#define POOLRAD_RECOVERED_RESIDENT_ROUTE_290B_H

#include <stdint.h>

/* Bounded model of RES_00BA:290B (loaded 0x034AB). External calls and
 * string contents remain host boundaries. */
typedef enum {
    PR_REC_290B_LOCAL_1C13,
    PR_REC_290B_LOCAL_1BD7,
    PR_REC_290B_SYSTEM_0B25,
    PR_REC_290B_SYSTEM_0B0B,
    PR_REC_290B_SYSTEM_0B98,
    PR_REC_290B_RES_03F1_0137,
    PR_REC_290B_RES_0709_0626
} pr_rec_290b_target;

typedef enum {
    PR_REC_290B_WORD,
    PR_REC_290B_AX_LOW_BYTE,
    PR_REC_290B_SS_LOCAL,
    PR_REC_290B_CS_OFFSET,
    PR_REC_290B_DS_OFFSET
} pr_rec_290b_push_kind;

typedef struct {
    pr_rec_290b_push_kind kind;
    /* WORD is a full pushed word; AX_LOW_BYTE fixes only the low byte.
     * SS_LOCAL values are positive BP-minus displacements; CS_OFFSET and
     * DS_OFFSET are offsets in the indicated segment. */
    uint16_t value;
} pr_rec_290b_push;

typedef struct {
    uint16_t image_site;
    pr_rec_290b_target target;
    uint8_t push_count;
    /* Logical far pointers occupy one entry, in source push order. The
     * near-call push CS used for the far return is not a callee argument. */
    pr_rec_290b_push pushes[4];
} pr_rec_290b_call;

typedef struct {
    uint16_t offset;
    uint16_t segment;
} pr_rec_290b_far_ptr;

typedef struct {
    void *context;
    /* Zero means the host operation completed. All reads occur at their
     * decoded sites, so callbacks may observe effects of earlier calls. */
    int (*read_ds_byte)(void *context, uint16_t offset, uint8_t *value);
    int (*read_ds_far_ptr)(void *context, uint16_t offset,
                           pr_rec_290b_far_ptr *value);
    int (*read_far_byte)(void *context, pr_rec_290b_far_ptr base,
                         uint16_t effective_offset, uint8_t *value);
    int (*read_far_word)(void *context, pr_rec_290b_far_ptr base,
                         uint16_t effective_offset, uint16_t *value);
    /* At image sites 0x03513 and 0x0356F, observed_length is nonnull and
     * must receive the post-call byte at local [BP-5] or [BP-8]. The model
     * uses it for the following unsigned comparison with 2. Other calls
     * receive null. String data and callee side effects belong to the host. */
    int (*call)(void *context, const pr_rec_290b_call *call,
                uint8_t *observed_length);
} pr_rec_290b_io;

typedef enum {
    PR_REC_290B_OK = 0,
    PR_REC_290B_ARGUMENT,
    PR_REC_290B_BOUNDARY
} pr_rec_290b_status;

/* No caller argument slots are read; the body ends in plain RETF. A missing
 * or failing callback stops when reached and preserves prior host effects.
 * This model does not claim the callee-managed string contents or AX return. */
pr_rec_290b_status pr_rec_res_00ba_290b(const pr_rec_290b_io *io);

#endif
