#ifndef POOLRAD_RECOVERED_OVERLAY_U38_10DE_CPU_H
#define POOLRAD_RECOVERED_OVERLAY_U38_10DE_CPU_H
#include <stddef.h>
#include <stdint.h>
#include "poolrad/recovered/overlay_u38_gated_append_001f_cpu.h"
#include "poolrad/recovered/system_string_leaves_cpu.h"
#include "poolrad/recovered/resident_00ba_sidecar_lookup_2199_cpu.h"
#include "poolrad/recovered/overlay_u9e_list_append_0e52_cpu.h"

/* Raw-CPU, full-state model of GAME.OVR unit 38h +[10DE,1168) (138 bytes, 60 instructions, RETF 0Ah), the handler that
 * the 009E dispatcher reaches through slot 43 (public entry 37, stub 0041:00D9). It is a PARTIAL CHAIN: the resident
 * window message RES_00BA_16EC is recorded as not closable, and the message path stops before its call. Nothing is
 * skipped or treated as a success.
 * Arguments (after the far call: [SP] IP, [SP+2] CS): [BP+6] a word, unused by this body; [BP+8] a far pointer A (a record
 * whose byte +3 is read once); [BP+0Ch] a far pointer B (a record whose byte +10h is a counter and whose far list head is
 * at +7Fh/+81h). Locals (SUB SP,10h): [BP-10h] a 16-byte string buffer; [BP-4] a 4-byte cursor cell (the same bytes on
 * the other path).
 *   1. Append. Push B, AX = AH:2Bh, AX = AH:(A+3), AX = 3Ch; PUSH CS; CALL +001F: the credited gated append
 *      pr_u38_gated_append_001f_cpu (DS:681F = 0: a 9-byte node (byte 0 = 2Bh, word 1 = 3Ch, byte 3 = A+3, byte 4 = 1)
 *      through the live stub 00B0:0052, unit 9E +0E52 and GetMem, AL := 1; DS:681F <> 0: AL := 0; RETF 0Ah). OR AL,AL;
 *      JE: AL = 0 returns.
 *   2. Branch on B+10h: CMP byte ES:[DI+10h],3; JBE goes to step 4.
 *   3. Message (B+10h > 3). Push B and SS:BP-10h; push CS:10D2; 0AF8:0B0B (SYSTEM StrLoad) copies the 11-character string at
 *      CS:10D2 into the buffer and pops only its source (RETF 4); push AX = AH:0Ah and AX = AH:01h (AH is whatever StrLoad
 *      left); then +1126 LCALL 00BA:16EC: the model stops PENDING at +1126 with the six argument words (B, the buffer
 *      pointer, 0Ah, 1) pushed and nothing pushed for the call. 16EC's effects (RETF 0Ch), DEC byte B+10h and the JMP to
 *      the epilogue are NOT completed paths on this branch.
 *   4. Lookup (B+10h <= 3). Push B, AX = AH:1Fh and SS:BP-4; LCALL 00BA:2199: the credited sidecar lookup
 *      pr_res_00ba_2199_cpu searches B's list (head +7Fh/+81h, links +5/+7) for a node whose byte 0 is 1Fh, leaves the
 *      cursor in [BP-4] and AL = found (RETF 0Ah). OR AL,AL; JNE: found returns.
 *   5. Second append (not found). Push B, AX = AH:1Fh (2199's AH), 0 (XOR AX,AX), 00FFh, 0000h; LCALL 00B0:0052: the loaded
 *      stub of unit 9E entry 10 (five bytes at the fixup segment read live at CS:1160, offset 0052) must be EAh 52 0E (a far
 *      JMP to +0E52), checked before the call pushes anything (CODE otherwise); the credited list append
 *      pr_u9e_list_append_0e52_cpu runs at the stub's segment: a 9-byte node (byte 0 = 1Fh, word 1 = 0, byte 3 = FFh,
 *      byte 4 = 0) through GetMem, RETF 0Ch. Then MOV SP,BP; POP BP; RETF 0Ah.
 * Every far pointer is reread from the frame at each use (LES at each use). CS is the unit's load segment; the far-call
 * segment words CS:111E, CS:1129, CS:1145 and CS:1160 are loader fixups, read live. The 2Bh and (A+3) pushes carry the
 * entry AH; MOV AX,3Ch then clears AH (+001F preserves it, so the first 1Fh push is 001Fh); the 0Ah and 01h pushes carry
 * StrLoad's AH and the second 1Fh push carries 2199's. Flat 1 MiB A20-off RAM: offsets wrap at 16 bits, physical
 * addresses at 1 MiB, word operands are physically contiguous. Flags follow every flag-setting instruction (SUB SP; the
 * ORs; the CMP; XOR AX,AX; DEC byte keeps CF) over each callee's own exit flags. No undefined flag. See
 * docs/re/OVL_0038_10DE_CPU.md for the contract and evidence. */
typedef struct {
    uint16_t ax, bx, cx, dx, si, di;
    uint16_t bp, sp, cs, ip, ds, es, ss, flags;
} pr_u38_10de_cpu;

typedef enum {
    PR_U38_10DE_OK = 0,         /* RETF 0Ah executed (the gate return, the found return, or after the second append) */
    PR_U38_10DE_ARGUMENT,       /* null RAM, CPU or limits, RAM not 1 MiB, IP not 10DEh; nothing changed */
    PR_U38_10DE_CODE,           /* the loaded stub 00B0:0052 at +115D is not the far JMP to +0E52; IP at the call, nothing pushed */
    PR_U38_10DE_CALLEE,         /* +001F, StrLoad, 2199 or +0E52 stopped (diagnostic.callee and callee_status); state as it left it */
    PR_U38_10DE_TRANSFER,       /* a callee returned somewhere other than the instruction after its call */
    PR_U38_10DE_PENDING         /* +1126 LCALL 00BA:16EC reached (RES_00BA_16EC not closable); IP 1126, six words pushed */
} pr_u38_10de_status;

typedef struct {
    unsigned heap_steps;            /* GetMem's free-list walk cap, for +001F's append and for +0E52 (may be 0) */
    size_t links;                   /* the tail-walk cap of +0E52, for both appends (may be 0) */
    unsigned lookup_loopbacks;      /* 2199's loop-back cap (a cyclic list without the key stops at LIMIT) */
} pr_u38_10de_limits;

typedef struct {
    int callee;                     /* -1 none; 1 = +001F, 2 = StrLoad 0AF8:0B0B, 3 = 16EC (PENDING), 4 = 2199, 5 = U9E +0E52 (stub 0052) */
    int callee_status;              /* the last callee's own status (-1 when none ran) */
    uint16_t site;                  /* the call site of the last callee or stop (0 when none) */
    pr_u38_001f_diagnostic d001f;   /* +001F's diagnostic (append and GetMem statuses, -1 until they run) */
    int getmem_status;              /* GetMem's status inside the second append (-1 until it runs) */
} pr_u38_10de_diagnostic;

/* diagnostic may be NULL. limits must not. */
pr_u38_10de_status pr_u38_handler_10de_cpu(uint8_t *ram, size_t ram_size, pr_u38_10de_cpu *cpu,
                                           const pr_u38_10de_limits *limits, pr_u38_10de_diagnostic *diagnostic);
#endif
