#ifndef POOLRAD_RECOVERED_OVERLAY_U3BD_POST_UP_0DAE_OUTER_RETURN_H
#define POOLRAD_RECOVERED_OVERLAY_U3BD_POST_UP_0DAE_OUTER_RETURN_H

#include <stddef.h>
#include <stdint.h>

/* Executes +0DAE MOV SP,BP; +0DB0 POP BP; +0DB1 RETF 6.
 * The destination is read from the reached guest stack. */
typedef struct {
    uint8_t *ram;
    size_t ram_size;
    uint16_t cs, ss, bp, sp, ip, ax, di, flags;
} pr_u3bd_post_up_0dae_state;

typedef enum {
    PR_U3BD_POST_UP_0DAE_OK = 0,
    PR_U3BD_POST_UP_0DAE_ARGUMENT,
    PR_U3BD_POST_UP_0DAE_BOUNDARY
} pr_u3bd_post_up_0dae_status;

pr_u3bd_post_up_0dae_status pr_u3bd_post_up_0dae_outer_return(
    pr_u3bd_post_up_0dae_state *state);

#endif
