#ifndef POOLRAD_RECOVERED_RESIDENT_464_MODE0_PREFIX_H
#define POOLRAD_RECOVERED_RESIDENT_464_MODE0_PREFIX_H

#include <stddef.h>
#include <stdint.h>

#include "poolrad/recovered/resident_464_block_wrapper.h"

/* RES_0464:02A5 raw video-state-0 route, first pixel only. A successful
 * prefix reaches +0x03D1, before the four +0x007B nibble-helper paths.
 * See docs/re/RESIDENT_464_MODE0_PREFIX.md. */
typedef enum {
    PR_R464_MODE0_READY_PRIMARY = 0,
    PR_R464_MODE0_NO_WORK,
    PR_R464_MODE0_ARGUMENT,
    PR_R464_MODE0_BOUNDS,
    PR_R464_MODE0_AUX_MAPPING,
    PR_R464_MODE0_TABLE
} pr_r464_mode0_status;

typedef enum {
    PR_R464_MODE0_STOP_NONE = 0,
    PR_R464_MODE0_STOP_HEADER,
    PR_R464_MODE0_STOP_MOVE,
    PR_R464_MODE0_STOP_AUX_TABLE,
    PR_R464_MODE0_STOP_AUX_READ,
    PR_R464_MODE0_STOP_PRIMARY_HELPERS,
    PR_R464_MODE0_STOP_RETURN
} pr_r464_mode0_stop;

typedef struct {
    uint16_t source_offset; /* DOS [BP+0x0E], unchanged in this prefix. */
    uint16_t data_offset;   /* DOS [BP-0x1C], initially zero. */
    uint16_t auxiliary_index; /* DOS [BP-0x1E], advances twice if present. */
    uint16_t next_offset;   /* +0x03D1 or common return +0x08CE. */
    uint16_t width_end, height_end; /* DOS decremented WORD loop bounds. */
    uint16_t x, y;         /* Both zero at the first-pixel boundary. */
    uint8_t layer;
    uint8_t count;         /* Entry copy of object +8, DOS [BP-0x29]. */
    uint8_t copied[4];     /* DOS local [BP-0x15..-0x12]. */
    uint8_t bitmask;
    unsigned move_calls, bitmask_calls, table_reads, aux_or_writes;
    pr_r464_mode0_stop stopped_at;
} pr_r464_mode0_report;

/* aux_pattern_low[0..3] supplies the low bytes of the four private DS
 * WORDs selected by indices 4..7 at +0x25E5. It is read only when the
 * matching bit is set. A nonzero far pointer in object +0x13/+0x15 is a
 * presence gate; auxiliary maps that separate far allocation from offset
 * zero. Host bounds stop at the first unavailable reached access and retain
 * earlier effects. The caller-provided arrays must not alias. */
pr_r464_mode0_status pr_r464_mode0_first_pixel_prefix(
    const uint8_t *source, size_t source_size, uint16_t source_offset,
    uint8_t raw_unused, uint8_t selector, pr_r464_block_object *object,
    pr_r464_block_object *auxiliary, const uint8_t *aux_pattern_low,
    size_t aux_pattern_size, pr_r464_mode0_report *report);

#endif
