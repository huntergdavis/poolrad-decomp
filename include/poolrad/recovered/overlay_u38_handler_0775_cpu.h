#ifndef POOLRAD_RECOVERED_OVERLAY_U38_HANDLER_0775_CPU_H
#define POOLRAD_RECOVERED_OVERLAY_U38_HANDLER_0775_CPU_H
#include <stddef.h>
#include <stdint.h>
#include "poolrad/recovered/resident_00ba_sidecar_lookup_2199_cpu.h"
#include "poolrad/recovered/system_string_leaves_cpu.h"
#include "poolrad/recovered/overlay_u9e_list_release_0026_cpu.h"

/* Raw-CPU, full-state model of GAME.OVR unit 38h +[0775,07D2) (93 bytes, 40 instructions, RETF 0Ah), the handler that
 * the 009E dispatcher reaches through slot 22 (public entry 20, stub 0041:0084). It is a PARTIAL CHAIN: one callee,
 * OVL_0038_0058, has no model, and the body stops before its call. Nothing is skipped or treated as a success.
 * Arguments (after the far call: [SP] IP, [SP+2] CS): [BP+6] a byte and [BP+8] a far pointer A, both unused by this body;
 * [BP+0Ch] a far pointer B (a character record). Locals (SUB SP,16h): [BP-4] a far-pointer cursor cell, [BP-15h] a 21-byte
 * string buffer.
 *   1. 00BA:2199 (the credited sidecar lookup) with (B, key word AX = AH:37h, SS:BP-4): it walks the list at B+7Fh/+81h for
 *      a node whose byte 0 is 37h and returns AL nonzero when found.
 *   2. Found: push B, AX = AH:06h and SS:BP-15h; 0AF8:0B0B (SYSTEM StrLoad) copies the 16-character string at CS:0764 into
 *      SS:BP-15h and pops only its source (RETF 4); PUSH CS; then +07AB CALL +0058 (OVL_0038_0058, unmodelled): the model
 *      stops PENDING at +07AB with CS and the five words (B, AX, the buffer pointer) still pushed. Its RETF 0Ah, the rest of
 *      the body and the return are not completed paths.
 *   3. Not found: DS:681F := 1; push B, AX = AH:0Fh, then AX := 0 and DX := 0 pushed (a nil item); LCALL 00B0:002A: the
 *      loaded stub of unit 9E entry 2 (five bytes at the fixup segment read live at CS:07C5) must be EAh 26 00 (a far JMP to
 *      +0026), checked before the call pushes anything (CODE otherwise); the composed pr_u9e_list_release_0026_cpu runs at
 *      the stub's segment (a nil item searches B+7Fh by the type byte 0Fh; its own CODE/CALLEE/TRANSFER/PENDING/LIMIT stops
 *      are reported as CALLEE here, with the state as it left it). Then DS:681F := 0; MOV SP,BP; POP BP; RETF 0Ah.
 * Every far pointer is reread from the frame at each use, in source order. CS is the unit's load segment; the far-call
 * segment words CS:078C, CS:07A8 and CS:07C5 are loader fixups, read live. AH is never cleared before the key, 06h and 0Fh
 * pushes: the callees see the live AH. Flat 1 MiB A20-off RAM: offsets wrap at 16 bits, physical addresses at 1 MiB, word
 * operands are physically contiguous. See docs/re/OVL_0038_0775_CPU.md for the contract and evidence. */
typedef struct {
    uint16_t ax, bx, cx, dx, si, di;
    uint16_t bp, sp, cs, ip, ds, es, ss, flags;
} pr_u38_0775_cpu;

typedef enum {
    PR_U38_0775_OK = 0,         /* RETF 0Ah executed (only reachable on the not-found path) */
    PR_U38_0775_ARGUMENT,       /* null RAM, CPU or limits, RAM not 1 MiB, IP not 0775h; nothing changed */
    PR_U38_0775_CODE,           /* +07C2: the loaded stub at (CS:07C5):002A is not EAh 26 00; IP 07C2, nothing pushed */
    PR_U38_0775_CALLEE,         /* 2199, StrLoad or U9E +0026 stopped (diagnostic.callee and callee_status); state as it left it */
    PR_U38_0775_TRANSFER,       /* a callee returned somewhere other than the instruction after its call */
    PR_U38_0775_PENDING         /* +07AB CALL +0058 reached (OVL_0038_0058 has no model); CS pushed at +07AA, IP 07AB */
} pr_u38_0775_status;

typedef struct {
    unsigned lookup_loopbacks;      /* 2199's cap on taken loop-backs (a cyclic list without the key never ends) */
    pr_u9e_0026_limits release;     /* U9E +0026's list and heap caps */
} pr_u38_0775_limits;

typedef struct {
    int callee;                     /* -1 none; 1 = 00BA:2199, 2 = StrLoad 0AF8:0B0B, 3 = U9E +0026 (through the stub), 4 = +0058 (PENDING) */
    int callee_status;              /* the last callee's own status (-1 when none ran) */
    uint16_t site;                  /* the call site of the last callee or stop (0 when none) */
    pr_u9e_0026_diagnostic d0026;   /* U9E +0026's diagnostic (freemem_status -1 until it runs) */
} pr_u38_0775_diagnostic;

/* diagnostic may be NULL. limits must not. */
pr_u38_0775_status pr_u38_handler_0775_cpu(uint8_t *ram, size_t ram_size, pr_u38_0775_cpu *cpu,
                                           const pr_u38_0775_limits *limits, pr_u38_0775_diagnostic *diagnostic);
#endif
