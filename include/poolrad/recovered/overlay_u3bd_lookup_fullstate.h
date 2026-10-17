#ifndef POOLRAD_RECOVERED_OVERLAY_U3BD_LOOKUP_FULLSTATE_H
#define POOLRAD_RECOVERED_OVERLAY_U3BD_LOOKUP_FULLSTATE_H

#include <stddef.h>
#include <stdint.h>

#define PR_U3BD_LOOKUP_RAM_SIZE (1u << 20)

/* 14-word raw CPU state shared with the merged +031F leaf model. */
typedef struct {
    uint16_t ax, bx, cx, dx, si, di;
    uint16_t bp, sp, cs, ip, ds, es, ss, flags;
} pr_u3bd_lookup_cpu;

typedef enum {
    PR_U3BD_LOOKUP_OK = 0,
    PR_U3BD_LOOKUP_ARGUMENT,   /* null RAM/CPU or RAM below 1 MiB; nothing changed */
    PR_U3BD_LOOKUP_TRANSFER    /* an internal callee's RETF frame no longer returned to its call site */
} pr_u3bd_lookup_status;

/* Full-state (1 MiB A20-off RAM, 14 CPU words) models of two already
 * strict GAME.OVR unit 0x3BD originals, folded from their original bytes
 * so that unit 0x33 callers can compose them on raw RAM. They add no new
 * strict credit; the typed translations overlay_u3bd_nibble.c and
 * overlay_u3bd_field.c remain the reviewed behavioural notes.
 *
 * Both enter after the far CALL with SS:[SP] return IP, [SP+2] return CS,
 * [SP+4] selector, [SP+6] y and [SP+8] x (each read as its low byte) and
 * leave with RETF 6. cpu->ip is the body's load offset (0x047B / 0x0350
 * in the game); internal PUSH CS / CALL return offsets follow it, and the
 * +0350 body enters +047B at ip + 0x12B. Every operand is reread from RAM
 * in source order, including the unwritten [BP-1] local that an
 * unrecognised selector returns. */

/* Entry 2, +047B..+0579 (255 bytes, 103 instructions): one near call to
 * the +031F range leaf; the map-id gate on DS:84DC; 0..15 wrap of the
 * stack arguments; selector 0/2/4/6 nibble reads through far DS:6A5C. */
pr_u3bd_lookup_status pr_u3bd_nibble_047b_fullstate(
    uint8_t *ram, size_t ram_size, pr_u3bd_lookup_cpu *cpu);

/* Entry 3, +0350..+047A (299 bytes, 120 instructions): the same gate and
 * wrap, a default result of 1, a near call to +047B with the wrapped
 * arguments, then selector 6/4/2/0 two-bit fields of the fourth quarter. */
pr_u3bd_lookup_status pr_u3bd_field_0350_fullstate(
    uint8_t *ram, size_t ram_size, pr_u3bd_lookup_cpu *cpu);

#endif
