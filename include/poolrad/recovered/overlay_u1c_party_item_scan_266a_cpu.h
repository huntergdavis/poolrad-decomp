#ifndef POOLRAD_RECOVERED_OVERLAY_U1C_PARTY_ITEM_SCAN_266A_CPU_H
#define POOLRAD_RECOVERED_OVERLAY_U1C_PARTY_ITEM_SCAN_266A_CPU_H

#include <stddef.h>
#include <stdint.h>

#define PR_U1C_266A_RAM_SIZE (1u << 20)

typedef struct {
    uint16_t ax, bx, cx, dx, si, di;
    uint16_t bp, sp, cs, ip, ds, es, ss, flags;
} pr_u1c_266a_cpu;

typedef struct {
    uint16_t system_cs;             /* the loaded SYSTEM segment, which U25 +0000's string cores require */
    size_t max_operands;            /* U25 +0000's operand budget (positive) */
    size_t max_string_iterations;   /* U25 +0000's per-string budget (positive) */
    unsigned max_loopbacks;         /* cap on this routine's taken JMP +26CB / JMP +26AA */
} pr_u1c_266a_limits;

typedef enum {
    PR_U1C_266A_OK = 0,
    PR_U1C_266A_ARGUMENT,  /* null RAM/CPU/limits or RAM that is not exactly 1 MiB; nothing changed */
    PR_U1C_266A_CODE,      /* a reached LCALL, or the stub it names, is not the expected far call or loaded
                              EA jump; stopped before the LCALL, IP at the site */
    PR_U1C_266A_CALLEE,    /* a callee model stopped; *callee names it and *callee_status holds its status */
    PR_U1C_266A_TRANSFER,  /* a callee's RETF popped a CS:IP other than the call's next instruction */
    PR_U1C_266A_LIMIT      /* host cap on the walks' loop-backs; IP at the loop head the JMP reaches */
} pr_u1c_266a_status;

typedef enum {
    PR_U1C_266A_NO_CALLEE = -1,
    PR_U1C_266A_LOAD_OPERANDS = 0,  /* U25 +0000 through stub xxxx:002A */
    PR_U1C_266A_SLOT_VALUE,         /* U25 +0136 through stub xxxx:0025 */
    PR_U1C_266A_FILLCHAR            /* SYSTEM FillChar 0AF8:16A8 */
} pr_u1c_266a_callee;

/* Raw-CPU, full-state model of the complete GAME.OVR unit 0x1C routine
 * +266A..+271E (181 bytes, 61 instructions), entered after a far call with
 * SS:[SP] return IP and [SP+2] return CS; plain RETF. After PUSH BP;
 * MOV BP,SP; SUB SP,0Ch:
 *   load one ECL operand (U25 +0000 with count 1, its low byte from AL = 1
 *   and AH as found); [BP-0Bh] := 0; v := AL of U25 +0136 with index 1;
 *   [BP-1] := v; FillChar(DS:6F78, 6, 0); DS:[6F79] := 1;
 *   for p := the far pointer DS:[5D96] while p <> nil and [BP-0Bh] = 0:
 *     for q := p^[0C8h] while q <> nil and [BP-0Bh] = 0:
 *       if q^[2Eh] = v: DS:[6F78] := 1, DS:[6F79] := 0, [BP-0Bh] := 1;
 *       q := q^[2Ah];
 *     p := p^[104h];
 * The two U25 calls are LCALL xxxx:002A and LCALL xxxx:0025 through the
 * overlay stub, which must hold its loaded JMP FAR (EA) to U25 +0000 and
 * +0136; they run as the strict raw-CPU pr_u25_load_operands_0000 (with
 * the limits' SYSTEM segment and budgets) and pr_u25_slot_value_0136, and
 * return straight to this routine. FillChar is LCALL xxxx:16A8 run by
 * pr_system_fillchar_16a8. Each must return to the next instruction.
 * Cyclic lists loop forever in DOS: max_loopbacks caps the taken JMPs and
 * the next one stops at that loop head (entry IP + 61h or + 40h) with
 * LIMIT and every earlier effect kept. U25 +0136 has no loop budget of its
 * own, so a cyclic party list it walks does not return, as in DOS. The
 * final flags are the last OR or CMP's, with DF clear from FillChar;
 * SUB SP,0Ch's flags are computed for a CODE stop at the first site.
 * Offsets wrap at 16 bits, physical addresses at 1 MiB; LES pairs, POP and
 * this routine's RETF frame are read physically contiguous. *callee and
 * *callee_status (either may be NULL) receive the last callee run and its
 * status, or NO_CALLEE and -1. */
pr_u1c_266a_status pr_u1c_party_item_scan_266a_cpu(uint8_t *ram, size_t ram_size, pr_u1c_266a_cpu *cpu,
                                                   const pr_u1c_266a_limits *limits, pr_u1c_266a_callee *callee,
                                                   int *callee_status);

#endif
