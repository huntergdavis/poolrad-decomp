#ifndef POOLRAD_RECOVERED_OVERLAY_U7F_DIV5_LEAVES_CPU_H
#define POOLRAD_RECOVERED_OVERLAY_U7F_DIV5_LEAVES_CPU_H
#include "poolrad/recovered/memory_bus.h"

typedef struct {
    uint16_t ax, bx, cx, dx, si, di;
    uint16_t bp, sp, cs, ip, ds, es, ss, flags;
} pr_u7f_div5_cpu;

typedef enum {
    PR_U7F_DIV5_CPU_RETURNED = 0,
    PR_U7F_DIV5_CPU_ARGUMENT,   /* null bus/RAM/CPU/budget, RAM not exactly 1 MiB, IP not the entry, entry SP outside [0004h,FFFAh]; nothing changed */
    PR_U7F_DIV5_CPU_LIMIT,      /* the host budget ran out at the next unexecuted instruction (cpu->ip) */
    PR_U7F_DIV5_CPU_TRANSFER    /* RETF 2 popped a return address other than the entry frame's (the body overwrote its own frame); CPU at the popped CS:IP */
} pr_u7f_div5_cpu_status;

/* GAME.OVR unit 0x7F public entry 10 [0130,0185): 85 bytes / 31 instructions, RETF 2, no calls, no relocation. Enter after the far call at IP 0130:
 * SS:[SP] return IP, [SP+2] return CS, [SP+4] the value word ([BP+6]). PUSH BP; MOV BP,SP; SUB SP,2; the byte [BP-1] counts 0..4 (do-while): each pass
 * LES DI,DS:[5D92] (the active-character far pointer) and the word ES:[DI + 88h + 2*CBW(i)] := 0. Then the value DIV 5: ES:[DI+90h] := the quotient
 * (LES again), and once more: XCHG DX,AX; ES:[DI+8Eh] := the remainder (LES again). MOV SP,BP; POP BP; RETF 2. The record is reached THROUGH THE BUS
 * (a pointer the body did not create); the stack and DS:[5D92] are flat. The flags follow the pinned DOSBox-X stored rules: SHL by 1 sets AF and
 * OF = bit15 XOR bit14; XOR clears CF/OF/AF; INC keeps CF; a word DIV writes the six arithmetic flags (AF=SF=OF=0, ZF = remainder 0 and quotient odd,
 * CF = (remainder AND 3) in 1..2, PF = even parity of the four bytes of remainder:quotient). The second DIV is the last flag writer at RETURNED.
 * Every operand is reread from RAM in source order; LES reads its segment word at (offset+2) mod 64K. Each instruction consumes one unit of *budget;
 * LIMIT leaves the next unexecuted instruction in cpu->ip. Exactly 1 MiB of A20-off RAM; entry SP in [0004h,FFFAh]. Code is immutable. */
pr_u7f_div5_cpu_status pr_u7f_div5_0130_cpu(pr_bus *bus, size_t ram_size, pr_u7f_div5_cpu *cpu, size_t *budget);

/* GAME.OVR unit 0x7F public entry 11 [0185,01DB): 86 bytes / 34 instructions, RETF 2, no calls, no relocation (the 40 bytes after it are +0203's
 * literal, never read here). Same frame. The byte [BP-1] counts 0..4 (do-while): DI := CBW(i) SHL 1 SHL 1, and the words DS:[DI+67F4h] and
 * DS:[DI+67F6h] := 0. Then the value DIV 5; XOR DX,DX; DS:[6804] := the quotient, DS:[6806] := 0; once more: XCHG DX,AX; XOR DX,DX; DS:[6800] := the
 * remainder, DS:[6802] := 0. MOV SP,BP; POP BP; RETF 2. All accesses are flat (DS globals and the stack). Flags as for entry 10; the last flag
 * writer at RETURNED is XOR DX,DX. Budget, LIMIT, RAM and SP as for entry 10. */
pr_u7f_div5_cpu_status pr_u7f_div5_0185_cpu(pr_bus *bus, size_t ram_size, pr_u7f_div5_cpu *cpu, size_t *budget);
#endif
