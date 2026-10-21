#ifndef POOLRAD_RECOVERED_SYSTEM_PARAMSTR_16BC_CPU_H
#define POOLRAD_RECOVERED_SYSTEM_PARAMSTR_16BC_CPU_H
#include <stddef.h>
#include <stdint.h>

#define PR_PARAMSTR_RAM_SIZE (1u << 20)

/* Raw-CPU, full-state model of the Turbo Pascal SYSTEM ParamStr path of START.EXE 0AF8:16BC (79 bytes, RETF 2)
 * with its near helper 0AF8:[1712,1744) (50 bytes), for a non-zero argument. It stays DECLARED_BOUNDARY with no
 * credit: the C exists only so RES_0899_0059 can compose the real bytes (docs/re/SYSTEM_PARAMSTR_16BC_CPU.md).
 *
 * Entered after a far call: [SP] IP, [SP+2] CS, [SP+4] the index n (word), [SP+6] the destination offset,
 * [SP+8] its segment. PUSH BP; MOV BP,SP; MOV DX,[BP+6]; OR DX,DX; CALL 1712, which takes ES := PrefixSeg
 * DS:[4422], DI := 80h, CX := the command-tail length ES:[0080], and walks the tail word by word (a word is a
 * run of bytes above 20h) until the n-th, leaving SI at its start and AX = its length (0 past the last word);
 * then PUSH DS; PUSH ES; POP DS; LES DI,[BP+8]; CLD; STOSB (the length byte); XCHG CX,AX; REP MOVSB (the
 * text); POP DS; POP BP; RETF 2. Pure memory: no interrupt, no service.
 *
 * Contract: n = 0 (the DOS-version/environment-copy mode, INT 21h AH=30h) is ARGUMENT, not modelled: RES_0899_0059
 * always passes 1, and a caller that can pass 0 carries that gap in its own row. Entry SP in [0006h,FFF6h] keeps
 * the frame (deepest entry SP - 6, highest entry SP + 9) inside SS without crossing offset FFFFh. Every other
 * operand is read from RAM in source order; byte operands wrap at 16 bits within their segment, as SI/DI do. */
typedef struct {
    uint16_t ax, bx, cx, dx, si, di;
    uint16_t bp, sp, cs, ip, ds, es, ss, flags;
} pr_paramstr_cpu;

typedef enum {
    PR_PARAMSTR_RETURNED = 0,  /* RETF 2 executed */
    PR_PARAMSTR_ARGUMENT       /* null RAM/CPU, RAM not exactly 1 MiB, IP not 16BCh, SP outside [0006h,FFF6h],
                                  or n = 0 (mode 0, not modelled); nothing changed */
} pr_paramstr_status;

pr_paramstr_status pr_system_paramstr_16bc(uint8_t *ram, size_t ram_size, pr_paramstr_cpu *cpu);
#endif
