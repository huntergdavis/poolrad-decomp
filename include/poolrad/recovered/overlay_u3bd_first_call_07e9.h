#ifndef POOLRAD_RECOVERED_OVERLAY_U3BD_FIRST_CALL_07E9_H
#define POOLRAD_RECOVERED_OVERLAY_U3BD_FIRST_CALL_07E9_H

#include "poolrad/recovered/overlay_u3bd_loop_reset_07c7.h"

/* The adjacent +07C7 reset hands this same 13-field CPU frame to +07E9. */
typedef pr_u3bd_loop_reset_frame pr_u3bd_first_call_frame;

/* Bounded entry-5 caller transfer from loaded 2D14:+07E9 to the entry-2
 * callee at 2D14:+047B. Executes the near CALL at +07F6, including its
 * return-IP push. It does not execute any entry-2 instruction. RAM is the
 * caller's 1 MiB real-mode space; invalid host inputs have no effects. */
int pr_u3bd_first_call_07e9(uint8_t *ram, size_t ram_size,
                            pr_u3bd_first_call_frame *frame, uint16_t *ip);

#endif
