#ifndef POOLRAD_RECOVERED_RESIDENT_00BA_SIDECAR_LOOKUP_2199_CPU_H
#define POOLRAD_RECOVERED_RESIDENT_00BA_SIDECAR_LOOKUP_2199_CPU_H

#include <stddef.h>
#include <stdint.h>

#define PR_RES_2199_RAM_SIZE (1u << 20)

typedef struct {
    uint16_t ax, bx, cx, dx, si, di;
    uint16_t bp, sp, cs, ip, ds, es, ss, flags;
} pr_res_2199_cpu;

typedef enum {
    PR_RES_2199_OK = 0,
    PR_RES_2199_ARGUMENT,  /* null RAM/CPU or RAM that is not exactly 1 MiB; nothing changed */
    PR_RES_2199_LIMIT      /* host cap: one more loop-back than max_loopbacks; CS:IP = CS:(entry IP + 20h) */
} pr_res_2199_status;

/* Raw-CPU, full-state model of the complete START.EXE resident routine
 * RES_00BA_2199 (expanded image 0x2D39..0x2DA7, 111 bytes, 37
 * instructions, three conditional branches, no calls, no DS access),
 * the 9-byte sidecar lookup whose callback model is
 * resident_sidecar_lookup.h. Entered at CS:IP = resident 00BA:2199 after
 * the far CALL with SS:[SP] return IP, [SP+2] return CS and the Pascal
 * arguments above them: [BP+6]/[BP+8] the cursor cell C (far pointer),
 * [BP+0Ah] the key word (low byte k) and [BP+0Ch]/[BP+0Eh] the character
 * object X (far pointer). RETF 0Ah removes the arguments.
 *
 * Ordered effects: [BP-2] := 0; C := X->+7F/+81 (both words, AX and DX
 * loaded first). Then, from the loop head +20h: AX := C.low | C.high
 * (flags of OR); exit when that is zero or when [BP-2] (CMP [BP-2],0) is
 * nonzero. Otherwise, if byte 0 of the node C points at equals [BP+0Ah]'s
 * low byte, [BP-2] := 1; else C := node+5/+7 (AX and DX loaded first).
 * Each pass jumps back to the loop head. At exit [BP-1] := [BP-2] and AL
 * := [BP-1]; AH keeps the OR result's high byte, DX the last head or link
 * high word loaded, ES:DI the cursor-cell pointer; the final flags are
 * the OR's (AF cleared, as both oracles do) or the CMP's. Every operand,
 * including C, the node pointer and the key, is reread from RAM in source
 * order, so a cursor cell that aliases the frame, a node or the character
 * behaves as DOS does. Offsets wrap at 16 bits, physical addresses at
 * 1 MiB; words, LES pairs and the RETF frame are read physically
 * contiguous, as the Unicorn oracle reads them.
 *
 * max_loopbacks: the number of taken JMP +5Eh -> +20h loop-backs the
 * model follows (a match takes one more before it exits). On the next
 * one it stops at the loop head with CS unchanged, IP = entry IP + 20h,
 * SP = BP - 2 and every earlier effect kept (status LIMIT), which is the
 * original's state on reaching the loop head that time. A list of n
 * nodes needs at most n + 1 loop-backs; a cyclic list without the key
 * makes the original loop forever. */
pr_res_2199_status pr_res_00ba_2199_cpu(uint8_t *ram, size_t ram_size, pr_res_2199_cpu *cpu, unsigned max_loopbacks);

#endif
