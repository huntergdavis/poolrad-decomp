#ifndef POOLRAD_RECOVERED_OVERLAY_U3BD_POST_UP_05FB_WRAPPER_RETURN_H
#define POOLRAD_RECOVERED_OVERLAY_U3BD_POST_UP_05FB_WRAPPER_RETURN_H

#include <stddef.h>
#include <stdint.h>

/* Reached only after the external resident display transfer returns.
 * Executes +05FB POP BP; +05FC RETF and stops at the caller's +0DAE. */
typedef struct {
    uint8_t *ram;
    size_t ram_size;
    uint16_t cs, ss, bp, sp, ip, ax, di, flags;
} pr_u3bd_post_up_05fb_state;

typedef enum {
    PR_U3BD_POST_UP_05FB_OK = 0,
    PR_U3BD_POST_UP_05FB_ARGUMENT,
    PR_U3BD_POST_UP_05FB_BOUNDARY
} pr_u3bd_post_up_05fb_status;

pr_u3bd_post_up_05fb_status pr_u3bd_post_up_05fb_wrapper_return(
    pr_u3bd_post_up_05fb_state *state);

#endif
