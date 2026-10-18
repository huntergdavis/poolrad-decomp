#ifndef POOLRAD_RECOVERED_OVERLAY_U38_STRING_MATCH_CPU_H
#define POOLRAD_RECOVERED_OVERLAY_U38_STRING_MATCH_CPU_H

#include <stddef.h>
#include <stdint.h>

#define PR_U38_STRING_MATCH_RAM_SIZE (1u << 20)

typedef struct {
    uint16_t ax, bx, cx, dx, si, di;
    uint16_t bp, sp, cs, ip, ds, es, ss, flags;
} pr_u38_string_match_cpu;

typedef enum {
    PR_U38_STRING_MATCH_OK = 0,
    PR_U38_STRING_MATCH_ARGUMENT,  /* null RAM/CPU, RAM that is not exactly 1 MiB, or IP not the entry; nothing changed */
    PR_U38_STRING_MATCH_CODE,      /* the LCALL is not 9A 10 0C; stopped at it with its four argument words pushed */
    PR_U38_STRING_MATCH_CALLEE,    /* the string compare stopped; *callee_status holds its status */
    PR_U38_STRING_MATCH_TRANSFER,  /* the compare's RETF popped a CS:IP other than the call's next instruction */
    PR_U38_STRING_MATCH_LIMIT      /* max_loopbacks loop-backs were taken; the next stops at the loop's INC */
} pr_u38_string_match_status;

/* Raw-CPU, full-state model of GAME.OVR unit 0x38 +0679..+06E3 (107 bytes,
 * 37 instructions), the U9E effect handler in slot 18 (public entry 17),
 * entered after a far call with five argument words, of which only the far
 * pointer [BP+0Ch] is read; RETF 0Ah. After PUSH BP; MOV BP,SP; SUB SP,2:
 *  1. LES DI,[BP+0Ch]; LES DI,ES:[DI+108h]; the far pointer ES:[DI+0Ah]/
 *     [DI+0Ch] is stored at DS:[6826h]/[6828h] (call it P).
 *  2. Unless P's bytes +9Fh and +6Ch are both 1 (CMP; JNE), it returns.
 *  3. For i = 1..4 in the byte [BP-1], LCALL 0AF8:0C10 at +06CD, the merged
 *     raw-CPU SYSTEM string compare pr_system_strcmp_0c10, compares P's
 *     string (left) with the table string DS:(0332h + 16i) (right): XOR
 *     AH,AH; SHL DI,4; ADD DI,0332h. Each equal pair (ZF) increments the
 *     byte DS:[6822h]; CMP BYTE [BP-1],4; JNE loops back to INC [BP-1].
 *  4. MOV SP,BP; POP BP; RETF 0Ah.
 * P and every operand are reread from RAM in source order; LES pairs, word
 * operands, POP and the RETF frame are read physically contiguous; offsets
 * wrap at 16 bits, physical addresses at 1 MiB. Flags follow every
 * flag-setting instruction (SHL's never survive ADD DI). On return the flags
 * are the last CMP's, and AX, BX, CX, DX, SI, DI, ES and DF are the last
 * compare's when one ran. The counter can alias DS:[6822h] or the strings,
 * so max_loopbacks caps the taken JNE loop-backs (three when nothing
 * aliases). The code must sit at its original offsets (IP = 0679h).
 * *callee_status (may be NULL) receives the last compare's status, or -1. */
pr_u38_string_match_status pr_u38_string_match_0679_cpu(uint8_t *ram, size_t ram_size, pr_u38_string_match_cpu *cpu,
                                                        unsigned max_loopbacks, int *callee_status);

/* Raw-CPU, full-state model of GAME.OVR unit 0x38 +0947..+09B1 (107 bytes,
 * 37 instructions), the U9E effect handler in slot 26 (public entry 23):
 * +0679's code shifted by 2CEh, comparing with the table strings
 * DS:(0372h + 16i) for i = 1..8 (ADD DI,0372h; CMP BYTE [BP-1],8). The
 * LCALL is at +099B, the loop-back INC at +0981, and RETF 0Ah at +09AF;
 * without aliasing the loop takes seven loop-backs. Otherwise as above,
 * with IP = 0947h. */
pr_u38_string_match_status pr_u38_string_match_0947_cpu(uint8_t *ram, size_t ram_size, pr_u38_string_match_cpu *cpu,
                                                        unsigned max_loopbacks, int *callee_status);

#endif
