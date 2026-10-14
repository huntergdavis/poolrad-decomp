#ifndef POOLRAD_RECOVERED_OVERLAY_U3BD_LOOP_RESET_07C7_H
#define POOLRAD_RECOVERED_OVERLAY_U3BD_LOOP_RESET_07C7_H

#include <stddef.h>
#include <stdint.h>

#define PR_U3BD_LOOP_RESET_RAM_BYTES 0x100000u

typedef struct {
    uint16_t ax, bx, cx, dx, si, di, bp, sp;
    uint16_t ds, es, ss, cs, flags;
} pr_u3bd_loop_reset_frame;

/* Bounded U3BD 2D14:+07C7 local reset. Starts before MOV AX,[BP-5]
 * and stops at +07E9, the reached target of +07E4 JB. RAM is the
 * caller's 1 MiB real-mode address space; invalid host inputs have no
 * effects. This does not execute the successor or complete entry 5. */
int pr_u3bd_loop_reset_07c7(uint8_t *ram, size_t ram_size,
                             pr_u3bd_loop_reset_frame *frame,
                             uint16_t *ip);

#endif
