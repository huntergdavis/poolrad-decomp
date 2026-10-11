#ifndef POOLRAD_RECOVERED_RESIDENT_CHARACTER_TARGET_SELECT_H
#define POOLRAD_RECOVERED_RESIDENT_CHARACTER_TARGET_SELECT_H

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    uint16_t offset;
    uint16_t segment;
} pr_rec_select_far_ptr;

typedef struct {
    void *context;
    bool (*read_far_word)(void *context, uint16_t segment, uint16_t offset,
                          uint16_t *value);
    bool (*read_far_byte)(void *context, uint16_t segment, uint16_t offset,
                          uint8_t *value);
    bool (*read_global_byte)(void *context, uint16_t offset, uint8_t *value);
    bool (*write_far_word)(void *context, uint16_t segment, uint16_t offset,
                           uint16_t value);
} pr_rec_target_select_io;

typedef enum {
    PR_REC_TARGET_SELECT_OK = 0,
    PR_REC_TARGET_SELECT_ARGUMENT,
    PR_REC_TARGET_SELECT_MEMORY
} pr_rec_target_select_status;

/* START.EXE RES_00BA_2E8B. The four-byte output slot is cleared before
 * inspecting the character link. Each output-word write is observable.
 * On success AX contains the original routine's result, including AH. */
pr_rec_target_select_status pr_rec_res_00ba_2e8b(
    const pr_rec_target_select_io *io, pr_rec_select_far_ptr character,
    pr_rec_select_far_ptr output_slot, uint16_t *ax);

#endif
