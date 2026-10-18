#ifndef POOLRAD_RECOVERED_OVERLAY_U86_ROLL_VALUE_07D6_H
#define POOLRAD_RECOVERED_OVERLAY_U86_ROLL_VALUE_07D6_H

#include <stddef.h>
#include <stdint.h>

#define PR_U86_07D6_RAM_SIZE (1u << 20)

typedef struct {
    uint16_t ax, bx, cx, dx, si, di;
    uint16_t bp, sp, cs, ip, ds, es, ss, flags;
} pr_u86_07d6_cpu;

typedef enum {
    PR_U86_07D6_OK = 0,
    PR_U86_07D6_ARGUMENT,  /* null RAM/CPU or RAM that is not exactly 1 MiB; nothing changed */
    PR_U86_07D6_CODE,      /* a reached far-call site, or the 00B0:0048 stub it names, is not in its loaded form */
    PR_U86_07D6_CALLEE,    /* the composed U9E +0DE3 chain stopped; *callee_status holds its status */
    PR_U86_07D6_TRANSFER   /* a composed callee returned to a CS:IP other than its call's next instruction */
} pr_u86_07d6_status;

/* Raw-CPU, full-state model of the complete GAME.OVR unit 0x86:+07D6..+08C2
 * (237 bytes, 99 instructions), entered after the far CALL with SS:[SP]
 * return IP, [SP+2] return CS and [SP+4] a word whose low byte k is the
 * argument; RETF 2. The result word [BP-4] is, for k =
 *   28h: 10 * roll(1, 6);       39h or 3Dh: roll(5, 4);
 *   3Bh: 10 * roll(1, 4) + 28h;
 *   3Fh: 10 * roll(2, 0Ah) when DS:49F3 is 5, else 10 * (roll(1, 0Ah) + 0Ah);
 *   43h: 5A0h;
 *   otherwise T[5] * RES_00BA_26C2(k) + T[4] (16-bit, with T the 16-byte
 *   DS table entry at 31FA + 16k, i.e. bytes 31FE/31FF + 16k).
 * roll(n, s) is unit 0x9E entry 8 (+0DE3) reached through the loaded stub
 * 00B0:0048 (PUSH n; PUSH s; LCALL) and run by the merged
 * pr_u9e_entry8_fullstate_compose_0de3 with max_system_calls; AL is its
 * result (XOR AH,AH). [BP-2] := [BP-4]; AX := [BP-2]. The final flags are
 * the last MUL's, ADD's, XOR AH,AH's or CMP's on the path. MUL r16 follows
 * the merged U3C6 +0CB9 model's pinned DOSBox-X profile: CF = OF = (DX !=
 * 0), ZF = (AX == 0), PF from all 32 product bits, SF and AF kept (SF, ZF,
 * AF and PF are architecturally undefined after MUL).
 *
 * Every far call is read from RAM at its site (9A, offset, segment); CS
 * and the next IP relative to the entry IP are pushed, and each callee
 * must return there. Every operand is reread from RAM in source order;
 * offsets wrap at 16 bits, physical addresses at 1 MiB; LES pairs and the
 * RETF frame are read physically contiguous. *callee_status (may be NULL)
 * receives the last composed model's status, or -1 when none ran. */
pr_u86_07d6_status pr_u86_roll_value_07d6(uint8_t *ram, size_t ram_size, pr_u86_07d6_cpu *cpu,
                                          unsigned max_system_calls, int *callee_status);

#endif
