#ifndef POOLRAD_RECOVERED_OVERLAY_U7D_TWO_DIGITS_0565_CPU_H
#define POOLRAD_RECOVERED_OVERLAY_U7D_TWO_DIGITS_0565_CPU_H

#include <stddef.h>
#include <stdint.h>

#define PR_U7D_0565_RAM_SIZE (1u << 20)

typedef struct {
    uint16_t ax, bx, cx, dx, si, di;
    uint16_t bp, sp, cs, ip, ds, es, ss, flags;
} pr_u7d_0565_cpu;

typedef enum {
    PR_U7D_0565_NO_CALLEE = -1,
    PR_U7D_0565_BYTE_STRING = 0,  /* resident 00BA:1037, pr_res_00ba_1037_cpu */
    PR_U7D_0565_STRSTORE,         /* SYSTEM 0B25, pr_system_strstore_0b25 */
    PR_U7D_0565_STRLOAD,          /* SYSTEM 0B0B, pr_system_strload_0b0b */
    PR_U7D_0565_STRCAT,           /* SYSTEM 0B98, pr_system_strcat_0b98 */
    PR_U7D_0565_COPY              /* SYSTEM 0B57, pr_system_strcopy_0b57 */
} pr_u7d_0565_callee;

typedef enum {
    PR_U7D_0565_OK = 0,
    PR_U7D_0565_ARGUMENT,  /* null RAM/CPU, RAM that is not exactly 1 MiB, or IP not 0565h; nothing changed */
    PR_U7D_0565_CODE,      /* a reached LCALL's opcode and offset are not the original's; stopped at it, its
                              arguments pushed */
    PR_U7D_0565_CALLEE,    /* a callee stopped (00BA:1037's CODE, CALLEE or TRANSFER, or an ARGUMENT);
                              *callee_status holds its status */
    PR_U7D_0565_TRANSFER   /* a callee's RETF popped a CS:IP other than the call's next instruction */
} pr_u7d_0565_status;

/* Raw-CPU, full-state model of the complete GAME.OVR unit 0x7D routine
 * +0565..+05E2 (126 bytes, 54 instructions; the inventory's 139-byte row
 * also spans the 13 constant bytes after its RETF), entered after a far
 * call: SS:[SP] return IP, [SP+2] return CS, [SP+4] the byte b and
 * [SP+6]/[SP+8] the far result R; RETF 2 leaves R on the stack. It writes b
 * as a two-character string. After PUSH BP; MOV BP,SP; SUB SP,12Ah, with the
 * 256-byte temporary T = SS:BP-129h and s = SS:BP-29h (string[40]):
 *  1. T := 00BA:1037(b) (LCALL 00BA:1037, AL := b over the entry AH); s := T
 *     (StrStore, maximum 40).
 *  2. CMP BYTE [BP+6],0Ah; if b < 10: T := the constant '0' at CS:0563
 *     (StrLoad), T := T + s (StrConcat), s := T (StrStore, 40).
 *  3. T := Copy(s, 1, 2); R := T (StrStore, 40).
 *  4. MOV SP,BP; POP BP; RETF 2.
 * Each LCALL's opcode and offset are checked when reached and its segment is
 * read from RAM; the callees are the merged raw-CPU models listed in
 * pr_u7d_0565_callee. Every operand is reread from RAM in source order; LES
 * pairs, POP and the RETF frame are read physically contiguous; offsets wrap
 * at 16 bits, physical addresses at 1 MiB. Flags: on return they are the
 * last StrStore's; at a CODE stop they are those of the previous writer
 * (SUB SP,12Ah at the first site, CMP at StrLoad's or, for b >= 10, at
 * Copy's, otherwise the previous callee's). *callee and *callee_status
 * (either may be NULL) receive the last callee run and its status, or
 * NO_CALLEE and -1. */
pr_u7d_0565_status pr_u7d_two_digits_0565_cpu(uint8_t *ram, size_t ram_size, pr_u7d_0565_cpu *cpu, pr_u7d_0565_callee *callee,
                                              int *callee_status);

#endif
