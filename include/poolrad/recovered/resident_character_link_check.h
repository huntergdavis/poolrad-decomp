#ifndef POOLRAD_RECOVERED_RESIDENT_CHARACTER_LINK_CHECK_H
#define POOLRAD_RECOVERED_RESIDENT_CHARACTER_LINK_CHECK_H

#include "poolrad/recovered/resident_sidecar_remove.h"

#include <stdint.h>

/* Host outcomes for START.EXE RES_00BA_2E03. DOS returns its Boolean in AL;
 * it has no status result for an invalid memory access. */
typedef enum {
    PR_REC_CHARACTER_LINK_CHECK_OK = 0,
    PR_REC_CHARACTER_LINK_CHECK_ARGUMENT,
    PR_REC_CHARACTER_LINK_CHECK_MEMORY
} pr_rec_character_link_check_status;

typedef struct {
    void *context;
    /* Every word/byte read is a separate observed DOS access. Offsets wrap
     * at 16 bits within the supplied segment. Return 1 on success. */
    int (*read_far_word)(void *context, uint16_t segment, uint16_t offset,
                         uint16_t *value);
    int (*read_far_byte)(void *context, uint16_t segment, uint16_t offset,
                         uint8_t *value);
    int (*read_global_byte)(void *context, uint16_t offset, uint8_t *value);
} pr_rec_character_link_check_io;

/* Character = [BP+6,+8]. If its +0xCC/+0xCE pointer is nonzero, re-read
 * that pointer, use linked byte +0x2E as a 16-byte DS-table index from
 * 0x558E, and return AL=1 iff the indexed byte is >1. The meanings of
 * the link and table remain raw. On host failure *result is unchanged. */
pr_rec_character_link_check_status pr_rec_res_00ba_2e03(
    const pr_rec_character_link_check_io *io, pr_rec_far_ptr character,
    uint8_t *result);

#endif
