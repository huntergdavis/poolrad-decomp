#ifndef POOLRAD_RECOVERED_RESIDENT_CHARACTER_FIELD_ZERO_H
#define POOLRAD_RECOVERED_RESIDENT_CHARACTER_FIELD_ZERO_H

#include "poolrad/recovered/resident_sidecar_remove.h"

#include <stdint.h>

/* START.EXE RES_00BA_23B7 tests one raw character byte. Status values
 * describe host adapter outcomes; the DOS routine returns only AL. */
typedef enum {
    PR_REC_CHARACTER_FIELD_ZERO_OK = 0,
    PR_REC_CHARACTER_FIELD_ZERO_ARGUMENT,
    PR_REC_CHARACTER_FIELD_ZERO_MEMORY
} pr_rec_character_field_zero_status;

typedef struct {
    void *context;
    /* Read the exact ES:DI byte. Return 1 on success, 0 if the host cannot
     * model the access. The 16-bit field offset may wrap. */
    int (*read_far_byte)(void *context, uint16_t segment, uint16_t offset,
                         uint8_t *value);
} pr_rec_character_field_zero_io;

/* Character = [BP+6,+8]. Read character byte +0x10E once; set *is_zero to
 * AL=1 for zero and AL=0 for nonzero. The field's gameplay meaning remains
 * unknown. On host failure, leave *is_zero unchanged. */
pr_rec_character_field_zero_status pr_rec_res_00ba_23b7(
    const pr_rec_character_field_zero_io *io, pr_rec_far_ptr character,
    uint8_t *is_zero);

#endif
