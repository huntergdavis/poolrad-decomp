#ifndef POOLRAD_RECOVERED_RESIDENT_464_DIRTY_TRANSFER_H
#define POOLRAD_RECOVERED_RESIDENT_464_DIRTY_TRANSFER_H

#include <stddef.h>
#include <stdint.h>

#define PR_R464_DIRTY_ROWS 168u

typedef struct {
    uint8_t dirty;
    uint16_t first, last;       /* interleaved object data indices */
    uint16_t low, stage_base;   /* DS:[4AD0] and [5050+(row+8)*2] */
} pr_r464_dirty_row;

typedef struct {
    pr_r464_dirty_row row[PR_R464_DIRTY_ROWS];
    uint16_t page_base;         /* DS:[5060] */
    uint8_t map_mask[4];       /* DS:[25F6..25F9] */
    uint8_t gc_set_reset, gc_enable_set_reset, gc_rotate;
    uint8_t gc_mode, gc_bit_mask; /* pre-flush GC registers 0,1,3,5,8 */
    uint16_t vram_origin;       /* segment-relative start of each plane */
    size_t vram_size;
    uint8_t *staging[4];       /* separate pre-flush A200 plane buffers */
    uint8_t *page[4];          /* separate pre-flush A000 plane buffers */
} pr_r464_dirty_state;

typedef struct {
    size_t dirty_rows;
    size_t transfer_bytes;      /* sum of dirty-row byte lengths */
    size_t stage_plane_writes;
    size_t page_plane_writes;
} pr_r464_dirty_report;

typedef enum {
    PR_R464_DIRTY_OK = 0,
    PR_R464_DIRTY_BAD_ARG,
    PR_R464_DIRTY_UNSUPPORTED,
    PR_R464_DIRTY_SHORT_OBJECT,
    PR_R464_DIRTY_SHORT_VRAM
} pr_r464_dirty_result;

/* Bounded video-state-1 path of RES_0464:1B52. Preflight host mappings
 * first. On success, stage every dirty row, then perform all latch-backed
 * Move calls, then reset dirty flags and first/last extents for all rows.
 * On a host stop, state, plane data and report remain unchanged. Other
 * video modes and invalid DOS pointer/fault effects are outside scope. */
pr_r464_dirty_result pr_r464_dirty_transfer(
    pr_r464_dirty_state *state, const uint8_t *object, size_t object_size,
    pr_r464_dirty_report *report);

#endif
