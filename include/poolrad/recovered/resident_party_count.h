#ifndef POOLRAD_RECOVERED_RESIDENT_PARTY_COUNT_H
#define POOLRAD_RECOVERED_RESIDENT_PARTY_COUNT_H

#include "poolrad/recovered/resident_sidecar_remove.h"

#include <stddef.h>
#include <stdint.h>

/* START.EXE RES_00BA_23DB traverses the character party list and counts
 * records by two raw byte fields. These are host adapter outcomes; the DOS
 * routine returns with RETF and does not define a result value. */
typedef enum {
    PR_REC_PARTY_COUNT_OK = 0,
    PR_REC_PARTY_COUNT_ARGUMENT,
    PR_REC_PARTY_COUNT_MEMORY,
    PR_REC_PARTY_COUNT_LIMIT
} pr_rec_party_count_status;

typedef struct {
    void *context;
    /* DS-global accesses at exact 16-bit offsets. The routine initializes
     * 0x6814 and 0x6815, reads the party head at 0x5D96/0x5D98, and may
     * increment 0x6814 + a signed byte index outside the initialized pair. */
    int (*read_global_word)(void *context, uint16_t offset, uint16_t *value);
    int (*read_global_byte)(void *context, uint16_t offset, uint8_t *value);
    int (*write_global_byte)(void *context, uint16_t offset, uint8_t value);
    /* ES:DI character-field reads. Field offsets use 16-bit wraparound. */
    int (*read_far_word)(void *context, uint16_t segment, uint16_t offset,
                         uint16_t *value);
    int (*read_far_byte)(void *context, uint16_t segment, uint16_t offset,
                         uint8_t *value);
    /* Host guard for corrupt cycles; the DOS routine has no node bound. */
    size_t max_nodes;
} pr_rec_party_count_io;

/* Head = DS:0x5D96/0x5D98; next = character +0x104/+0x106. A nonzero
 * character byte +0x10D selects an increment at DS:0x6814 plus the signed
 * byte at +0x10E. The meaning of those fields remains unassigned. */
pr_rec_party_count_status pr_rec_res_00ba_23db(const pr_rec_party_count_io *io);

#endif
