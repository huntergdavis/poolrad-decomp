#ifndef POOLRAD_RECOVERED_OVERLAY_U38_070F_CPU_H
#define POOLRAD_RECOVERED_OVERLAY_U38_070F_CPU_H
#include <stddef.h>
#include <stdint.h>
#include "poolrad/recovered/system_string_leaves_cpu.h"

/* Raw-CPU model of GAME.OVR unit 38h +[070F,0764) (85 bytes, 30 instructions, RETF 0Ah): public entry 19, stub
 * 0041:007F, the 009E dispatcher's slot-21 handler. The ledger's 102-byte bound (98 decoded) runs into the next entry's
 * length-prefixed constant "dies from poison" at +0764, which is not code. Entered after a far call: [SP] IP, [SP+2] CS,
 * then five argument words of which only [BP+0Ch]/[BP+0Eh] (far pointer B) are read; [BP+6] and the far pointer at
 * [BP+8] are never read. PUSH BP; MOV BP,SP; SUB SP,0Ch (a 12-byte local L at SS:BP-0Ch).
 *   1. C := the far pointer at B+108h (B and C reread with LES at every use). CMP byte C+2, 0; equal: step 4.
 *   2. Push B; push SS:L; push CS:0703; LCALL 0AF8:0B0B (SYSTEM StrLoad; segment word CS:0737 is a loader fixup):
 *      L := the 11-character Pascal string at CS:0703 ("is silenced", 12 bytes), through the accepted leaf model
 *      pr_system_strload_0b0b (RETF 4 pops the source and leaves SS:L on the stack; AL := 0Bh, CX := 0, DX := DS,
 *      BX := its entry SP, DF cleared, flags XOR CH,CH's; AH keeps the entry AH).
 *   3. Push AX with AL = 0Ah (row), push AX with AL = 1 (wait); LCALL 00BA:16EC (segment word CS:0742 is a fixup): the
 *      resident window message with (wait 1, row 0Ah, message SS:L, B's two words for 1816), RETF 0Ch. The model
 *      STOPS here (STOP_16EC) with the CPU exactly as the LCALL left it: the return frame (CS, 0744) pushed under the
 *      six argument words, CS the live word at CS:0742, IP 16ECh. A merged bounded bus-domain model of 16EC exists
 *      (pr_res_00ba_window_16ec_cpu); composing it is a separate bounded step, not done here. Nothing is rolled back.
 *   4. (Reached directly when C+2 = 0; after 16EC otherwise, unreachable in this model.) C+2 := 0; C+1 := 0 (B and C
 *      reread before each store). MOV SP,BP; POP BP; RETF 0Ah.
 * CS is the unit's load segment. Flat 1 MiB A20-off RAM: offsets wrap at 16 bits, physical addresses at 1 MiB, word
 * operands are physically contiguous. No backward branch, so no instruction budget of its own.
 * Composition profile (full CPU14 + RAM state is claimed INSIDE it only): the CPU and diagnostic objects are distinct
 * from RAM; loaded code and bindings (CS:0703..070E, CS:0737, CS:0742) are read live and immutable during the run;
 * StrLoad's accepted contract applies to the copy (SI/DI wrap at 16 bits, L and the source may alias); the 16EC entry
 * SP precondition of that model ([0800h,FFF0h]) is NOT checked here because 16EC is not entered; interrupts, faults
 * and self-modifying code are outside this synchronous profile. Flags: SUB SP,0Ch at 0712 is overwritten by CMP at 071D
 * before any read; on the OK path the return flags are CMP C+2,0's; at STOP_16EC they are StrLoad's. No flag of this
 * body is architecturally undefined (SUB/CMP only). See docs/re/OVL_0038_070F_CPU.md. */
typedef struct {
    uint16_t ax, bx, cx, dx, si, di;
    uint16_t bp, sp, cs, ip, ds, es, ss, flags;
} pr_u38_070f_cpu;

typedef enum {
    PR_U38_070F_OK = 0,         /* RETF 0Ah executed (the C+2 = 0 path) */
    PR_U38_070F_ARGUMENT,       /* null pointers, RAM not exactly 1 MiB, IP not 070Fh; nothing changed */
    PR_U38_070F_CALLEE,         /* StrLoad did not return (its status in diagnostic.callee_status); state as it left it */
    PR_U38_070F_TRANSFER,       /* StrLoad returned somewhere other than CS:0739 */
    PR_U38_070F_STOP_16EC       /* the LCALL at 073F executed; CPU at the callee entry (RES_00BA_16EC not composed) */
} pr_u38_070f_status;

typedef struct {
    int callee;                 /* -1 none; 1 = SYSTEM 0B0B StrLoad; 2 = 00BA:16EC */
    int callee_status;          /* StrLoad's status when callee is 1, otherwise -1 */
    uint16_t site;              /* IP of the last LCALL entered, 0 when none */
} pr_u38_070f_diagnostic;

/* diagnostic may be NULL. */
pr_u38_070f_status pr_u38_070f_cpu_run(uint8_t *ram, size_t ram_size, pr_u38_070f_cpu *cpu, pr_u38_070f_diagnostic *diagnostic);
#endif
