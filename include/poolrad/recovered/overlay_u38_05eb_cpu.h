#ifndef POOLRAD_RECOVERED_OVERLAY_U38_05EB_CPU_H
#define POOLRAD_RECOVERED_OVERLAY_U38_05EB_CPU_H
#include <stddef.h>
#include <stdint.h>
#include "poolrad/recovered/overlay_u38_gated_append_001f_cpu.h"

/* Raw-CPU model of GAME.OVR unit 38h +[05EB,0647) (92 bytes, 35 instructions, RETF 0Ah): public entry 14,
 * stub 0041:0066, the 009E dispatcher's slot-15 handler. Entered after a far call: [SP] IP, [SP+2] CS, then five argument
 * words: [BP+6] a word this body never reads, [BP+8] far pointer A, [BP+0Ch] far pointer B.
 *   1. PUSH CS / CALL 001F: this unit's +001F with (a = 0Ah, c = byte A+3, b = 0Fh, record B), composed through its raw
 *      model pr_u38_gated_append_001f_cpu (DS:[681F] gates an append through the live stub 0052 / U9E +0E52 / GetMem).
 *      The two byte arguments are pushed as AX, so the entry AH sits in the high bytes of those stack words. The frame
 *      001F sees is exactly a far call's (PUSH CS supplies the segment; 001F's RETF 0Ah pops both and the five words).
 *   2. AL = 0 (001F stored result 0): return. B+11Bh <= 1 (unsigned, B reread with LES): return. Otherwise
 *      DS:[6819] := 0 and LCALL 00B0:007F with (B, 1, 0, 0), again as AX pushes with AH as 001F left it. The segment
 *      word CS:062D is a loader fixup (unit 9Eh's stub); stub offset 007F is entry 19, OVL_009E_1344, which has no model:
 *      the model STOPS there (STOP_1344) with the CPU exactly as the LCALL left it: the return frame (CS, 062F) pushed,
 *      CS the live word at CS:062D, IP 007F. Nothing is rolled back.
 *   3. (Unreachable while step 2 stops.) DS:[49F3] = 5: return; else LCALL 00BA:0767 (segment word CS:0641, a fixup) with
 *      the far pointer DS:[5D92]/[5D94]: STOP_0767, the same saved-state convention.
 * CS is the unit's load segment. Flat 1 MiB A20-off RAM: offsets wrap at 16 bits, physical addresses at 1 MiB, word
 * operands are physically contiguous. There is no backward branch, so no instruction budget of its own.
 * Composition profile (inherited from 001F's merged contract; full CPU14 + RAM state is claimed INSIDE it only):
 *   - 001F is entered at SP' = (entry SP - 16) mod 65536 and requires SP' in [0040,FFF2]; outside that, 001F returns
 *     ARGUMENT and this model reports CALLEE with callee_status ARGUMENT and the partial prefix in place (BP frame and the
 *     seven pushed words written, IP 001F). ARGUMENT of this body itself (null/size/IP) changes nothing.
 *   - The CPU, limits and diagnostic objects are distinct from RAM. Loaded code and bindings are immutable during the
 *     run: the segment words CS:062D/CS:0641 and 001F's binding chain (CS:004A, stub 0052, U9E +0E52, SYSTEM) are read
 *     live but not rewritten by the modelled code. Reached dependency return frames are nonwrapping. Data, heap and
 *     stack aliases stay live (001F's append may write under this frame). No rollback.
 *   - Interrupts, faults and self-modifying code are outside this synchronous profile.
 * Flags: OR AL,AL at 0607 leaves AF architecturally undefined; this tree's logic() convention sets AF := 0 (CF/OF := 0),
 * which is VISIBLE at the RETF when AL = 0 (the OR is the last flags writer on that path) and overwritten by CMP at 060E
 * otherwise. That is a deterministic modelling convention, not a claim of native AF equality.
 * See docs/re/OVL_0038_05EB_CPU.md for the contract and evidence. */
typedef struct {
    uint16_t ax, bx, cx, dx, si, di;
    uint16_t bp, sp, cs, ip, ds, es, ss, flags;
} pr_u38_05eb_cpu;

typedef enum {
    PR_U38_05EB_OK = 0,         /* RETF 0Ah executed */
    PR_U38_05EB_ARGUMENT,       /* null pointers, RAM not exactly 1 MiB, IP not 05EBh; nothing changed */
    PR_U38_05EB_CALLEE,         /* 001F stopped (its status in diagnostic.callee_status); state as it left it */
    PR_U38_05EB_TRANSFER,       /* 001F returned somewhere other than CS:0607 */
    PR_U38_05EB_STOP_1344,      /* the LCALL at 062A executed; CPU at the callee entry (no model for OVL_009E_1344) */
    PR_U38_05EB_STOP_0767       /* the LCALL at 063E executed; CPU at the callee entry (no raw model for RES_00BA_0767) */
} pr_u38_05eb_status;

typedef struct {
    unsigned max_heap_steps;    /* 001F's GetMem walk cap */
    size_t max_links;           /* 001F's list-append link cap */
} pr_u38_05eb_limits;

typedef struct {
    int callee;                 /* -1 none; 1 = 001F (near); 2 = 00B0:007F (OVL_009E_1344); 3 = 00BA:0767 */
    int callee_status;          /* 001F's status when callee is 1, otherwise -1 */
    uint16_t site;              /* IP of the last call instruction entered, 0 when none */
    pr_u38_001f_diagnostic d001f;
} pr_u38_05eb_diagnostic;

/* limits may be NULL (both caps zero: 001F's dependency meanings apply). diagnostic may be NULL. */
pr_u38_05eb_status pr_u38_05eb_cpu_run(uint8_t *ram, size_t ram_size, pr_u38_05eb_cpu *cpu,
                                       const pr_u38_05eb_limits *limits, pr_u38_05eb_diagnostic *diagnostic);
#endif
