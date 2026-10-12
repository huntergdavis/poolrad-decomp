#ifndef POOLRAD_RECOVERED_RESIDENT_464_MODE2_H
#define POOLRAD_RECOVERED_RESIDENT_464_MODE2_H

#include <stddef.h>
#include <stdint.h>

#include "poolrad/recovered/resident_464_block_wrapper.h"

/* Bounded model of RES_0464:02A5's video-state-2 route only. The entry
 * assumes a nonzero DOS destination pointer and video byte exactly 2.
 * Object bytes represent its far allocation from offset zero; auxiliary
 * bytes map its nonzero far pointer at header +0x13. See the evidence note. */
typedef enum {
    PR_R464_MODE2_OK = 0,
    PR_R464_MODE2_ARGUMENT,
    PR_R464_MODE2_BOUNDS,
    PR_R464_MODE2_AUX_MAPPING
} pr_r464_mode2_status;

typedef enum {
    PR_R464_MODE2_STOP_NONE = 0,
    PR_R464_MODE2_STOP_HEADER,
    PR_R464_MODE2_STOP_MOVE,
    PR_R464_MODE2_STOP_PRIMARY_READ,
    PR_R464_MODE2_STOP_AUX_WRITE,
    PR_R464_MODE2_STOP_AUX_READ
} pr_r464_mode2_stop;

typedef struct {
    uint16_t source_offset; /* DOS [BP+0x0E], initially 0x11 in wrapper. */
    uint16_t data_offset;   /* DOS [BP-0x1C], initialized to zero. */
    uint16_t mask_index;
    uint8_t layer;
    unsigned move_calls, mask_steps, aux_writes, primary_writes;
    pr_r464_mode2_stop stopped_at;
} pr_r464_mode2_report;

/* The raw unused [BP+0x0C] byte is retained in the signature. Host bounds
 * reject a read/write that would leave the supplied buffers or cross a
 * 16-bit segment boundary; these stops are not DOS error branches. Prior
 * copies and writes remain visible when a later reached site stops. */
pr_r464_mode2_status pr_r464_postprocess_mode2(
    const uint8_t *source, size_t source_size, uint16_t source_offset,
    uint8_t raw_unused, uint8_t selector, pr_r464_block_object *object,
    pr_r464_block_object *auxiliary, pr_r464_mode2_report *report);

#endif
