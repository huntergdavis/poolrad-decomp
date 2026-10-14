#ifndef POOLRAD_RECOVERED_OVERLAY_U3BD_JNE_080A_H
#define POOLRAD_RECOVERED_OVERLAY_U3BD_JNE_080A_H

#include "poolrad/recovered/overlay_u3bd_loop_reset_07c7.h"

typedef pr_u3bd_loop_reset_frame pr_u3bd_jne_080a_frame;

typedef enum {
    PR_U3BD_080A_INVALID = 0,
    PR_U3BD_080A_PRECALL_0211,
    PR_U3BD_080A_CALLEE_0211,
    PR_U3BD_080A_CALLEE_047B,
    PR_U3BD_080A_BRANCH_0865,
    PR_U3BD_080A_FIRST_CALLEE_0211
} pr_u3bd_080a_result;

/* Execute from loaded 2D14:+080A until the next call entry or +0865.
 * With stop_before_second_call=1, the saved +0847 arm stops at +0860
 * before its CALL; otherwise it stops at +0211 after pushing return +0863.
 * Other call arms stop at their callee entry. No callee body is executed.
 * The caller supplies 1 MiB real-mode RAM; invalid host inputs have no effects. */
pr_u3bd_080a_result pr_u3bd_jne_080a(uint8_t *ram, size_t ram_size,
                                       pr_u3bd_jne_080a_frame *frame,
                                       uint16_t *ip,
                                       int stop_before_second_call);

#endif
