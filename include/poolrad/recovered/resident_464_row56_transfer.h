#ifndef POOLRAD_RECOVERED_RESIDENT_464_ROW56_TRANSFER_H
#define POOLRAD_RECOVERED_RESIDENT_464_ROW56_TRANSFER_H

#include <stddef.h>
#include <stdint.h>

/* Reached RES_0464:1B52 video-state-1 row 56 only. The arrays represent
 * A200/A000 at the calculated eleven-byte span, one byte per EGA plane and
 * column. They are inputs and mutable outputs. See the address-only note. */
#define PR_R464_ROW56_BYTES 11u

typedef struct {
    uint8_t plane[4][PR_R464_ROW56_BYTES];
} pr_r464_row56_planes;

typedef struct {
    uint8_t dirty;
    uint16_t first, last;       /* DS row-56 interleaved object indices */
    uint16_t row_base, low;     /* DS row offset and low display offset */
    uint16_t page_base;         /* DS:[5060] */
    uint8_t map_mask[4];       /* DS:[25F6..25F9] */
    uint8_t gc_set_reset, gc_enable_set_reset, gc_rotate;
    uint8_t gc_mode, gc_bit_mask; /* pre-flush GC registers 0,1,3,5,8 */
    pr_r464_row56_planes staging, page;
} pr_r464_row56_state;

typedef enum {
    PR_R464_ROW56_OK = 0,
    PR_R464_ROW56_BAD_ARG,
    PR_R464_ROW56_UNSUPPORTED,
    PR_R464_ROW56_SHORT_OBJECT
} pr_r464_row56_result;

/* Object points at its DOS header. On success, output_offset is the
 * segment-relative A200/A000 offset. A host stop leaves state unchanged.
 * The model requires the reached mode-0 staging register state and the
 * exact row-56 span. It does not translate the other dirty rows. */
pr_r464_row56_result pr_r464_row56_transfer(
    pr_r464_row56_state *state, const uint8_t *object, size_t object_size,
    uint16_t *output_offset);

#endif
