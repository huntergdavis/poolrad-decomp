#ifndef POOLRAD_RECOVERED_OVERLAY_U3BD_POST_UP_026E_ARM_H
#define POOLRAD_RECOVERED_OVERLAY_U3BD_POST_UP_026E_ARM_H

#include <stddef.h>
#include <stdint.h>

/* Unit 0x3BD:+026E..+0288, entered after an unequal outer-loop test.
 * ss represents one 16-bit guest stack segment. */
typedef struct {
    uint8_t *ss;
    size_t ss_size;
    uint16_t bp;
    uint16_t ax;
    uint16_t flags;
} pr_u3bd_post_up_026e_arm_state;

typedef struct {
    uint16_t outer_before;
    uint16_t outer_after;
    uint16_t copied_word;
    uint8_t source_byte;
    uint16_t compared_ax;
    uint16_t next_ip; /* +028B for signed <=, +030E otherwise. */
} pr_u3bd_post_up_026e_arm_report;

typedef enum {
    PR_U3BD_POST_UP_026E_ARM_OK = 0,
    PR_U3BD_POST_UP_026E_ARM_ARGUMENT,
    PR_U3BD_POST_UP_026E_ARM_BOUNDARY
} pr_u3bd_post_up_026e_arm_status;

pr_u3bd_post_up_026e_arm_status pr_u3bd_post_up_026e_arm(
    pr_u3bd_post_up_026e_arm_state *state,
    pr_u3bd_post_up_026e_arm_report *report);

#endif
