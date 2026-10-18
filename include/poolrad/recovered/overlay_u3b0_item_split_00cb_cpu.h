#ifndef POOLRAD_RECOVERED_OVERLAY_U3B0_ITEM_SPLIT_00CB_CPU_H
#define POOLRAD_RECOVERED_OVERLAY_U3B0_ITEM_SPLIT_00CB_CPU_H

#include <stddef.h>
#include <stdint.h>

#define PR_U3B0_00CB_RAM_SIZE (1u << 20)

typedef struct {
    uint16_t ax, bx, cx, dx, si, di;
    uint16_t bp, sp, cs, ip, ds, es, ss, flags;
} pr_u3b0_00cb_cpu;

typedef enum {
    PR_U3B0_00CB_OK = 0,
    PR_U3B0_00CB_ARGUMENT,  /* null RAM/CPU or RAM that is not exactly 1 MiB; nothing changed */
    PR_U3B0_00CB_CODE,      /* a reached LCALL is not xxxx:0B25, xxxx:16A8 or xxxx:0DC0 as expected; stopped before it, IP at the site */
    PR_U3B0_00CB_TRANSFER   /* a SYSTEM callee's RETF popped a CS:IP other than the call's next instruction */
} pr_u3b0_00cb_status;

/* Raw-CPU, full-state model of the complete GAME.OVR unit 0x3B0 routine
 * +00CB..+01B9 (239 bytes, 93 instructions), a nested Pascal procedure
 * entered after a far call with SS:[SP] return IP, [SP+2] return CS, [SP+4]
 * the static link L (the parent's BP) and [SP+6]/[SP+8] a far string
 * pointer S; RETF 6. After PUSH BP; MOV BP,SP; SUB SP,2Eh it copies S into
 * the local string[40] at BP-29h through the SYSTEM string store
 * (LCALL xxxx:0B25), clears the 40 bytes at SS:L-8Dh through FillChar
 * (LCALL xxxx:16A8, value 0), and sets k := 1 at [BP-2Ah], n := the copied
 * length at [BP-2Ch] and the FOR bound [BP-2Eh] := n. For i := 1 to n (i at
 * [BP-2Bh]; skipped when n = 0) it tests the local byte [BP+i-29h] against
 * the 32-byte set at CS:00AB through the SYSTEM set membership test
 * (LCALL xxxx:0DC0); for a member, with the start/end byte pairs at
 * SS:L-8Fh+2j and SS:L-8Eh+2j:
 *   if start[k] = 0, start[k] := i;
 *   else end[k] := i - 2, k := k + 1, start[k] := i.
 * Finally end[k] := n and L[-8Eh] := k (the count), then MOV SP,BP; POP BP;
 * RETF 6. Every operand is reread from RAM in source order, so a pair store
 * that aliases i, k, n or the bound is seen by later reads. The scan still
 * ends within n - 1 <= 254 loop-backs, so no host cap is needed: pairs can
 * climb onto the locals only through the call frames just below them, and
 * then either a start store onto the bound sets it to i and ends the loop,
 * or k sticks no higher than the start slot that every PUSH AX resets to
 * AH = 0; i's own slot only ever increments. The callees are the merged
 * raw-CPU pr_system_strstore_0b25,
 * pr_system_fillchar_16a8 and pr_system_set_in_0dc0; each LCALL's segment
 * word is read from the code bytes (a fixup in the loaded image), and the
 * set pointer's 00ABh immediate stays absolute. Flags: the final flags are
 * ADD DI,AX's at +01A2; at a CODE stop they are SUB SP,2Eh's, ADD
 * DI,0FF73h's or XOR AH,AH's for the three sites in order; every other
 * write is overwritten first and is folded. Offsets wrap at 16 bits, physical addresses at 1 MiB; the LES
 * pair, POP and this routine's RETF frame are read physically contiguous.
 * *callee_status (may be NULL) receives the last callee's status, or -1
 * when none ran. */
pr_u3b0_00cb_status pr_u3b0_item_split_00cb_cpu(uint8_t *ram, size_t ram_size, pr_u3b0_00cb_cpu *cpu, int *callee_status);

#endif
