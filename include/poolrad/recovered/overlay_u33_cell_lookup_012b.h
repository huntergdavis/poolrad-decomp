#ifndef POOLRAD_RECOVERED_OVERLAY_U33_CELL_LOOKUP_012B_H
#define POOLRAD_RECOVERED_OVERLAY_U33_CELL_LOOKUP_012B_H

#include <stddef.h>
#include <stdint.h>

#define PR_U33_012B_RAM_SIZE (1u << 20)

/* 14-word raw CPU state shared with the unit 0x3BD lookup models. */
typedef struct {
    uint16_t ax, bx, cx, dx, si, di;
    uint16_t bp, sp, cs, ip, ds, es, ss, flags;
} pr_u33_012b_cpu;

typedef enum {
    PR_U33_012B_OK = 0,
    PR_U33_012B_ARGUMENT,   /* null RAM/CPU or RAM below 1 MiB; nothing changed */
    PR_U33_012B_CODE,       /* a far-call site or its stub is not the loaded-state original */
    PR_U33_012B_TRANSFER    /* a callee's RETF frame no longer returned to its call site */
} pr_u33_012b_status;

/* Complete GAME.OVR unit 0x33:+012B..+01AC (130 bytes, 49 instructions),
 * entered after the far CALL: SS:[SP] return IP, [SP+2] return CS, [SP+4]
 * an unused word (the caller's BP), [SP+6] facing, [SP+8] y and [SP+A] x,
 * each read as its low byte; RETF 8. With x and y both in 0..15 (signed
 * byte compares) it calls unit 0x3BD entry 3 (+0350, the two-bit field)
 * at +0155 through stub 03D5:002F: a zero field returns 1; otherwise it
 * calls entry 2 (+047B, the nibble) at +0170 through stub 03D5:002A: a
 * zero nibble returns 0 and any other returns 3. Out of range, it returns
 * 0 when y equals DS:6AAE and facing is 2 or 6, else 1.
 * The two far calls are modelled as the loaded-state transfer: the
 * relocated segment word is read from the call site in RAM at
 * cs:(ip + 2D) and cs:(ip + 48), the five-byte far jump at that
 * paragraph's offset 2F or 2A supplies the callee CS:IP, the folded
 * full-state entry 3 / entry 2 models run on the same RAM and CPU words,
 * and each RETF frame is checked. cpu->ip is the body's load offset
 * (0x012B in the game). Every operand is reread from RAM in source order. */
pr_u33_012b_status pr_u33_cell_lookup_012b(
    uint8_t *ram, size_t ram_size, pr_u33_012b_cpu *cpu);

#endif
