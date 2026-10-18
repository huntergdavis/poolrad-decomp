#ifndef POOLRAD_RECOVERED_OVERLAY_U86_ENTRY3_0729_H
#define POOLRAD_RECOVERED_OVERLAY_U86_ENTRY3_0729_H

#include <stddef.h>
#include <stdint.h>

#define PR_U86_0729_RAM_SIZE (1u << 20)

typedef struct {
    uint16_t ax, bx, cx, dx, si, di;
    uint16_t bp, sp, cs, ip, ds, es, ss, flags;
} pr_u86_0729_cpu;

typedef enum {
    PR_U86_0729_OK = 0,
    PR_U86_0729_ARGUMENT,  /* null RAM/CPU or RAM that is not exactly 1 MiB; nothing changed */
    PR_U86_0729_CODE,      /* the reached +073A site is not LCALL xxxx:26C2 (9A C2 26); stopped before it */
    PR_U86_0729_TRANSFER   /* RES_00BA_26C2's RETF 2 popped a CS:IP other than the call's next instruction */
} pr_u86_0729_status;

/* Raw-CPU, full-state model of the complete GAME.OVR unit 0x86:+0729..+07D5
 * (public entry 3, 173 bytes, 69 instructions), entered after the far CALL
 * with SS:[SP] return IP, [SP+2] return CS and [SP+4] a word whose low byte
 * k is the argument; RETF 2. With T = the 16-byte DS table at 31FA + 16k:
 *   if DS:6DBF is 0: r := RES_00BA_26C2(k) (a real far call read from RAM
 *     at +073A, run by pr_res_00ba_26c2_cpu, which must return to +073F);
 *     [BP-2] := low byte of T[3] * r + T[2] (MUL DX; DX := the product's low
 *     word);
 *   else [BP-2] := low byte of 6 * T[3] + T[2] (SI := 2 * T[3], DX := T[2]).
 * Then [BP-2] := 1 when it is 0 and T[6] is nonzero, and [BP-2] := 1 when
 * it is FFh. [BP-1] := [BP-2]; AL := [BP-1]. The final flags are
 * CMP [BP-2],FFh's; the XOR/SHL/MUL/ADD writes before the CMPs are folded.
 * AH, CL, DX, SI, DI and ES return as the path left them. Every operand is
 * reread from RAM in source order; offsets wrap at 16 bits, physical
 * addresses at 1 MiB; LES pairs and the RETF frame are read physically
 * contiguous. */
pr_u86_0729_status pr_u86_entry3_0729(uint8_t *ram, size_t ram_size, pr_u86_0729_cpu *cpu);

#endif
