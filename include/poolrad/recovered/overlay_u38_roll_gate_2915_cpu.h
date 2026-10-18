#ifndef POOLRAD_RECOVERED_OVERLAY_U38_ROLL_GATE_2915_CPU_H
#define POOLRAD_RECOVERED_OVERLAY_U38_ROLL_GATE_2915_CPU_H

#include <stddef.h>
#include <stdint.h>

#define PR_U38R_RAM_SIZE (1u << 20)

typedef struct {
    uint16_t ax, bx, cx, dx, si, di;
    uint16_t bp, sp, cs, ip, ds, es, ss, flags;
} pr_u38r_cpu;

typedef enum {
    PR_U38R_OK = 0,
    PR_U38R_ARGUMENT,  /* null RAM/CPU or RAM that is not exactly 1 MiB; nothing changed */
    PR_U38R_CODE,      /* a reached far-call site, or the 00B0:0048 stub it names, is not in its loaded form */
    PR_U38R_CALLEE,    /* the composed U9E +0DE3 chain stopped; *callee_status holds its status */
    PR_U38R_TRANSFER   /* a composed callee returned to a CS:IP other than its call's next instruction */
} pr_u38r_status;

/* Raw-CPU, full-state models of four complete GAME.OVR unit 0x38 routines,
 * composing the merged raw-CPU RES_00BA_26C2 (#920), the merged U9E entry-8
 * dice chain (through the loaded stub 00B0:0048, with max_system_calls) and
 * the merged U38 +0000 helper (#912, through PUSH AX; PUSH CS; CALL +0000).
 * roll(n, s) is MOV AL,n; PUSH AX; MOV AL,s; PUSH AX; LCALL 00B0:0048.
 *
 * +2915 (97 bytes, 42 instructions), entered after PUSH AX; PUSH CS;
 * CALL +2915 (a far frame with the key word whose low byte is b): r :=
 * RES_00BA_26C2(DS:681B) into [BP-1]; [BP-2] := low byte of
 * b - 5 * (11 - r) (16-bit arithmetic; SI := 11 - r, DX := 5 * SI). Unless
 * DS:6817 is 0 and DS:6819 bit 3 is clear, roll(1, 64h) and, if AL is not
 * above [BP-2], helper(0). RETF 2. Final flags: OR AL,AL's (the skipped
 * roll), CMP AL,[BP-2]'s, or the helper's.
 * Entry 89 (+2976) and entry 90 (+2984), 14 bytes / 8 instructions each:
 * PUSH 32h (resp. 0Fh) as AL; PUSH CS; CALL +2915; POP BP; RETF 0Ah.
 * Entry 91 (+2992, 36 bytes / 19 instructions): roll(1, 64h); unless AL is
 * above 5Ah, helper(35h) then helper(0Bh); RETF 0Ah.
 *
 * Every far call is read from RAM at its site (9A, offset, segment); CS
 * and the next IP relative to the entry IP are pushed, and each callee
 * must return there. Every operand is reread from RAM in source order;
 * offsets wrap at 16 bits, physical addresses at 1 MiB; LES pairs and the
 * RETF frame are read physically contiguous. *callee_status (may be NULL)
 * receives the last composed model's status, or -1 when none ran. */
pr_u38r_status pr_u38_roll_gate_2915_cpu(uint8_t *ram, size_t ram_size, pr_u38r_cpu *cpu, unsigned max_system_calls,
                                         int *callee_status);
pr_u38r_status pr_u38_entry89_2976_cpu(uint8_t *ram, size_t ram_size, pr_u38r_cpu *cpu, unsigned max_system_calls,
                                       int *callee_status);
pr_u38r_status pr_u38_entry90_2984_cpu(uint8_t *ram, size_t ram_size, pr_u38r_cpu *cpu, unsigned max_system_calls,
                                       int *callee_status);
pr_u38r_status pr_u38_entry91_2992_cpu(uint8_t *ram, size_t ram_size, pr_u38r_cpu *cpu, unsigned max_system_calls,
                                       int *callee_status);

#endif
