#ifndef POOLRAD_RECOVERED_OVERLAY_U1C_GOTO_00F4_CPU_H
#define POOLRAD_RECOVERED_OVERLAY_U1C_GOTO_00F4_CPU_H

#include <stddef.h>
#include <stdint.h>

#define PR_U1C_GOTO_RAM_SIZE (1u << 20)

typedef struct {
    uint16_t ax, bx, cx, dx, si, di;
    uint16_t bp, sp, cs, ip, ds, es, ss, flags;
} pr_u1c_goto_cpu;

typedef struct {
    uint16_t system_cs;            /* the loaded SYSTEM segment U25 +0000's string cores call */
    size_t max_operands;           /* U25 +0000's operand budget (positive) */
    size_t max_string_iterations;  /* U25 +0000's string-core budget (positive) */
} pr_u1c_goto_limits;

typedef enum {
    PR_U1C_GOTO_NO_CALLEE = -1,
    PR_U1C_GOTO_LOAD_OPERANDS = 0,
    PR_U1C_GOTO_WORD
} pr_u1c_goto_callee;

typedef enum {
    PR_U1C_GOTO_OK = 0,
    PR_U1C_GOTO_ARGUMENT,  /* null RAM/CPU/limits or RAM that is not exactly 1 MiB; nothing changed */
    PR_U1C_GOTO_CODE,      /* a reached LCALL or the overlay stub it names is not as expected; stopped before it, IP at the site */
    PR_U1C_GOTO_CALLEE,    /* a callee model stopped; *callee names it and *callee_status holds its status */
    PR_U1C_GOTO_TRANSFER   /* a callee's RETF popped a CS:IP other than the call's next instruction */
} pr_u1c_goto_status;

/* Raw-CPU, full-state model of the complete GAME.OVR unit 0x1C routine
 * +00F4..+0110 (29 bytes, 13 instructions), the ECL GOTO handler, entered
 * after a far call with SS:[SP] return IP and [SP+2] return CS; RETF. It is
 * the raw-CPU counterpart of the provisional wrapper pr_u1c_goto_00f4.
 * PUSH BP; MOV BP,SP; MOV AL,1; PUSH AX and LCALL stub:002A (EA to U25
 * +0000) load one ECL operand; MOV AL,[7086h]; PUSH AX; MOV AL,[7046h];
 * PUSH AX and LCALL stub:004D (EA to U25 +06AF) assemble AX := (DS:[7046h]
 * << 8) + DS:[7086h], the operand's two value bytes; MOV [49EDh],AX sets
 * the script PC to it; POP BP; RETF. The callees are the merged raw-CPU
 * pr_u25_load_operands_0000 (with the limits' SYSTEM segment and budgets)
 * and pr_u25_word_06af; each LCALL's segment word and each stub's EA target
 * are read from RAM. Every operand is reread from RAM in source order. The
 * routine writes no flags of its own: the final flags are +06AF's, and at a
 * CODE stop they are the entry flags (first site) or U25 +0000's (second).
 * Offsets wrap at 16 bits, physical addresses at 1 MiB; POP and this
 * routine's RETF frame are read physically contiguous, and each callee
 * keeps its own contract. *callee and *callee_status (either may be NULL)
 * receive the last callee run and its status, or NO_CALLEE and -1. */
pr_u1c_goto_status pr_u1c_goto_00f4_cpu(uint8_t *ram, size_t ram_size, pr_u1c_goto_cpu *cpu,
                                             const pr_u1c_goto_limits *limits, pr_u1c_goto_callee *callee,
                                             int *callee_status);

#endif
