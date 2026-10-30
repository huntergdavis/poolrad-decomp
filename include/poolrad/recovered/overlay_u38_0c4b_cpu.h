#ifndef POOLRAD_RECOVERED_OVERLAY_U38_0C4B_CPU_H
#define POOLRAD_RECOVERED_OVERLAY_U38_0C4B_CPU_H
#include <stddef.h>
#include <stdint.h>
#include "poolrad/recovered/system_string_leaves_cpu.h"

/* Raw-CPU, full-state model of GAME.OVR unit 38h +[0C4B,0CA8) (93 bytes, 37 instructions, RETF 0Ah), the handler that
 * the 009E dispatcher reaches through slot 39 (public entry 33, stub 0041:00C5). It is a PARTIAL CHAIN: the resident
 * window message RES_00BA_16EC is recorded as not closable, and the body stops before its call. Nothing is skipped or
 * treated as a success.
 * Arguments (after the far call: [SP] IP, [SP+2] CS): [BP+6] a word, unused by this body; [BP+8] a far pointer A (a record
 * whose byte +3 holds flag bits); [BP+0Ch] a far pointer B (a record whose word +30h is a counter). Locals (SUB SP,6):
 * [BP-5] a 5-byte string buffer ([BP-6] is never touched).
 *   1. AL := A+3 AND 10h (AND, then OR AL,AL): nonzero jumps to step 3.
 *   2. A+3 := A+3 + 10h (XOR AH,AH; ADD AX,10h; the byte store; bit 4 was clear, so no carry leaves the byte). Push B and
 *      SS:BP-5; push CS:0C46; 0AF8:0B0B (SYSTEM StrLoad) copies the 4-character string at CS:0C46 into the buffer and pops
 *      only its source (RETF 4); push AX = AH:0Ah and AX = AH:01h (AH is whatever StrLoad left); then +0C8C LCALL
 *      00BA:16EC: the model stops PENDING at +0C8C with the six argument words (B, the buffer pointer, 0Ah, 1) pushed and
 *      nothing pushed for the call. 16EC's effects (RETF 0Ch) and the following INC word B+30h are NOT completed paths on
 *      this branch: step 3 is reached by the model only through the jump of step 1.
 *   3. DS:681A := the low byte of (zero-extended DS:681A) SHL 1 (the byte doubles, its top bit falls into AH and is lost);
 *      MOV SP,BP; POP BP; RETF 0Ah.
 * Every far pointer is reread from the frame at each use (LES at each use). CS is the unit's load segment; the far-call
 * segment words CS:0C84 and CS:0C8F are loader fixups, read live. Flat 1 MiB A20-off RAM: offsets wrap at 16 bits,
 * physical addresses at 1 MiB, word operands are physically contiguous. Flags follow every flag-setting instruction
 * (SUB SP; AND/OR/XOR logic; ADD AX,10h; INC word keeps CF; SHL AX,1: CF = old bit 15, OF = MSB(result) ^ CF, SZP of the
 * result, AF := 1 by the pinned DOSBox-X rule, and that AF is visible at the RETF of step 3). See
 * docs/re/OVL_0038_0C4B_CPU.md for the contract and evidence. */
typedef struct {
    uint16_t ax, bx, cx, dx, si, di;
    uint16_t bp, sp, cs, ip, ds, es, ss, flags;
} pr_u38_0c4b_cpu;

typedef enum {
    PR_U38_0C4B_OK = 0,         /* RETF 0Ah executed (only through the step-1 jump: A+3 bit 4 already set) */
    PR_U38_0C4B_ARGUMENT,       /* null RAM or CPU, RAM not 1 MiB, IP not 0C4Bh; nothing changed */
    PR_U38_0C4B_CALLEE,         /* StrLoad did not return (diagnostic.callee_status); state as it left it */
    PR_U38_0C4B_TRANSFER,       /* StrLoad returned somewhere other than the instruction after its call */
    PR_U38_0C4B_PENDING         /* +0C8C LCALL 00BA:16EC reached (RES_00BA_16EC not closable); IP 0C8C, six words pushed */
} pr_u38_0c4b_status;

typedef struct {
    int callee;                     /* -1 none; 1 = StrLoad 0AF8:0B0B, 2 = 16EC (PENDING) */
    int callee_status;              /* the last callee's own status (-1 when none ran) */
    uint16_t site;                  /* the call site of the last callee or stop (0 when none) */
} pr_u38_0c4b_diagnostic;

/* diagnostic may be NULL. */
pr_u38_0c4b_status pr_u38_handler_0c4b_cpu(uint8_t *ram, size_t ram_size, pr_u38_0c4b_cpu *cpu,
                                           pr_u38_0c4b_diagnostic *diagnostic);
#endif
