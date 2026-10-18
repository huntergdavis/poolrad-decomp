#ifndef POOLRAD_RECOVERED_OVERLAY_U1C_COMPARE_AND_0AED_CPU_H
#define POOLRAD_RECOVERED_OVERLAY_U1C_COMPARE_AND_0AED_CPU_H

#include <stddef.h>
#include <stdint.h>

#define PR_U1C_AND_0AED_RAM_SIZE (1u << 20)

typedef struct {
    uint16_t ax, bx, cx, dx, si, di;
    uint16_t bp, sp, cs, ip, ds, es, ss, flags;
} pr_u1c_and_0aed_cpu;

typedef struct {
    uint16_t system_cs;            /* the loaded SYSTEM segment U25 +0000's string cores call */
    size_t max_operands;           /* U25 +0000's operand budget (positive) */
    size_t max_string_iterations;  /* U25 +0000's string-core budget (positive) */
} pr_u1c_and_0aed_limits;

typedef enum {
    PR_U1C_AND_0AED_NO_CALLEE = -1,
    PR_U1C_AND_0AED_FILLCHAR = 0,
    PR_U1C_AND_0AED_LOAD_OPERANDS,
    PR_U1C_AND_0AED_SLOT_VALUE
} pr_u1c_and_0aed_callee;

typedef enum {
    PR_U1C_AND_0AED_OK = 0,
    PR_U1C_AND_0AED_ARGUMENT,  /* null RAM/CPU/limits or RAM that is not exactly 1 MiB; nothing changed */
    PR_U1C_AND_0AED_CODE,      /* a reached LCALL or the overlay stub it names is not as expected; stopped before it, IP at the site */
    PR_U1C_AND_0AED_CALLEE,    /* a callee model stopped; *callee names it and *callee_status holds its status */
    PR_U1C_AND_0AED_TRANSFER   /* a callee's RETF popped a CS:IP other than the call's next instruction */
} pr_u1c_and_0aed_status;

/* Raw-CPU, full-state model of the complete GAME.OVR unit 0x1C routine
 * +0AED..+0B51 (101 bytes, 40 instructions), the ECL COMPARE AND handler,
 * entered after a far call with SS:[SP] return IP and [SP+2] return CS;
 * RETF. It is the raw-CPU counterpart of the provisional hook model
 * pr_u1c_compare_and_0aed. After PUSH BP; MOV BP,SP; SUB SP,0Ah:
 * FillChar(DS:6F78h, 6, 0) (LCALL xxxx:16A8); load four ECL operands
 * (MOV AL,4; PUSH AX; LCALL stub:002A, the stub's EA jump reaching U25
 * +0000); for i := 1 to 4 (byte at BP-9): v[i] := slot value i (PUSH AX
 * with AL = i; LCALL stub:0025 reaching U25 +0136; MOV DX,AX), stored at
 * BP-0Ah + 2i; then DS:[6F78h] := 1 when v[1] = v[2] and v[3] = v[4], else
 * DS:[6F79h] := 1; MOV SP,BP; POP BP; RETF. The callees are the merged
 * raw-CPU pr_system_fillchar_16a8, pr_u25_load_operands_0000 (with the
 * limits' SYSTEM segment and budgets) and pr_u25_slot_value_0136; each
 * LCALL's segment word and each stub's EA target are read from RAM. The
 * callees write data only before i is set (U25 +0000) or in stack frames
 * below this one, so the loop always makes four passes; a cyclic party
 * list inside U25 +0136 does not terminate, as in the original. Every
 * operand is reread from RAM in source order. Flags: the final flags are
 * the last CMP's (v[1]:v[2], or v[3]:v[4] when the first pair is equal); at
 * a CODE stop they are SUB SP,0Ah's at the FillChar and load sites (DF
 * cleared by FillChar at the latter), U25 +0000's at the first value site
 * and INC BYTE [BP-9]'s (CF from CMP BYTE [BP-9],4) at later ones; the
 * XOR and SHL writes are overwritten first and are folded. Offsets wrap at
 * 16 bits, physical addresses at 1 MiB; POP and this routine's RETF frame
 * are read physically contiguous, and each callee keeps its own contract.
 * *callee and *callee_status (either may be NULL) receive the last callee
 * run and its status, or NO_CALLEE and -1. */
pr_u1c_and_0aed_status pr_u1c_compare_and_0aed_cpu(uint8_t *ram, size_t ram_size, pr_u1c_and_0aed_cpu *cpu,
                                                   const pr_u1c_and_0aed_limits *limits, pr_u1c_and_0aed_callee *callee,
                                                   int *callee_status);

#endif
