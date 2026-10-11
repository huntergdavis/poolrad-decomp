#ifndef POOLRAD_RECOVERED_RESIDENT_CHARACTER_TARGET_RESET_H
#define POOLRAD_RECOVERED_RESIDENT_CHARACTER_TARGET_RESET_H

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    uint16_t offset;
    uint16_t segment;
} pr_rec_reset_far_ptr;

typedef struct {
    void *context;
    bool (*read_far_word)(void *context, uint16_t segment, uint16_t offset,
                          uint16_t *value);
    bool (*write_far_byte)(void *context, uint16_t segment, uint16_t offset,
                           uint8_t value);
} pr_rec_target_reset_io;

typedef enum {
    PR_REC_TARGET_RESET_OK = 0,
    PR_REC_TARGET_RESET_ARGUMENT,
    PR_REC_TARGET_RESET_MEMORY
} pr_rec_target_reset_status;

/* START.EXE RES_00BA_2637: follow character +0x108 separately before
 * each ordered zero write at target +3, +0, +7, +6. On success AL becomes
 * one and AH retains its input value. Host callback failure stops after
 * preceding writes and leaves AX unchanged. */
pr_rec_target_reset_status pr_rec_res_00ba_2637(
    const pr_rec_target_reset_io *io, pr_rec_reset_far_ptr character,
    uint16_t *ax);

#endif
