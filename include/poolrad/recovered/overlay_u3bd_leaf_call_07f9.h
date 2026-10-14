#ifndef POOLRAD_RECOVERED_OVERLAY_U3BD_LEAF_CALL_07F9_H
#define POOLRAD_RECOVERED_OVERLAY_U3BD_LEAF_CALL_07F9_H

#include "poolrad/recovered/overlay_u3bd_loop_reset_07c7.h"

typedef pr_u3bd_loop_reset_frame pr_u3bd_leaf_call_frame;

/* Executes the entry-5 caller at loaded 2D14:+07F9, including its local
 * +031F in-grid leaf, and stops before the JNE at +080A. This is a bounded
 * state transfer, not a full entry-5 translation. RAM is 1 MiB of real-mode
 * address space. Invalid host arguments have no effect. */
int pr_u3bd_leaf_call_07f9(uint8_t *ram, size_t ram_size,
                            pr_u3bd_leaf_call_frame *frame, uint16_t *ip);

#endif
