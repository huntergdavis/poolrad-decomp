#ifndef POOLRAD_RECOVERED_RESIDENT_0709_RECT_FILL_00B7_H
#define POOLRAD_RECOVERED_RESIDENT_0709_RECT_FILL_00B7_H

#include <stddef.h>
#include <stdint.h>

typedef enum {
    PR_RES0709_RECT_OK = 0,
    PR_RES0709_RECT_ARGUMENT,
    PR_RES0709_RECT_BOUNDARY
} pr_res0709_rect_status;

/* Each plane begins at EGA A000:0000 and includes any selected 8192-byte page.
 * The register fields expose the state left by the reached +0000 and +00A0
 * local callees. Other EGA registers are outside this model. */
typedef struct {
    uint8_t *plane[4];
    size_t plane_bytes;
    uint8_t active_page;       /* DS:4A1A, read and preserved */
    uint16_t draw_segment;     /* DS:4A18, restored from active_page */
    uint8_t gc[9];
    uint8_t gc_index;
    uint8_t seq_index;
    uint8_t seq_map_mask;
} pr_res0709_rect_state;

typedef struct {
    uint8_t color;
    uint8_t page;
    uint8_t bottom;
    uint8_t right;
    uint8_t top;
    uint8_t left;
} pr_res0709_rect_args;

typedef struct {
    size_t plane_byte_stores;  /* C model targets; native store count unmeasured */
    size_t changed_plane_bytes;
    uint16_t selected_segment;
    uint16_t restored_segment;
} pr_res0709_rect_report;

/* Bounded semantics of complete RES_0709:00B7..0171 (186 bytes). Coordinates
 * must name an inclusive rectangle within the 40x25 display, and every EGA
 * plane must contain its complete target. Unsupported inputs return BOUNDARY
 * before any state or page mutation. Color uses its low EGA write-mode-2 nibble.
 * This models page and final register effects, not timed port transactions. */
pr_res0709_rect_status pr_res0709_rect_fill_00b7(
    pr_res0709_rect_state *state, pr_res0709_rect_args args,
    pr_res0709_rect_report *report);

#endif
