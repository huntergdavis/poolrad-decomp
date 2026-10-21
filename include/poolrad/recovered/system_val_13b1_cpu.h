#ifndef POOLRAD_RECOVERED_SYSTEM_VAL_13B1_CPU_H
#define POOLRAD_RECOVERED_SYSTEM_VAL_13B1_CPU_H
#include <stddef.h>
#include <stdint.h>

#define PR_VAL_RAM_SIZE (1u << 20)

/* Raw-CPU, full-state model of the Turbo Pascal SYSTEM Val(S; var Code) for a longint result, START.EXE 0AF8:[13B1,13E2)
 * (49 bytes, 22 instructions, RETF 8) with its near parser 0AF8:[12CE,1366) (152 bytes, 74 instructions). Readiness
 * coverage only, no identity claim (docs/re/SYSTEM_VAL_13B1_CPU.md). Pure memory: no interrupt, port or service.
 *
 * Entered after a far call: [SP] IP, [SP+2] CS, [SP+4] the far address of Code (a word), [SP+8] the far address of S
 * (a length-prefixed string). PUSH BP; MOV BP,SP; LES DI,S; CX := length; skip leading 20h bytes; CALL 12CE, which
 * parses an optional '+'/'-', then either decimal digits or '$' and hex digits (a-f folded to A-F), into DX:AX,
 * returning CF set on an empty number, a decimal overflow (DX >= 1000h before a step, or a negative step result) or a
 * hex overflow (a bit shifted out of DX); a '-' negates the result. Back at 13CB: on CF, or when unparsed characters
 * remain (CX <> 0), Code := DI - S offset (the 1-based position of the first bad character) and DX:AX := 0; otherwise
 * Code := 0 and DX:AX is the value. LES DI,Code; STOSW-free MOV ES:[DI],CX; POP BP; RETF 8.
 *
 * Contract: exactly 1 MiB of A20-off RAM; IP = 13B1h; entry SP in [000Ah,FFF4h] so the pushes (deepest entry SP - 0Ah:
 * BP, the near return and three parser pushes) and the argument words (up to entry SP + 0Bh) neither wrap nor reach
 * below 0. Every operand is reread from RAM in source order; DI and the stack offsets wrap at 16 bits, words are
 * linear-contiguous with 1 MiB wrap. Flags: every instruction's flags are computed; SHL's architecturally undefined AF
 * is overwritten (ADD, DEC or SUB) on every path before anything reads it and is set as DOSBox-X computes it (1). */
typedef struct {
    uint16_t ax, bx, cx, dx, si, di;
    uint16_t bp, sp, cs, ip, ds, es, ss, flags;
} pr_val_cpu;

typedef enum {
    PR_VAL_RETURNED = 0,  /* RETF 8 executed */
    PR_VAL_ARGUMENT       /* null RAM/CPU, RAM not exactly 1 MiB, IP not 13B1h, SP outside [000Ah,FFF4h]; nothing changed */
} pr_val_status;

pr_val_status pr_system_val_13b1(uint8_t *ram, size_t ram_size, pr_val_cpu *cpu);
#endif
