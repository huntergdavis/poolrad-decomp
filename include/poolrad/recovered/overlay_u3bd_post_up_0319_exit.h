#ifndef POOLRAD_RECOVERED_OVERLAY_U3BD_POST_UP_0319_EXIT_H
#define POOLRAD_RECOVERED_OVERLAY_U3BD_POST_UP_0319_EXIT_H

#include <stddef.h>
#include <stdint.h>

/* Original unit 0x3BD:+0319..+031F. ss is a single 16-bit segment
 * window; word reads wrap at the segment boundary. */
typedef struct {
    const uint8_t *ss;
    size_t ss_size;
    uint16_t bp, sp, cs, ip;
    uint16_t ax, flags;
} pr_u3bd_post_up_0319_exit_state;

typedef struct {
    uint16_t saved_bp;
    uint16_t return_ip, return_cs;
    uint16_t exit_sp;
} pr_u3bd_post_up_0319_exit_report;

typedef enum {
    PR_U3BD_POST_UP_0319_EXIT_OK = 0,
    PR_U3BD_POST_UP_0319_EXIT_ARGUMENT,
    PR_U3BD_POST_UP_0319_EXIT_BOUNDARY
} pr_u3bd_post_up_0319_exit_status;

pr_u3bd_post_up_0319_exit_status pr_u3bd_post_up_0319_exit(
    pr_u3bd_post_up_0319_exit_state *state,
    pr_u3bd_post_up_0319_exit_report *report);

#endif
