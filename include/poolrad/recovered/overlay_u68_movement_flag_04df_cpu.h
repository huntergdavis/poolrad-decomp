#ifndef POOLRAD_RECOVERED_OVERLAY_U68_MOVEMENT_FLAG_04DF_CPU_H
#define POOLRAD_RECOVERED_OVERLAY_U68_MOVEMENT_FLAG_04DF_CPU_H

#include <stddef.h>
#include <stdint.h>

#define PR_U68_04DF_RAM_SIZE (1u << 20)

typedef struct {
    uint16_t ax, bx, cx, dx, si, di;
    uint16_t bp, sp, cs, ip, ds, es, ss, flags;
} pr_u68_04df_cpu;

typedef enum {
    PR_U68_04DF_OK = 0,
    PR_U68_04DF_ARGUMENT,  /* null RAM/CPU, RAM that is not exactly 1 MiB, or IP not 04DFh; nothing changed */
    PR_U68_04DF_CODE,      /* the +0513 LCALL is not 9A 48 00 or its loaded stub is not EA E3 0D; stopped at the site */
    PR_U68_04DF_CALLEE,    /* the dice chain or +0109 stopped; *callee_status holds its status */
    PR_U68_04DF_TRANSFER,  /* a callee's RETF popped a CS:IP other than the call's next instruction in this CS */
    PR_U68_04DF_LIMIT      /* max_loopbacks JMP 04F6 loop-backs were taken; the next stops at +04F6 */
} pr_u68_04df_status;

/* Raw-CPU, full-state model of GAME.OVR unit 0x68 +04DF..+05AA (204 bytes,
 * 81 instructions), the strict form of the bounded movement flag
 * (overlay_u68_movement_flag.h, whose roll and +0109 calls are hooks).
 * Entered after a far call (PUSH CS; CALL from entry 1), no arguments;
 * RETF. After PUSH BP; MOV BP,SP; SUB SP,0Ah:
 *  1. Until one succeeds, each party member (head DS:[5D96h], link
 *     +104h/+106h) rolls U9E entry 8 with the pushed words (AH:1, AH:64h)
 *     through LCALL 00B0:0048 and its loaded stub (EA 0DE3 seg), the merged
 *     pr_u9e_entry8_fullstate_compose_0de3 with max_system_calls 256; a
 *     roll AL not above the member's byte +78h, with its byte +10Ch zero,
 *     succeeds. The link is read even after a success.
 *  2. DS:[6DDFh] := 0.
 *  3. On success, +0109 (the raw pr_u68_map_update_0109_cpu, PUSH CS; CALL)
 *     marks (x, y, facing) = DS:[6AADh], [6AAEh], [6AAFh], then the
 *     neighbour (x + t1[facing], y + t2[facing], (facing + 4) mod 8) with
 *     the signed tables t1 at DS:2736h and t2 at DS:273Fh (CBW; ADD; CWD;
 *     IDIV), all reread after the first call.
 *  4. AL := the success byte; MOV SP,BP; POP BP; RETF.
 * Every operand is reread from RAM in source order; LES pairs, word
 * operands, POP and the RETF frame are read physically contiguous; offsets
 * wrap at 16 bits, physical addresses at 1 MiB. Flags follow every
 * flag-setting instruction (IDIV's undefined ones never survive +0109's
 * CMP). The party list can be cyclic in RAM, so max_loopbacks caps the
 * taken JMP 04F6 loop-backs (one per member rolled). The code must sit at
 * its original offsets (IP = 04DFh), with +0109 at its own. *callee_status
 * (may be NULL) receives the last callee's status, or -1. */
pr_u68_04df_status pr_u68_movement_flag_04df_cpu(uint8_t *ram, size_t ram_size, pr_u68_04df_cpu *cpu, unsigned max_loopbacks,
                                              int *callee_status);

#endif
