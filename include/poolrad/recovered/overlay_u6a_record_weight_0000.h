#ifndef POOLRAD_RECOVERED_OVERLAY_U6A_RECORD_WEIGHT_0000_H
#define POOLRAD_RECOVERED_OVERLAY_U6A_RECORD_WEIGHT_0000_H

#include <stddef.h>
#include <stdint.h>

#define PR_U6A_0000_RAM_SIZE (1u << 20)

/* 14-word raw CPU state shared with the unit 0x86 +323D predicate model. */
typedef struct {
    uint16_t ax, bx, cx, dx, si, di;
    uint16_t bp, sp, cs, ip, ds, es, ss, flags;
} pr_u6a_0000_cpu;

typedef enum {
    PR_U6A_0000_OK = 0,
    PR_U6A_0000_ARGUMENT,   /* null RAM/CPU or RAM below 1 MiB; nothing changed */
    PR_U6A_0000_TRANSFER    /* the callee's RETF frame no longer returned to +008F */
} pr_u6a_0000_status;

/* Complete GAME.OVR unit 0x6A:+0000..+0158 (345 bytes, 121 instructions),
 * entered after the far CALL: SS:[SP] return IP, [SP+2] return CS,
 * [SP+4] record offset, [SP+6] record segment; RETF 4.
 * Pass one scans record bytes +17..+2B: each byte above 7F indexes the
 * DS table at 31FB (stride 16, low seven bits) and the looked-up byte
 * raises the first unsigned maximum and the first 8-bit sum. Pass two
 * walks the far-linked list at record +C8/+CA through node links +2A/+2C;
 * for every node the far call at +008A (stub 0096:003E, unit 0x86 public
 * entry 6 +323D, the merged pr_u86_record_predicate_323d) gates the same
 * table scan over node bytes +3C..+3E into the second maximum and sum.
 * A tier byte of 0, 4 (either sum nonzero) or 6 (either maximum above 2)
 * is stored at record +2C, and AX returns
 * 15*sum2 + 15*sum1 + 60*tier (16-bit), with BX = 15*sum2, CX = 15*sum1,
 * DX = the high word of the last product, ES:DI = the record pointer and
 * the flags of the final ADD AX,BX. Every operand is reread from RAM in
 * source order. A cyclic list does not return, as in DOS. The entry
 * cpu->ip is the body's load offset (0 in the game); the far call pushes
 * the return offset relative to it. */
pr_u6a_0000_status pr_u6a_record_weight_0000(
    uint8_t *ram, size_t ram_size, pr_u6a_0000_cpu *cpu);

#endif
