#ifndef POOLRAD_RECOVERED_RESIDENT_00BA_BYTE_STRING_1037_CPU_H
#define POOLRAD_RECOVERED_RESIDENT_00BA_BYTE_STRING_1037_CPU_H

#include <stddef.h>
#include <stdint.h>

#define PR_RES_1037_RAM_SIZE (1u << 20)

typedef struct {
    uint16_t ax, bx, cx, dx, si, di;
    uint16_t bp, sp, cs, ip, ds, es, ss, flags;
} pr_res_1037_cpu;

typedef enum {
    PR_RES_1037_NO_CALLEE = -1,
    PR_RES_1037_STR = 0,       /* SYSTEM Str 1366, the pr_system_str_1366 of this batch */
    PR_RES_1037_STRSTORE       /* SYSTEM StrStore 0B25, the merged pr_system_strstore_0b25 */
} pr_res_1037_callee;

typedef enum {
    PR_RES_1037_OK = 0,
    PR_RES_1037_ARGUMENT,  /* null RAM/CPU, RAM that is not exactly 1 MiB, or IP not 1037h; nothing changed */
    PR_RES_1037_CODE,      /* a reached LCALL is not 9A 66 13 (Str) or 9A 25 0B (StrStore); stopped at it, its
                              arguments pushed */
    PR_RES_1037_CALLEE,    /* a callee rejected its entry (Str's power table is not the original); *callee_status
                              holds its status */
    PR_RES_1037_TRANSFER   /* a callee's RETF popped a CS:IP other than the call's next instruction */
} pr_res_1037_status;

/* Raw-CPU, full-state model of the complete resident routine 00BA:1037..1072
 * (60 bytes, 28 instructions) in the expanded START.EXE image, entered after
 * a far call: SS:[SP] return IP, [SP+2] return CS, [SP+4] the byte b and
 * [SP+6]/[SP+8] the far result R; RETF 2 leaves R on the stack. After PUSH
 * BP; MOV BP,SP; SUB SP,100h:
 *  1. Str(Longint(b) : 0, T) into the 256-byte temporary T = SS:BP-100h with
 *     maximum length 255 (LCALL xxxx:1366, its segment read from RAM).
 *  2. StrStore(T, R, 255) (LCALL xxxx:0B25).
 *  3. MOV SP,BP; POP BP; RETF 2.
 * Str is pr_system_str_1366 and StrStore the merged pr_system_strstore_0b25.
 * Every operand is reread from RAM in source order; LES pairs, POP and the
 * RETF frame are read physically contiguous; offsets wrap at 16 bits,
 * physical addresses at 1 MiB. Flags: at a Str site they are XOR AX,AX's;
 * at the StrStore site and on return they are the callees'. *callee and
 * *callee_status (either may be NULL) receive the last callee run and its
 * status, or NO_CALLEE and -1. */
pr_res_1037_status pr_res_00ba_1037_cpu(uint8_t *ram, size_t ram_size, pr_res_1037_cpu *cpu, pr_res_1037_callee *callee,
                                        int *callee_status);

#endif
