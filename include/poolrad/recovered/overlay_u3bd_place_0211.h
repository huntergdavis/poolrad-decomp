#ifndef POOLRAD_RECOVERED_OVERLAY_U3BD_PLACE_0211_H
#define POOLRAD_RECOVERED_OVERLAY_U3BD_PLACE_0211_H

#include <stddef.h>
#include <stdint.h>

#define PR_U3BD_0211_RAM_SIZE (1u << 20)

typedef struct {
    uint16_t ax, bx, cx, dx, si, di;
    uint16_t bp, sp, cs, ip, ds, es, ss, flags;
} pr_u3bd_0211_cpu;

typedef enum {
    PR_U3BD_0211_OK = 0,
    PR_U3BD_0211_ARGUMENT,
    PR_U3BD_0211_CALLEE_REQUIRED,
    PR_U3BD_0211_CALLEE_FAILED,
    PR_U3BD_0211_CALLEE_TRANSFER
} pr_u3bd_0211_status;

/* At +02FB the model has pushed the far return IP/CS over five argument
 * words and set CPU CS:IP to the loaded call target. The callback executes
 * the external tile callee, including its own return and argument cleanup.
 * It may change any CPU register or RAM byte. Return zero on success. */
typedef int (*pr_u3bd_0211_tile_call)(uint8_t *ram, size_t ram_size,
                                      pr_u3bd_0211_cpu *cpu, void *context);

/* Translate the complete +0211..+031E helper around its sole external call.
 * Entry CPU points at +0211 after the caller's PUSH CS; CALL near frame.
 * The helper reads its four argument low bytes at SS:[BP+6,+8,+A,+C],
 * rereads locals and table data in source order, and applies a live RETF 8.
 * The two far-return words read contiguously across an SS offset boundary;
 * SP and the byte piece cursor wrap. A20 is disabled.
 * Loaded code bytes must remain immutable. A callback is needed only on a
 * positive unclipped tile. Nonreturning or aliased callee transfers stop at
 * the callback boundary with the callback's CPU/RAM state intact. */
pr_u3bd_0211_status pr_u3bd_place_0211(
    uint8_t *ram, size_t ram_size, pr_u3bd_0211_cpu *cpu,
    pr_u3bd_0211_tile_call tile_call, void *context);

#endif
