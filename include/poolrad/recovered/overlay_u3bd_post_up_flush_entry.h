#ifndef POOLRAD_RECOVERED_OVERLAY_U3BD_POST_UP_FLUSH_ENTRY_H
#define POOLRAD_RECOVERED_OVERLAY_U3BD_POST_UP_FLUSH_ENTRY_H

#include <stddef.h>
#include <stdint.h>

/* Reached +0DAA -> +05F3 wrapper -> relocated far target. Stops before
 * executing the resident transfer at the loaded target CS:IP. */
typedef struct {
    uint8_t *ram;
    size_t ram_size;
    uint16_t cs, ss, bp, sp, ip, ax, flags;
} pr_u3bd_post_up_flush_entry_state;

typedef struct {
    uint16_t near_return_ip, far_return_ip;
    uint16_t target_cs, target_ip;
} pr_u3bd_post_up_flush_entry_report;

typedef enum {
    PR_U3BD_POST_UP_FLUSH_ENTRY_OK = 0,
    PR_U3BD_POST_UP_FLUSH_ENTRY_ARGUMENT,
    PR_U3BD_POST_UP_FLUSH_ENTRY_BOUNDARY
} pr_u3bd_post_up_flush_entry_status;

pr_u3bd_post_up_flush_entry_status pr_u3bd_post_up_flush_entry(
    pr_u3bd_post_up_flush_entry_state *state,
    pr_u3bd_post_up_flush_entry_report *report);

#endif
