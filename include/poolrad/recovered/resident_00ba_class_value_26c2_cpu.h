#ifndef POOLRAD_RECOVERED_RESIDENT_00BA_CLASS_VALUE_26C2_CPU_H
#define POOLRAD_RECOVERED_RESIDENT_00BA_CLASS_VALUE_26C2_CPU_H

#include <stddef.h>
#include <stdint.h>

#define PR_RES_26C2_RAM_SIZE (1u << 20)

typedef struct {
    uint16_t ax, bx, cx, dx, si, di;
    uint16_t bp, sp, cs, ip, ds, es, ss, flags;
} pr_res_26c2_cpu;

typedef enum {
    PR_RES_26C2_OK = 0,
    PR_RES_26C2_ARGUMENT  /* null RAM/CPU or RAM that is not exactly 1 MiB; nothing changed */
} pr_res_26c2_status;

/* Raw-CPU, full-state model of the complete START.EXE resident routine
 * RES_00BA_26C2 (expanded image 0x3262..0x32C8, 103 bytes, 38 instructions, no calls),
 * entered after the far CALL with SS:[SP] return IP, [SP+2] return CS and
 * [SP+4] a word whose low byte k is the argument; RETF 2.
 *
 * c := DS:[31FA + 16k] (DI := 16k, CL := 4). The result byte [BP-1] is the
 * record byte +96h (c = 0) or +9Bh (c = 1) of the far pointer DS:5D92
 * (ES:DI := that pointer), 0Ch (c = 2), or the stack byte already at
 * [BP-1] (any other c). Then, if DS:6DBF is nonzero and DS:[31FA + 16k]
 * (reread, DI := 16k again) is not 2, [BP-1] := 6. AL := [BP-1], AH = 0.
 * The final flags are CMP [6DBF],0's when that byte is 0, else
 * CMP [DI+31FA],2's; the SUB/XOR/SHL writes before them are folded. Every
 * operand is reread from RAM in source order; offsets wrap at 16 bits,
 * physical addresses at 1 MiB; LES pairs and the RETF frame are read
 * physically contiguous. */
pr_res_26c2_status pr_res_00ba_26c2_cpu(uint8_t *ram, size_t ram_size, pr_res_26c2_cpu *cpu);

#endif
