#ifndef POOLRAD_RECOVERED_OVERLAY_U38_HANDLER_09C0_CPU_H
#define POOLRAD_RECOVERED_OVERLAY_U38_HANDLER_09C0_CPU_H
#include <stddef.h>
#include <stdint.h>
#include "poolrad/recovered/overlay_u9e_entry8_fullstate_compose_0de3.h"
#include "poolrad/recovered/overlay_u38_helper_cluster_cpu.h"
#include "poolrad/recovered/system_string_leaves_cpu.h"
#include "poolrad/recovered/overlay_u9e_list_release_0026_cpu.h"

/* Raw-CPU, full-state model of GAME.OVR unit 38h +[09C0,0A3D) (125 bytes, 52 instructions, RETF 0Ah), the handler that
 * the 009E dispatcher reaches through slot 28 (public entry 24, stub 0041:0098). It is a PARTIAL CHAIN: the resident
 * window message RES_00BA_16EC is recorded as not closable, and the body stops before its call. Nothing is skipped or
 * treated as a success.
 * Arguments (after the far call: [SP] IP, [SP+2] CS): [BP+6] a byte, unused by this body; [BP+8] a far pointer A (a record
 * whose byte +3 is a counter); [BP+0Ch] a far pointer B. Locals (SUB SP,0Eh): [BP-0Eh] a 14-byte string buffer.
 *   1. Roll: push AX = AH:01h (the count), then AX := zero-extended A+3, plus 1 (the range); LCALL 00B0:0048: the loaded stub
 *      of unit 9E entry 8 (five bytes at the fixup segment read live at CS:09D7) must be EAh E3 0D (a far JMP to +0DE3),
 *      checked before the call pushes anything (CODE otherwise); the credited roll pr_u9e_entry8_fullstate_compose_0de3
 *      runs at the stub's segment and returns AL (RETF 4). AL <= 1 (unsigned): return.
 *   2. DS:681B = 0: return. DS:6820 <> 0: return.
 *   3. Push AX = AH:00h; PUSH CS; CALL +0000: the credited helper pr_u38_helper_0000_cpu (key 0: DS:6818 := 0, DS:6817 := 0;
 *      RETF 2). Push B and SS:BP-0Eh; 0AF8:0B0B (SYSTEM StrLoad) copies the 13-character string at CS:09B2 into the buffer
 *      and pops only its source (RETF 4); push AX = AH:0Ah and AX = AH:01h; then +0A0D LCALL 00BA:16EC: the model stops
 *      PENDING at +0A0D with the six argument words (B, the buffer pointer, 0Ah, 1) pushed and nothing pushed for the call.
 *      16EC's effects, A+3 := A+3 - 1, the test of the new A+3 against 0, the LCALL 00B0:002A (unit 9E +0026 through its
 *      loaded stub, composed exactly as for +0775) and the return are NOT completed paths on this branch: OK is reachable
 *      only through the three early returns of steps 1 and 2.
 * Every far pointer is reread from the frame at each use (LES at each use). CS is the unit's load segment; the far-call
 * segment words CS:09D7, CS:0A05, CS:0A10 and CS:0A35 are loader fixups, read live. AH is never cleared before the 01h,
 * 00h, 0Ah and 01h pushes (XOR AH,AH precedes only the range): the callees see the live AH. Flat 1 MiB A20-off RAM:
 * offsets wrap at 16 bits, physical addresses at 1 MiB, word operands are physically contiguous. Flags follow every
 * flag-setting instruction (XOR AH,AH logic; INC AX keeps CF; the CMPs; DEC byte keeps CF). See
 * docs/re/OVL_0038_09C0_CPU.md for the contract and evidence. */
typedef struct {
    uint16_t ax, bx, cx, dx, si, di;
    uint16_t bp, sp, cs, ip, ds, es, ss, flags;
} pr_u38_09c0_cpu;

typedef enum {
    PR_U38_09C0_OK = 0,         /* RETF 0Ah executed (only through the early returns) */
    PR_U38_09C0_ARGUMENT,       /* null RAM, CPU or limits, RAM not 1 MiB, IP not 09C0h; nothing changed */
    PR_U38_09C0_CODE,           /* a loaded stub (00B0:0048 at +09D4, 00B0:002A at +0A32) is not the far JMP to the entry; IP at the call, nothing pushed */
    PR_U38_09C0_CALLEE,         /* the roll, the helper, StrLoad or U9E +0026 stopped (diagnostic.callee and callee_status); state as it left it */
    PR_U38_09C0_TRANSFER,       /* a callee returned somewhere other than the instruction after its call */
    PR_U38_09C0_PENDING         /* +0A0D LCALL 00BA:16EC reached (RES_00BA_16EC not closable); IP 0A0D, six words pushed */
} pr_u38_09c0_status;

typedef struct {
    unsigned roll_system_calls;     /* the roll's cap on SYSTEM call slices (0 permits only a zero count; this body's count is 1) */
    pr_u9e_0026_limits release;     /* U9E +0026's list and heap caps (unreachable in this partial chain; kept for the exact composition) */
} pr_u38_09c0_limits;

typedef struct {
    int callee;                     /* -1 none; 1 = roll +0DE3 (stub 0048), 2 = helper +0000, 3 = StrLoad 0AF8:0B0B, 4 = 16EC (PENDING), 5 = U9E +0026 */
    int callee_status;              /* the last callee's own status (-1 when none ran) */
    uint16_t site;                  /* the call site of the last callee or stop (0 when none) */
    pr_u9e_0026_diagnostic d0026;   /* U9E +0026's diagnostic (freemem_status -1 until it runs) */
} pr_u38_09c0_diagnostic;

/* diagnostic may be NULL. limits must not. */
pr_u38_09c0_status pr_u38_handler_09c0_cpu(uint8_t *ram, size_t ram_size, pr_u38_09c0_cpu *cpu,
                                           const pr_u38_09c0_limits *limits, pr_u38_09c0_diagnostic *diagnostic);
#endif
