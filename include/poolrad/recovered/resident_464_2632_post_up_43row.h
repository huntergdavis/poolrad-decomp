#ifndef POOLRAD_RECOVERED_RESIDENT_464_2632_POST_UP_43ROW_H
#define POOLRAD_RECOVERED_RESIDENT_464_2632_POST_UP_43ROW_H

#include <stddef.h>
#include <stdint.h>

enum { PR_R464_2632_POST_UP_RAM_BYTES = 0xA0000u };

typedef struct {
    uint16_t cs, ip, ss, sp, ds;
} pr_r464_2632_post_up_frame;

typedef struct {
    uint16_t first_row, last_row;
    size_t offscreen_value_changes;
} pr_r464_2632_post_up_result;

typedef enum {
    PR_R464_2632_POST_UP_OK = 0,
    PR_R464_2632_POST_UP_ARGUMENT = 1,
    PR_R464_2632_POST_UP_BOUNDARY = 2
} pr_r464_2632_post_up_status;

/* Bounded page-state effect of the saved slot-A one-Up call at 2D14:0198,
 * returning at 2D14:019D. Reads its 18-byte caller frame from SS:SP.
 * Covers only the observed 43-row, mode-one, no-clipping path of resident
 * 0C88:2632. It writes the offscreen object, dirty/extent arrays, and loop
 * counter. CPU registers and runtime stack residue are outside this model.
 * Unsupported shapes have no effect on RAM. */
pr_r464_2632_post_up_status pr_r464_2632_post_up_43row(
    uint8_t *ram, size_t ram_size, const pr_r464_2632_post_up_frame *frame,
    pr_r464_2632_post_up_result *result);

#endif
