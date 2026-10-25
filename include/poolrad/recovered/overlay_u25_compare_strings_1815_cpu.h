#ifndef POOLRAD_RECOVERED_OVERLAY_U25_COMPARE_STRINGS_1815_CPU_H
#define POOLRAD_RECOVERED_OVERLAY_U25_COMPARE_STRINGS_1815_CPU_H
#include <stddef.h>
#include <stdint.h>
#define PR_U25_1815_RAM_SIZE (1u << 20)

typedef struct {
    uint16_t ax, bx, cx, dx, si, di;
    uint16_t bp, sp, cs, ip, ds, es, ss, flags;
} pr_u25_1815_cpu;

typedef enum {
    PR_U25_1815_RETURNED = 0,
    PR_U25_1815_ARGUMENT,   /* null RAM/CPU/budget, RAM not exactly 1 MiB, IP not 1815h, entry SP outside [0220h,FFF4h]; nothing changed */
    PR_U25_1815_LIMIT,      /* the host budget ran out at the next unexecuted instruction of this body (cpu->ip) */
    PR_U25_1815_CODE,       /* a far call site is not the original opcode and offset word; CPU at the call, before its frame */
    PR_U25_1815_TRANSFER    /* a callee's RETF or this body's RETF 8 popped a return address other than its own frame's; CPU at the popped CS:IP */
} pr_u25_1815_status;

/* GAME.OVR unit 0x25 public entry 15 [1815,18EB): compare two Pascal strings into six flag bytes, 214 bytes / 86 instructions, RETF 8. Enter after
 * the far call at IP 1815: SS:[SP] return IP, [SP+2] return CS, [SP+4]/[SP+6] the string b ([BP+6]), [SP+8]/[SP+0Ah] the string a ([BP+0Ah], pushed
 * first). PUSH BP; MOV BP,SP; SUB SP,200h. StrStore 0AF8:0B25 copies a (at most FFh bytes) to the local [BP-100h] and b to [BP-200h]; FillChar
 * 0AF8:16A8 writes six zero bytes at DS:[6F78]; then six calls of the SYSTEM string compare 0AF8:0C10 on (a, b) set, in order, DS:[6F78] := 1 when
 * a = b (JNE skips), [6F79] when a <> b (JE), [6F7A] when a < b (JAE), [6F7B] when a > b (JBE), [6F7C] when a <= b (JA), [6F7D] when a >= b (JB).
 * MOV SP,BP; POP BP; RETF 8. Each LCALL's segment word is the relocated SYSTEM segment, read from the code; the opcode and offset words are checked
 * (CODE). The callees are the raw models pr_system_strstore_0b25, pr_system_fillchar_16a8 and pr_system_strcmp_0c10; all accesses, theirs and this
 * body's, are flat RAM (this body only pushes the string pointers; the callees read them flat: declared, as for every flat SYSTEM string leaf).
 * The flags at RETURNED are the last string compare's. Every operand is reread from RAM in source order (the copies happen before the clear, so a
 * string aliasing the flag bytes is compared as on entry). Each instruction of THIS body consumes one unit of *budget (the callees do not; they are
 * bounded). Exactly 1 MiB of A20-off RAM; entry SP in [0220h,FFF4h] so the 512-byte locals and the callees' frames stay inside the stack segment.
 * Code is immutable. */
pr_u25_1815_status pr_u25_compare_strings_1815_cpu(uint8_t *ram, size_t ram_size, pr_u25_1815_cpu *cpu, size_t *budget);
#endif
