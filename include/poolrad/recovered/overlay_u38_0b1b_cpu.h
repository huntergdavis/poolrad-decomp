#ifndef POOLRAD_RECOVERED_OVERLAY_U38_0B1B_CPU_H
#define POOLRAD_RECOVERED_OVERLAY_U38_0B1B_CPU_H
#include <stddef.h>
#include <stdint.h>
#include "poolrad/recovered/system_string_leaves_cpu.h"

/* Raw-CPU model of GAME.OVR unit 38h +[0B1B,0BAC) (145 bytes, 47 instructions, RETF 0Ah): public entry 27, stub
 * 0041:00A7, the 009E dispatcher's slot-32 handler. The ledger's 145-byte bound is exact (the next entry +0BAC begins
 * with code). Entered after a far call: [SP] IP, [SP+2] CS, then five argument words: [BP+6] (never read), the far
 * pointer A at [BP+8]/[BP+0Ah] and the far pointer B at [BP+0Ch]/[BP+0Eh]; A and B are reread with LES at every use.
 * PUSH BP; MOV BP,SP; SUB SP,0Ah (a 10-byte local L at SS:BP-0Ah).
 *   1. A+4 := 0.
 *   2. CMP byte DS:[681Fh], 0; not equal: step 4.
 *   3. Push B; AL := 6, push AX (the entry AH rides in the high byte); push SS:L; push CS:0B11; LCALL 0AF8:0B0B
 *      (SYSTEM StrLoad; segment word CS:0B46 is a loader fixup): L := the 9-character Pascal string at CS:0B11
 *      ("collapses", 10 bytes), through the accepted leaf model pr_system_strload_0b0b (RETF 4 pops the source pair and
 *      leaves SS:L; AL := 09h, CX := 0, DX := DS, BX := its entry SP, SI/DI advanced, DF cleared, flags XOR CH,CH's; AH
 *      kept). Then PUSH CS; CALL 0058 (near, with a far return frame): the unit-local routine +0058 (181 bytes, itself
 *      reaching 00BA:16EC, 00B0:0061/002F, 03E0:0075, 0709:0DBE, 00BA:17E3 and 00BA:0767; no accepted model) receives
 *      (B, the AX word, SS:L) and would balance them with its RETF 0Ah. The model STOPS here (STOP_0058) with the CPU
 *      exactly as the CALL left it: CS and 0B4C pushed under the five argument words, CS unchanged, IP 0058h. Nothing is
 *      rolled back; the post-call steps are not reached.
 *   4. (Directly when DS:[681Fh] != 0; after 0058 otherwise, unreachable in this model.) AL := byte A+3; AH := 0; CL :=
 *      4; SHR AX,CL; B+10Eh := AL; B+10Fh := 1; B+76h := 0; B+6Bh := byte B+98h; B+72h := 0Ch; CMP byte B+84h, 0B3h;
 *      equal: B+84h := 0; B+9Fh := 0. MOV SP,BP; POP BP; RETF 0Ah.
 * CS is the unit's load segment. Flat 1 MiB A20-off RAM: offsets wrap at 16 bits, physical addresses at 1 MiB, word
 * operands are physically contiguous. No backward branch, so no instruction budget of its own.
 * Composition profile (full CPU14 + RAM state is claimed INSIDE it only): the CPU and diagnostic objects are distinct
 * from RAM; loaded code and bindings (CS:0B11..0B1A, CS:0B46) are read live and immutable during the run; StrLoad's
 * accepted contract applies to the copy (SI/DI wrap at 16 bits, L and the source may alias); +0058's own entry
 * preconditions are NOT checked because it is not entered; interrupts, faults and self-modifying code are outside this
 * synchronous profile. Flags: SUB SP,0Ah (0B1E) is overwritten by CMP DS:[681Fh],0 (0B29), which the JNE reads; on the
 * STOP_0058 path the flags are StrLoad's; on the OK path XOR AH,AH (0B53) and SHR AX,CL (0B57; OF for a count of 4
 * and AF are architecturally undefined, pinned here as the emitter's rule with AF := 1) are overwritten by CMP
 * B+84h,0B3h (0B8C) before any read, and the return flags are that CMP's. See docs/re/OVL_0038_0B1B_CPU.md. */
typedef struct {
    uint16_t ax, bx, cx, dx, si, di;
    uint16_t bp, sp, cs, ip, ds, es, ss, flags;
} pr_u38_0b1b_cpu;

typedef enum {
    PR_U38_0B1B_OK = 0,         /* RETF 0Ah executed (the DS:[681Fh] != 0 path) */
    PR_U38_0B1B_ARGUMENT,       /* null pointers, RAM not exactly 1 MiB, IP not 0B1Bh; nothing changed */
    PR_U38_0B1B_CALLEE,         /* StrLoad did not return (its status in diagnostic.callee_status); state as it left it */
    PR_U38_0B1B_TRANSFER,       /* StrLoad returned somewhere other than CS:0B48 */
    PR_U38_0B1B_STOP_0058       /* the CALL at 0B49 executed; CPU at the callee entry (unit 38h +0058 not composed) */
} pr_u38_0b1b_status;

typedef struct {
    int callee;                 /* -1 none; 1 = SYSTEM 0B0B StrLoad; 2 = unit 38h +0058 */
    int callee_status;          /* StrLoad's status when callee is 1, otherwise -1 */
    uint16_t site;              /* IP of the last call entered (0B43 or 0B49), 0 when none */
} pr_u38_0b1b_diagnostic;

/* diagnostic may be NULL. */
pr_u38_0b1b_status pr_u38_0b1b_cpu_run(uint8_t *ram, size_t ram_size, pr_u38_0b1b_cpu *cpu, pr_u38_0b1b_diagnostic *diagnostic);
#endif
